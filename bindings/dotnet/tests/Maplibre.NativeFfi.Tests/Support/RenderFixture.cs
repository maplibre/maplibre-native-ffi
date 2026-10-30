using System.Diagnostics;
using System.Runtime.ExceptionServices;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// An owned-texture session on a context from tests/graphics, driven by the managed thread that
/// owns the context.
/// </summary>
/// <remarks>
/// Every backend attaches with the caller driver: an OpenGL session shares the fixture's context,
/// which is current only on its graphics thread, and the other backends take the same path so one
/// test covers them all. The session's frame and driver-work wakes release a semaphore, and the
/// fixture blocks on it between services.
/// </remarks>
internal sealed class RenderFixture : IDisposable
{
    internal const uint Width = 32;
    internal const uint Height = 16;

    /// <summary>A style that paints every pixel opaque red.</summary>
    internal const string RedStyle = """
        {"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ff0000"}}]}
        """;

    private readonly SemaphoreSlim wake = new(0);
    private ulong token;

    /// <summary>The graphics backends this build renders with, from its backend masks.</summary>
    internal static TheoryData<TestGraphicsBackend> Backends
    {
        get
        {
            var data = new TheoryData<TestGraphicsBackend>();
            var backends = MaplibreNative.SupportedRenderBackendMask();
            if (backends.HasFlag(RenderBackendFlag.Metal))
                data.Add(TestGraphicsBackend.Metal);
            if (backends.HasFlag(RenderBackendFlag.Vulkan))
                data.Add(TestGraphicsBackend.Vulkan);
            if (backends.HasFlag(RenderBackendFlag.OpenGl))
            {
                var providers = MaplibreNative.OpenGlSupportedContextProviderMask();
                if (providers.HasFlag(OpenGlContextProviderFlag.Egl))
                    data.Add(TestGraphicsBackend.Egl);
                if (providers.HasFlag(OpenGlContextProviderFlag.Wgl))
                    data.Add(TestGraphicsBackend.Wgl);
            }
            return data;
        }
    }

