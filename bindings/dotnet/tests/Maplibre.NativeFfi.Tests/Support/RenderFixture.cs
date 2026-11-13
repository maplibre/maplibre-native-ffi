using System.Diagnostics;
using System.Runtime.ExceptionServices;
using Xunit;

namespace Maplibre.NativeFfi.Tests;

/// <summary>
/// A render session for the render tests, driven by the managed thread that creates it.
/// </summary>
/// <remarks>
/// A fixture with a backend renders into a ring of textures from tests/graphics with the caller
/// driver: an OpenGL session shares the fixture's context, which is current only on its graphics
/// thread, and the other backends take the same path so one test covers them all. An offscreen
/// fixture renders on a device that the library creates, with the core worker driver. The
/// session's wakes release a semaphore, and the fixture blocks on it between services.
/// </remarks>
internal sealed class RenderFixture : IDisposable
{
    internal const uint Width = 32;
    internal const uint Height = 16;

    // The number of textures in the ring of a fixture with a backend.
    private const int RingDepth = 2;

    internal const string RedStyle = """
        {"version":8,"sources":{},"layers":[{"id":"background","type":"background","paint":{"background-color":"#ff0000"}}]}
        """;

    private readonly SemaphoreSlim wake = new(0);
    private readonly List<TestGraphicsTexture> textures = [];
    private ulong token;

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

