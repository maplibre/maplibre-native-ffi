using Silk.NET.GLFW;

namespace Maplibre.NativeFfi.Examples.DotnetMap;

internal sealed unsafe class GlfwWindow : IDisposable
{
    private const int GlfwPlatform = 0x00050003;
    private const int GlfwPlatformNull = 0x00060005;
    private static readonly Dictionary<nint, GlfwWindow> Windows = new();
    private readonly GlfwCallbacks.WindowSizeCallback windowSizeCallback;
    private readonly GlfwCallbacks.FramebufferSizeCallback framebufferSizeCallback;
    private float contentScaleX;
    private float contentScaleY;
    private bool closed;

    static GlfwWindow()
    {
        NativeLibraryResolver.PreloadGlfw();
    }

    private GlfwWindow(Glfw glfw, WindowHandle* handle)
    {
        Glfw = glfw;
        Handle = handle;
        windowSizeCallback = OnWindowSize;
        framebufferSizeCallback = OnFramebufferSize;
    }

    public Glfw Glfw { get; }

    public WindowHandle* Handle { get; private set; }

    public nint NativeHandle => (nint)Handle;

    public bool ShouldClose => Glfw.WindowShouldClose(Handle);

    public Viewport CurrentViewport { get; private set; }

    public event Action<Viewport>? ViewportChanged;

    /// <summary>Opens a window, hidden when <paramref name="visible" /> is false.</summary>
    /// <remarks>
    /// A hidden window on a Linux host with no display uses GLFW's null platform, which creates
    /// surfaceless EGL contexts and headless Vulkan surfaces, so the smoke run needs no display
    /// server.
    /// </remarks>
    public static GlfwWindow Create(
        string title,
        int width,
        int height,
        bool visible,
        Action<Glfw> configureHints
    )
    {
        var glfw = Glfw.GetApi();
        if (!visible && OperatingSystem.IsLinux() && !HasDisplay())
        {
            GlfwNativeAccess.InitHint(GlfwPlatform, GlfwPlatformNull);
        }

        if (!glfw.Init())
        {
            throw new InvalidOperationException("GLFW initialization failed.");
        }

        try
        {
            glfw.DefaultWindowHints();
            glfw.WindowHint(WindowHintBool.Resizable, true);
            glfw.WindowHint(WindowHintBool.Visible, visible);
            configureHints(glfw);

            var handle = glfw.CreateWindow(width, height, title, null, null);
            if (handle is null)
            {
                glfw.GetError(out byte* description);
                var reason = description is null
                    ? "no reason reported"
                    : System.Runtime.InteropServices.Marshal.PtrToStringUTF8((nint)description);
                throw new InvalidOperationException($"GLFW window creation failed: {reason}");
            }

            var window = new GlfwWindow(glfw, handle);
            window.RegisterViewportCallbacks();
            window.RecomputeViewport("initial viewport", forceLog: true);
            return window;
        }
        catch
        {
            glfw.Terminate();
            throw;
        }
    }

    public Viewport ReadViewport() => CurrentViewport;

    public void PollEvents()
    {
        Glfw.PollEvents();
    }

    public void WaitEventsTimeout(double timeoutSeconds)
    {
        Glfw.WaitEventsTimeout(timeoutSeconds);
    }

    public bool CanRenderFrame()
    {
        return !CurrentViewport.IsEmpty;
    }

    public void Dispose()
    {
        if (closed)
        {
            return;
        }

        closed = true;
        if (Handle is not null)
        {
            Windows.Remove((nint)Handle);
            Glfw.SetWindowSizeCallback(Handle, null);
            Glfw.SetFramebufferSizeCallback(Handle, null);
            GlfwNativeAccess.SetWindowContentScaleCallback(Handle, null);
            Glfw.DestroyWindow(Handle);
            Handle = null;
        }
        Glfw.Terminate();
    }

    private static void OnWindowSize(WindowHandle* handle, int width, int height)
    {
        _ = width;
        _ = height;
        DispatchViewportChange(handle);
    }

    private static void OnFramebufferSize(WindowHandle* handle, int width, int height)
    {
        _ = width;
        _ = height;
        DispatchViewportChange(handle);
    }

    [System.Runtime.InteropServices.UnmanagedCallersOnly(
        CallConvs = [typeof(System.Runtime.CompilerServices.CallConvCdecl)]
    )]
    private static void OnContentScale(WindowHandle* handle, float scaleX, float scaleY)
    {
        if (!Windows.TryGetValue((nint)handle, out var window))
        {
            return;
        }

        window.contentScaleX = scaleX;
        window.contentScaleY = scaleY;
        window.RecomputeViewport("viewport changed", forceLog: false);
    }

    private static void DispatchViewportChange(WindowHandle* handle)
    {
        if (Windows.TryGetValue((nint)handle, out var window))
        {
            window.RecomputeViewport("viewport changed", forceLog: false);
        }
    }

    private void RegisterViewportCallbacks()
    {
        Windows[(nint)Handle] = this;
        Glfw.SetWindowSizeCallback(Handle, windowSizeCallback);
        Glfw.SetFramebufferSizeCallback(Handle, framebufferSizeCallback);
        GlfwNativeAccess.SetWindowContentScaleCallback(Handle, &OnContentScale);
    }

    private void RecomputeViewport(string label, bool forceLog)
    {
        Glfw.GetWindowSize(Handle, out var logicalWidth, out var logicalHeight);
        Glfw.GetFramebufferSize(Handle, out var physicalWidth, out var physicalHeight);
        var scaleX = ViewportScale(logicalWidth, physicalWidth, contentScaleX);
        var scaleY = ViewportScale(logicalHeight, physicalHeight, contentScaleY);
        var viewport = Viewport.FromWindowMetrics(
            logicalWidth,
            logicalHeight,
            physicalWidth,
            physicalHeight,
            scaleX,
            scaleY
        );

        if (!forceLog && viewport == CurrentViewport)
        {
            return;
        }

        CurrentViewport = viewport;
        CurrentViewport.Log(label);
        ViewportChanged?.Invoke(CurrentViewport);
    }

    private static bool HasDisplay() =>
        !string.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable("DISPLAY"))
        || !string.IsNullOrWhiteSpace(Environment.GetEnvironmentVariable("WAYLAND_DISPLAY"));

    private static float ViewportScale(int logicalSize, int physicalSize, float fallbackScale)
    {
        if (logicalSize > 0 && physicalSize > 0)
        {
            return (float)physicalSize / logicalSize;
        }

        return float.IsFinite(fallbackScale) && fallbackScale > 0 ? fallbackScale : 1;
    }
}
