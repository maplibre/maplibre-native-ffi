using Silk.NET.GLFW;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed unsafe class GlfwWindow : IDisposable
{
    private bool closed;

    private GlfwWindow(Glfw glfw, WindowHandle* handle)
    {
        Glfw = glfw;
        Handle = handle;
    }

    public Glfw Glfw { get; }

    public WindowHandle* Handle { get; private set; }

    public nint NativeHandle => (nint)Handle;

    public bool ShouldClose => Glfw.WindowShouldClose(Handle);

    public static GlfwWindow Create(
        string title,
        int width,
        int height,
        Action<Glfw> configureHints
    )
    {
        var glfw = Glfw.GetApi();
        if (!glfw.Init())
        {
            throw new InvalidOperationException("GLFW initialization failed.");
        }

        try
        {
            glfw.DefaultWindowHints();
            glfw.WindowHint(WindowHintBool.Resizable, true);
            configureHints(glfw);

            var handle = glfw.CreateWindow(width, height, title, null, null);
            if (handle is null)
            {
                throw new InvalidOperationException("GLFW window creation failed.");
            }

            return new GlfwWindow(glfw, handle);
        }
        catch
        {
            glfw.Terminate();
            throw;
        }
    }

    public Viewport ReadViewport()
    {
        Glfw.GetWindowSize(Handle, out var logicalWidth, out var logicalHeight);
        Glfw.GetFramebufferSize(Handle, out var physicalWidth, out var physicalHeight);
        var scaleX = logicalWidth > 0 ? (float)physicalWidth / logicalWidth : 1;
        var scaleY = logicalHeight > 0 ? (float)physicalHeight / logicalHeight : 1;
        return Viewport.FromWindowMetrics(
            logicalWidth,
            logicalHeight,
            physicalWidth,
            physicalHeight,
            scaleX,
            scaleY
        );
    }

    public void PollEvents()
    {
        Glfw.PollEvents();
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
            Glfw.DestroyWindow(Handle);
            Handle = null;
        }
        Glfw.Terminate();
    }
}