    private RenderFixture(TestGraphicsBackend? backend, Configure configure, QueueLock queueLock)
    {
        Backend = backend;
        try
        {
            if (backend is { } graphics)
            {
                Graphics = TestGraphics.Create(graphics);
                // An OpenGL host drives a shared-context session from the thread its context is
                // current on, the way Kotlin attaches on WGL and EGL.
                if (graphics is TestGraphicsBackend.Egl or TestGraphicsBackend.Wgl)
                    Graphics.MakeCurrent();
                for (var index = 0; index < RingDepth; index++)
                    textures.Add(Graphics.CreateTexture(Width, Height));
            }
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
                    backend is null
                        ? new RenderSessionAttachOptions
                        {
                            Driver = RenderDriverKind.CoreWorker,
                            FrameWake = new Wake(Signal),
                        }
                        : new RenderSessionAttachOptions
                        {
                            Driver = RenderDriverKind.CallerGraphicsThread,
                            FrameWake = new Wake(Signal),
                            DriverWorkWake = new Wake(Signal),
                        }
                ),
                queueLock
            );
        }
        catch
        {
            Dispose();
            throw;
        }
    }

    /// <summary>The backend of the fixture's textures, or null for an offscreen fixture.</summary>
    internal TestGraphicsBackend? Backend { get; }

    internal TestGraphics? Graphics { get; }

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

    internal static void Run(TestGraphicsBackend backend, Action<RenderFixture> test) =>
        Run(backend, static (_, options) => options, test);

    internal static void Run(
        TestGraphicsBackend backend,
        Configure configure,
        Action<RenderFixture> test
    ) => RunOn(backend, configure, default, test);

    /// <summary>
    /// Runs a test on a session whose Vulkan device wraps the test graphics' device with
    /// <paramref name="queueLock" />, which the session alone holds.
    /// </summary>
    internal static void RunWithQueueLock(QueueLock queueLock, Action<RenderFixture> test) =>
        RunOn(TestGraphicsBackend.Vulkan, static (_, options) => options, queueLock, test);

    /// <summary>Runs a test on an offscreen session.</summary>
    internal static void RunOffscreen(Action<RenderFixture> test) =>
        RunOn(null, static (_, options) => options, default, test);

    private static void RunOn(
        TestGraphicsBackend? backend,
        Configure configure,
        QueueLock queueLock,
        Action<RenderFixture> test
    )
    {
        ExceptionDispatchInfo? failure = null;
        var thread = new Thread(() =>
        {
            try
            {
                using var fixture = new RenderFixture(backend, configure, queueLock);
                test(fixture);
            }
            catch (Exception error)
            {
                failure = ExceptionDispatchInfo.Capture(error);
            }
        })
        {
            IsBackground = true,
            Name = $"{backend?.ToString() ?? "Offscreen"} render thread",
        };
        thread.Start();
        thread.Join();
        failure?.Throw();
    }

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
            ServiceDriverWork();
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

    /// <summary>
    /// Releases the session and waits until it retires, servicing it meanwhile, since an attached
    /// caller-driven session detaches at its next service. Returns how many services ran.
    /// </summary>
    internal int ReleaseSession()
    {
        var released = Session.ReleaseAsync();
        released.ContinueWith(
            _ => Signal(),
            CancellationToken.None,
            TaskContinuationOptions.ExecuteSynchronously,
            TaskScheduler.Default
        );
        var elapsed = Stopwatch.StartNew();
        var serviced = 0;
        while (!released.IsCompleted)
        {
            try
            {
                Session.ServiceDriverWork();
                serviced++;
            }
            catch (MaplibreException error) when (error.Status == Status.InvalidState)
            {
                // The session retired, and native reports its handle stale.
            }
            if (!released.IsCompleted)
                TestWaits.Wait(wake, elapsed, "the session to retire");
        }
        released.GetAwaiter().GetResult();
        return serviced;
    }

    internal RenderFrameResult RenderFrame()
    {
        var elapsed = Stopwatch.StartNew();
        while (true)
        {
            var demand = ++token;
            Session.DemandFrame(new FrameDemand { Token = demand });
            RenderFrameResult? result = null;
            while (result is null)
            {
                ServiceDriverWork();
                // Nullable, so a drain without the demand's result leaves it null.
                result = DrainFrames()
                    .Select(frame => (RenderFrameResult?)frame)
                    .LastOrDefault(frame => frame!.Value.Token == demand);
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
            {
                Session.Dispose();
                FinishDisposal();
            }
        }
        finally
        {
            try
            {
                // The map's release completes once its session has retired.
                if (Map is not null)
                    Complete(((IAsyncDisposable)Map).DisposeAsync().AsTask());
            }
            finally
            {
                try
                {
                    if (Runtime is not null)
                        Complete(Runtime.ReleaseAsync());
                }
                finally
                {
                    // Sessions retire before the textures and context they borrowed go away.
                    foreach (var texture in textures)
                        texture.Dispose();
                    Graphics?.Dispose();
                }
            }
        }
    }

    /// <summary>
    /// Services a disposed caller-driven session until the service no longer accepts it, since
    /// the session detaches, and lets go of the textures and its registrations, only in a service.
    /// </summary>
    private void FinishDisposal()
    {
        if (Backend is null)
            return;
        var elapsed = Stopwatch.StartNew();
        while (true)
        {
            try
            {
                Session.ServiceDriverWork();
            }
            catch (MaplibreException error) when (error.Status == Status.InvalidState)
            {
                return;
            }
            // A disposal that finishes without more driver work has no wake to signal, so the
            // wait is short and the loop checks again.
            wake.Wait(TimeSpan.FromMilliseconds(10));
            if (elapsed.Elapsed > TestWaits.Deadline)
                throw new TimeoutException("Timed out waiting for the session to retire.");
        }
    }

    private IReadOnlyList<RenderFrameResult> DrainFrames() => Session.DrainFrameResults();

    // Only a caller-driven session has work for the fixture's thread.
    private void ServiceDriverWork()
    {
        if (Backend is not null)
            Session.ServiceDriverWork();
    }

    /// <summary>
    /// Attaches a session to the fixture's map: on the fixture's textures, or offscreen.
    /// </summary>
    internal RenderSessionHandle Attach(RenderSessionAttachOptions options) =>
        Attach(options, default);

    private RenderSessionHandle Attach(RenderSessionAttachOptions options, QueueLock queueLock)
    {
        var logicalSize = new LogicalSize(Width, Height);
        if (Graphics is null)
            // With no device, the session creates its own.
            return Map.Attach(
                new RenderTarget.Offscreen(new OffscreenTarget(logicalSize)),
                null,
                options
            );
        var infos = textures.Select(texture => texture.Info).ToArray();
        // Every texture of the ring shares the first one's format and size.
        var first = infos[0];
        RenderTarget target = Backend switch
        {
            TestGraphicsBackend.Metal => new RenderTarget.MetalBorrowedTextures(
                new MetalBorrowedTexturesTarget
                {
                    LogicalSize = logicalSize,
                    PhysicalWidth = first.Width,
                    PhysicalHeight = first.Height,
                    Textures =
                    [
                        .. infos.Select(info => new MetalBorrowedTexture(
                            Borrow(info.MetalTexture)
                        )),
                    ],
                }
            ),
            TestGraphicsBackend.Vulkan => new RenderTarget.VulkanBorrowedTextures(
                new VulkanBorrowedTexturesTarget
                {
                    LogicalSize = logicalSize,
                    PhysicalWidth = first.Width,
                    PhysicalHeight = first.Height,
                    Textures =
                    [
                        .. infos.Select(info => new VulkanBorrowedTexture(
                            info.VulkanImage,
                            info.VulkanImageView
                        )),
                    ],
                    Format = first.Format,
                    FinalLayout = first.VulkanFinalLayout,
                }
            ),
            TestGraphicsBackend.Egl or TestGraphicsBackend.Wgl =>
                new RenderTarget.OpenGlBorrowedTextures(
                    new OpenGlBorrowedTexturesTarget
                    {
                        LogicalSize = logicalSize,
                        PhysicalWidth = first.Width,
                        PhysicalHeight = first.Height,
                        Textures =
                        [
                            .. infos.Select(info => new OpenGlBorrowedTexture(info.OpenGlTexture)),
                        ],
                        Target = first.OpenGlTarget,
                    }
                ),
            _ => throw new ArgumentOutOfRangeException(nameof(Backend)),
        };
        // The session keeps the device, and the device its queue lock, until the session ends.
        using var device = WrapDevice(Graphics.Context, queueLock);
        return Map.Attach(target, device, options);
    }

    /// <summary>
    /// Wraps the test graphics' device, or its OpenGL context, in a GPU device. A Vulkan device
    /// takes <paramref name="queueLock" />, the lock on the queue that the test graphics submit
    /// to.
    /// </summary>
    private GpuDeviceHandle WrapDevice(TestGraphicsContext context, QueueLock queueLock) =>
        Backend switch
        {
            TestGraphicsBackend.Metal
                when OperatingSystem.IsMacOS()
                    || OperatingSystem.IsIOS()
                    || OperatingSystem.IsTvOS()
                    || OperatingSystem.IsMacCatalyst() => GpuDeviceHandle.WrapMetal(
                new MetalDeviceDescriptor { Device = Borrow(context.MetalDevice) }
            ),
            TestGraphicsBackend.Vulkan
                when OperatingSystem.IsAndroid()
                    || OperatingSystem.IsLinux()
                    || OperatingSystem.IsMacOS()
                    || OperatingSystem.IsWindows() => GpuDeviceHandle.WrapVulkan(
                new VulkanDeviceDescriptor
                {
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
                    QueueLock = queueLock,
                    EnabledExtensions = context.VulkanDeviceExtensionNames(),
                }
            ),
            TestGraphicsBackend.Egl => GpuDeviceHandle.WrapOpenGl(
                new OpenGlContextDescriptor.Egl(
                    new EglContextDescriptor
                    {
                        Display = Borrow(context.EglDisplay),
                        Config = Borrow(context.EglConfig),
                        ShareContext = Borrow(context.EglContext),
                        GetProcAddress = Borrow(context.GetProcAddress),
                    }
                )
            ),
            TestGraphicsBackend.Wgl => GpuDeviceHandle.WrapOpenGl(
                new OpenGlContextDescriptor.Wgl(
                    new WglContextDescriptor
                    {
                        DeviceContext = Borrow(context.WglDeviceContext),
                        ShareContext = Borrow(context.WglContext),
                        GetProcAddress = Borrow(context.GetProcAddress),
                    }
                )
            ),
            _ => throw new ArgumentOutOfRangeException(nameof(Backend)),
        };

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