    private RenderFixture(TestGraphicsBackend backend, Configure configure)
    {
        Backend = backend;
        Graphics = TestGraphics.Create(backend);
        try
        {
            // An OpenGL host drives a shared-context session from the thread its context is
            // current on, the way Rust and Kotlin attach on WGL and EGL.
            if (backend is TestGraphicsBackend.Egl or TestGraphicsBackend.Wgl)
                Graphics.MakeCurrent();
            Runtime = RuntimeHandle.Create(new RuntimeOptions());
            Runtime.Discarding.SetResourceProvider(NativeFixture.DenyingProvider);
            Map = Runtime.CreateMap(
                NativeFixture.SmallMap with
                {
                    InitialExtent = new LogicalExtent(Width, Height, 1),
                }
            );
            Complete(Map.Ready);
            Complete(Map.SetStyleJsonAsync(RedStyle));
            Session = Attach(
                configure(
                    Signal,
                    new RenderSessionAttachOptions
                    {
                        Driver = RenderDriverKind.CallerGraphicsThread,
                        RequestedTextureRingDepth = 2,
                        FrameWake = new Wake(Signal),
                        DriverWorkWake = new Wake(Signal),
                    }
                )
            );
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    internal TestGraphicsBackend Backend { get; }

    internal TestGraphics Graphics { get; }

    internal RuntimeHandle Runtime { get; } = null!;

    internal MapHandle Map { get; } = null!;

    /// <summary>The attached session, which becomes ready once the fixture services it.</summary>
    internal RenderSessionHandle Session { get; } = null!;

    /// <summary>
    /// Changes the options that the session attaches with. It receives the fixture's wake, which
    /// every wake of the session calls so that the fixture makes progress.
    /// </summary>
    internal delegate RenderSessionAttachOptions Configure(
        Action wake,
        RenderSessionAttachOptions options
    );

    /// <summary>Runs a test on a new graphics thread that creates, drives, and disposes a fixture.</summary>
    internal static void Run(TestGraphicsBackend backend, Action<RenderFixture> test) =>
        Run(backend, static (_, options) => options, test);

    /// <summary>Runs a test on a fixture whose session attaches with the configured options.</summary>
    internal static void Run(
        TestGraphicsBackend backend,
        Configure configure,
        Action<RenderFixture> test
    )
    {
        ExceptionDispatchInfo? failure = null;
        var thread = new Thread(() =>
        {
            try
            {
                using var fixture = new RenderFixture(backend, configure);
                test(fixture);
            }
            catch (Exception error)
            {
                failure = ExceptionDispatchInfo.Capture(error);
            }
        })
        {
            IsBackground = true,
            Name = $"{backend} graphics thread",
        };
        thread.Start();
        thread.Join();
        failure?.Throw();
    }

    /// <summary>Services the session's driver on this thread until the operation completes.</summary>
    internal void Await(Task operation, string what = "a render session operation")
    {
        operation.ContinueWith(
            _ => Signal(),
            CancellationToken.None,
            TaskContinuationOptions.ExecuteSynchronously,
            TaskScheduler.Default
        );
        var elapsed = Stopwatch.StartNew();
        while (!operation.IsCompleted)
        {
            Session.ServiceDriverWork();
            if (!operation.IsCompleted)
                TestWaits.Wait(wake, elapsed, what);
        }
        operation.GetAwaiter().GetResult();
    }

    internal T Await<T>(Task<T> operation, string what = "a render session operation")
    {
        Await((Task)operation, what);
        return operation.Result;
    }

    /// <summary>Demands frames until one renders, and returns its result.</summary>
    internal RenderFrameResult RenderFrame()
    {
        var elapsed = Stopwatch.StartNew();
        while (true)
        {
            var demand = ++token;
            Session.RequestFrame(new FrameDemand { Token = demand });
            RenderFrameResult? result = null;
            while (result is null)
            {
                Session.ServiceDriverWork();
                result = DrainFrames().LastOrDefault(frame => frame.Token == demand);
                if (result is null)
                    TestWaits.Wait(wake, elapsed, "a rendered frame");
            }
            if (result.Value.Disposition == RenderResult.Rendered)
                return result.Value;
        }
    }

    public void Dispose()
    {
        try
        {
            // Dispose accepts the session in any state, and a test may already have disposed it.
            if (Session is not null)
                Session.Dispose();
        }
        finally
        {
            try
            {
                // The map's release completes once its session has retired.
                if (Map is not null)
                    Complete(Map.DisposeAsync().AsTask());
            }
            finally
            {
                try
                {
                    if (Runtime is not null)
                        Complete(Runtime.CloseAsync());
                }
                finally
                {
                    // Sessions retire before the context they borrowed goes away.
                    Graphics.Dispose();
                }
            }
        }
    }

    private IReadOnlyList<RenderFrameResult> DrainFrames() => Session.DrainFrameResults();

    /// <summary>Attaches an owned texture target of the fixture's backend to its map.</summary>
    internal RenderSessionHandle Attach(RenderSessionAttachOptions options)
    {
        var extent = new LogicalExtent(Width, Height, 1);
        var context = Graphics.Context;
        return Backend switch
        {
            TestGraphicsBackend.Metal
                when OperatingSystem.IsMacOS()
                    || OperatingSystem.IsIOS()
                    || OperatingSystem.IsTvOS()
                    || OperatingSystem.IsMacCatalyst() => Map.AttachMetalOwnedTexture(
                new MetalOwnedTextureDescriptor
                {
                    Extent = extent,
                    Context = new MetalContextDescriptor { Device = Borrow(context.MetalDevice) },
                    // MTLPixelFormatBGRA8Unorm.
                    PixelFormat = 80,
                },
                options
            ),
            TestGraphicsBackend.Vulkan
                when OperatingSystem.IsAndroid()
                    || OperatingSystem.IsLinux()
                    || OperatingSystem.IsMacOS()
                    || OperatingSystem.IsWindows() => Map.AttachVulkanOwnedTexture(
                new VulkanOwnedTextureDescriptor
                {
                    Extent = extent,
                    Context = new VulkanContextDescriptor
                    {
                        Instance = Borrow(context.VulkanInstance),
                        PhysicalDevice = Borrow(context.VulkanPhysicalDevice),
                        Device = Borrow(context.VulkanDevice),
                        GraphicsQueue = Borrow(context.VulkanQueue),
                        GraphicsQueueFamilyIndex = context.VulkanQueueFamilyIndex,
                        GetInstanceProcAddr = Borrow(context.VulkanGetInstanceProcAddr),
                        GetDeviceProcAddr = Borrow(context.VulkanGetDeviceProcAddr),
                    },
                },
                options
            ),
            TestGraphicsBackend.Egl => Map.AttachOpenGlOwnedTexture(
                new OpenGlOwnedTextureDescriptor
                {
                    Extent = extent,
                    Context = new OpenGlContextDescriptor(
                        OpenGlContextOwnership.Shared,
                        new OpenGlContextDescriptor.DataValue.Egl(
                            new EglContextDescriptor
                            {
                                Display = Borrow(context.EglDisplay),
                                Config = Borrow(context.EglConfig),
                                ShareContext = Borrow(context.EglContext),
                            }
                        )
                    ),
                },
                options
            ),
            TestGraphicsBackend.Wgl => Map.AttachOpenGlOwnedTexture(
                new OpenGlOwnedTextureDescriptor
                {
                    Extent = extent,
                    Context = new OpenGlContextDescriptor(
                        OpenGlContextOwnership.Shared,
                        new OpenGlContextDescriptor.DataValue.Wgl(
                            new WglContextDescriptor
                            {
                                DeviceContext = Borrow(context.WglDeviceContext),
                                ShareContext = Borrow(context.WglContext),
                            }
                        )
                    ),
                },
                options
            ),
            _ => throw new ArgumentOutOfRangeException(nameof(Backend)),
        };
    }

    // The semaphore is never disposed, so a wake or a completion continuation that lands after
    // the fixture has finished still has somewhere to go.
    private void Signal() => wake.Release();

    private static NativePointer Borrow(nint address) => NativePointer.FromBorrowedAddress(address);

    internal static void Complete(Task operation) =>
        Assert.True(operation.Wait(TestWaits.Deadline), "A native operation did not complete.");

    private static T Complete<T>(Task<T> operation)
    {
        Complete((Task)operation);
        return operation.Result;
    }
}
