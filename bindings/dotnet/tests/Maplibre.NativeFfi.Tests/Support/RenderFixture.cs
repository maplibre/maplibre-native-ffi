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
    internal static byte[] RedStyle =>
        """
            {"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ff0000"}}]}
            """u8.ToArray();

    private readonly SemaphoreSlim wake = new(0);
    private ulong token;

    /// <summary>The graphics backends this build renders with, from its backend masks.</summary>
    internal static TheoryData<TestGraphicsBackend> Backends
    {
        get
        {
            var data = new TheoryData<TestGraphicsBackend>();
            var backends = Maplibre.SupportedRenderBackendMask();
            if (backends.HasFlag(RenderBackendFlag.Metal))
                data.Add(TestGraphicsBackend.Metal);
            if (backends.HasFlag(RenderBackendFlag.Vulkan))
                data.Add(TestGraphicsBackend.Vulkan);
            if (backends.HasFlag(RenderBackendFlag.Opengl))
            {
                var providers = Maplibre.OpenglSupportedContextProviderMask();
                if (providers.HasFlag(OpenglContextProviderFlag.Egl))
                    data.Add(TestGraphicsBackend.Egl);
                if (providers.HasFlag(OpenglContextProviderFlag.Wgl))
                    data.Add(TestGraphicsBackend.Wgl);
            }
            return data;
        }
    }

    private RenderFixture(TestGraphicsBackend backend)
    {
        Backend = backend;
        Graphics = TestGraphics.Create(backend);
        try
        {
            // An OpenGL host drives a shared-context session from the thread its context is
            // current on, the way Rust and Kotlin attach on WGL and EGL.
            if (backend is TestGraphicsBackend.Egl or TestGraphicsBackend.Wgl)
                Graphics.MakeCurrent();
            Runtime = RuntimeHandle.Create(RuntimeOptions.Default);
            Complete(Runtime.SetResourceProviderAsync(NativeFixture.DenyingProvider));
            Map = Complete(
                Runtime.CreateMapAsync(
                    MapOptions.Default with
                    {
                        InitialExtent = new LogicalExtent(Width, Height, 1),
                    }
                )
            );
            Complete(Map.SetStyleJsonAsync(RedStyle));
            Session = Attach();
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

    /// <summary>The attached session. Its attachment completes once the fixture services it.</summary>
    internal RenderSessionHandle Session { get; } = null!;

    /// <summary>Runs a test on a new graphics thread that creates, drives, and disposes a fixture.</summary>
    internal static void Run(TestGraphicsBackend backend, Action<RenderFixture> test)
    {
        ExceptionDispatchInfo? failure = null;
        var thread = new Thread(() =>
        {
            try
            {
                using var fixture = new RenderFixture(backend);
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
            Session.ServiceDriverWork(0);
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
                Session.ServiceDriverWork(0);
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
            if (Session is { IsClosed: false })
                DetachAndClose();
        }
        finally
        {
            try
            {
                if (Map is { IsClosed: false })
                    Complete(Map.CloseAsync());
            }
            finally
            {
                try
                {
                    if (Runtime is { IsClosed: false })
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

    private void DetachAndClose()
    {
        if (
            Session.GetSnapshot().State
            is not (RenderSessionState.Detached or RenderSessionState.Abandoned)
        )
        {
            try
            {
                Await(Session.DetachAsync(), "the session to detach");
            }
            catch
            {
                try
                {
                    Session.Abandon();
                }
                finally
                {
                    Session.Close();
                }
                throw;
            }
        }
        Session.Close();
    }

    private IReadOnlyList<RenderFrameResult> DrainFrames()
    {
        using var batch = Session.DrainFrameResults();
        if (batch is null)
            return [];
        var results = new RenderFrameResult[checked((int)batch.Count())];
        for (var index = 0; index < results.Length; index++)
            results[index] = batch.Get((ulong)index);
        return results;
    }

    private RenderSessionHandle Attach()
    {
        var extent = new RenderTargetExtent(Width, Height, 1);
        var options = new RenderSessionAttachOptions
        {
            Driver = RenderDriverKind.CallerGraphicsThread,
            RequestedTextureRingDepth = 2,
            FrameWake = new Wake(Signal),
            DriverWorkWake = new Wake(Signal),
        };
        var context = Graphics.Context;
        return Backend switch
        {
            TestGraphicsBackend.Metal => Map.AttachMetalOwnedTexture(
                new MetalOwnedTextureDescriptor
                {
                    Extent = extent,
                    Context = new MetalContextDescriptor { Device = Borrow(context.MetalDevice) },
                },
                options
            ),
            TestGraphicsBackend.Vulkan => Map.AttachVulkanOwnedTexture(
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
            TestGraphicsBackend.Egl => Map.AttachOpenglOwnedTexture(
                new OpenglOwnedTextureDescriptor
                {
                    Extent = extent,
                    Context = new OpenglContextDescriptor(
                        OpenglContextOwnership.Shared,
                        new OpenglContextDescriptor.DataValue.Egl(
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
            TestGraphicsBackend.Wgl => Map.AttachOpenglOwnedTexture(
                new OpenglOwnedTextureDescriptor
                {
                    Extent = extent,
                    Context = new OpenglContextDescriptor(
                        OpenglContextOwnership.Shared,
                        new OpenglContextDescriptor.DataValue.Wgl(
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

    private static void Complete(Task operation) =>
        Assert.True(operation.Wait(TestWaits.Deadline), "A native operation did not complete.");

    private static T Complete<T>(Task<T> operation)
    {
        Complete((Task)operation);
        return operation.Result;
    }
}
