using Maplibre.Native;
using Maplibre.Native.Render;
using Silk.NET.GLFW;
using DesktopGL = Silk.NET.OpenGL.GL;
using Gles = Silk.NET.OpenGLES.GL;

namespace Maplibre.Native.Examples.DotnetMap;

internal sealed unsafe class OpenGLContext : IGraphicsContext
{
    private readonly bool gles;
    private readonly GlfwWindow window;
    private readonly DesktopGL? desktopGl;
    private readonly Gles? glesGl;
    private nint deviceContext;
    private bool closed;

    private OpenGLContext(GlfwWindow window, bool gles, DesktopGL? desktopGl, Gles? glesGl)
    {
        this.window = window;
        this.gles = gles;
        this.desktopGl = desktopGl;
        this.glesGl = glesGl;
    }

    public RenderBackend Backend => RenderBackend.OpenGL;

    public nint WindowHandle => window.NativeHandle;

    public bool ShouldClose => window.ShouldClose;

    public static OpenGLContext Create(string title, int width, int height)
    {
        if (OperatingSystem.IsLinux())
        {
            return CreateEgl(title, width, height);
        }

        if (OperatingSystem.IsWindows())
        {
            return CreateWgl(title, width, height);
        }

        throw new InvalidOperationException(
            "dotnet-map OpenGL context creation is supported on Linux/EGL and Windows/WGL."
        );
    }

    public OpenGLContextDescriptor Descriptor()
    {
        if (gles)
        {
            return new EglContextDescriptor
            {
                Display = new NativePointer(GlfwNativeAccess.GetEglDisplay()),
                Config = new NativePointer(EglConfig()),
                ShareContext = new NativePointer(GlfwNativeAccess.GetEglContext(window.Handle)),
                GetProcAddress = NativeCallbacks.GlfwGetProcAddress,
            };
        }

        return new WglContextDescriptor
        {
            DeviceContext = new NativePointer(deviceContext),
            ShareContext = new NativePointer(GlfwNativeAccess.GetWglContext(window.Handle)),
            GetProcAddress = NativeCallbacks.GlfwGetProcAddress,
        };
    }

    public NativePointer SurfacePointer() =>
        new(gles ? GlfwNativeAccess.GetEglSurface(window.Handle) : deviceContext);

    public Viewport ReadViewport() => window.ReadViewport();

    public void Resize(Viewport viewport)
    {
        _ = viewport;
        MakeCurrent();
    }

    public void PollEvents() => window.PollEvents();

    public void FinishFrame()
    {
        MakeCurrent();
        window.Glfw.SwapBuffers(window.Handle);
    }

    public void Dispose()
    {
        if (closed)
        {
            return;
        }

        closed = true;
        if (window.Handle is not null)
        {
            window.Glfw.MakeContextCurrent(window.Handle);
            if (gles)
            {
                glesGl?.Finish();
            }
            else
            {
                desktopGl?.Finish();
            }

            window.Glfw.MakeContextCurrent(null);
            if (OperatingSystem.IsWindows() && deviceContext != 0)
            {
                WindowsNative.ReleaseDeviceContextOrThrow(
                    GlfwNativeAccess.GetWin32Window(window.Handle),
                    deviceContext
                );
                deviceContext = 0;
            }
        }

        (glesGl as IDisposable)?.Dispose();
        (desktopGl as IDisposable)?.Dispose();
        window.Dispose();
    }

    private static OpenGLContext CreateEgl(string title, int width, int height)
    {
        if (!Maplibre.SupportedOpenGLContextProviders().HasFlag(OpenGLContextProvider.Egl))
        {
            throw new InvalidOperationException("Native library does not support EGL.");
        }

        OpenGLContext? context = null;
        var window = GlfwWindow.Create(
            title,
            width,
            height,
            glfw =>
            {
                glfw.WindowHint(WindowHintClientApi.ClientApi, ClientApi.OpenGLES);
                glfw.WindowHint(WindowHintContextApi.ContextCreationApi, ContextApi.EglContextApi);
                glfw.WindowHint(WindowHintInt.ContextVersionMajor, 3);
                glfw.WindowHint(WindowHintInt.ContextVersionMinor, 0);
            }
        );

        try
        {
            window.Glfw.MakeContextCurrent(window.Handle);
            var gl = Gles.GetApi(window.Glfw.GetProcAddress);
            context = new OpenGLContext(window, true, null, gl);
            _ = context.EglConfig();
            if (
                GlfwNativeAccess.GetEglDisplay() == 0
                || GlfwNativeAccess.GetEglContext(window.Handle) == 0
                || GlfwNativeAccess.GetEglSurface(window.Handle) == 0
            )
            {
                throw new InvalidOperationException("GLFW did not expose EGL handles.");
            }

            Console.WriteLine($"GLFW {window.Glfw.GetVersionString()}, OpenGL EGL/GLES");
            return context;
        }
        catch
        {
            context?.Dispose();
            if (context is null)
            {
                window.Dispose();
            }
            throw;
        }
    }

    private static OpenGLContext CreateWgl(string title, int width, int height)
    {
        if (!Maplibre.SupportedOpenGLContextProviders().HasFlag(OpenGLContextProvider.Wgl))
        {
            throw new InvalidOperationException("Native library does not support WGL.");
        }

        OpenGLContext? context = null;
        var window = GlfwWindow.Create(
            title,
            width,
            height,
            glfw =>
            {
                glfw.WindowHint(WindowHintClientApi.ClientApi, ClientApi.OpenGL);
                glfw.WindowHint(WindowHintInt.ContextVersionMajor, 3);
                glfw.WindowHint(WindowHintInt.ContextVersionMinor, 0);
            }
        );

        try
        {
            window.Glfw.MakeContextCurrent(window.Handle);
            var gl = DesktopGL.GetApi(window.Glfw.GetProcAddress);
            var hwnd = GlfwNativeAccess.GetWin32Window(window.Handle);
            var hglrc = GlfwNativeAccess.GetWglContext(window.Handle);
            var hdc = WindowsNative.GetDeviceContext(hwnd);
            if (hwnd == 0 || hglrc == 0 || hdc == 0)
            {
                throw new InvalidOperationException("GLFW did not expose WGL handles.");
            }

            context = new OpenGLContext(window, false, gl, null) { deviceContext = hdc };
            Console.WriteLine($"GLFW {window.Glfw.GetVersionString()}, OpenGL WGL");
            return context;
        }
        catch
        {
            context?.Dispose();
            if (context is null)
            {
                window.Dispose();
            }
            throw;
        }
    }

    private void MakeCurrent()
    {
        ObjectDisposedException.ThrowIf(closed, this);
        window.Glfw.MakeContextCurrent(window.Handle);
    }

    private nint EglConfig()
    {
        if (!GlfwNativeAccess.GetEglConfig(window.Handle, out var config) || config == 0)
        {
            throw new InvalidOperationException("GLFW did not expose an EGL config.");
        }

        return config;
    }
}
