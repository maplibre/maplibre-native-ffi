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

    public GlfwWindow Window => window;

    public bool ShouldClose => window.ShouldClose;

    public bool CanRenderFrame => window.CanRenderFrame();

    public bool IsGles => gles;

    public static OpenGLContext Create(string title, int width, int height)
    {
        var providers = Maplibre.SupportedOpenGLContextProviders();
        if (providers.HasFlag(OpenGLContextProvider.Egl))
        {
            return CreateEgl(title, width, height);
        }

        if (providers.HasFlag(OpenGLContextProvider.Wgl))
        {
            return CreateWgl(title, width, height);
        }

        throw new InvalidOperationException(
            "The loaded MapLibre native library does not support an OpenGL context provider usable by dotnet-map."
        );
    }

    public OpenGLContextDescriptor Descriptor(bool requirePbufferConfig)
    {
        if (gles)
        {
            return new EglContextDescriptor
            {
                Display = NativePointer.FromBorrowedAddress(GlfwNativeAccess.GetEglDisplay()),
                Config = NativePointer.FromBorrowedAddress(EglConfig(requirePbufferConfig)),
                ShareContext = NativePointer.FromBorrowedAddress(
                    GlfwNativeAccess.GetEglContext(window.Handle)
                ),
                GetProcAddress = NativeCallbacks.GlfwGetProcAddress,
            };
        }

        return new WglContextDescriptor
        {
            DeviceContext = NativePointer.FromBorrowedAddress(deviceContext),
            ShareContext = NativePointer.FromBorrowedAddress(
                GlfwNativeAccess.GetWglContext(window.Handle)
            ),
            GetProcAddress = NativeCallbacks.GlfwGetProcAddress,
        };
    }

    public NativePointer SurfacePointer() =>
        NativePointer.FromBorrowedAddress(
            gles ? GlfwNativeAccess.GetEglSurface(window.Handle) : deviceContext
        );

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

    public void MakeCurrentForRendering() => MakeCurrent();

    public uint GenTexture() => gles ? glesGl!.GenTexture() : desktopGl!.GenTexture();

    public void DeleteTexture(uint texture)
    {
        if (gles)
        {
            glesGl!.DeleteTexture(texture);
        }
        else
        {
            desktopGl!.DeleteTexture(texture);
        }
    }

    public void BindTexture(uint target, uint texture)
    {
        if (gles)
        {
            glesGl!.BindTexture((Silk.NET.OpenGLES.GLEnum)target, texture);
        }
        else
        {
            desktopGl!.BindTexture((Silk.NET.OpenGL.GLEnum)target, texture);
        }
    }

    public void TexParameter(uint target, uint pname, int value)
    {
        if (gles)
        {
            glesGl!.TexParameter(
                (Silk.NET.OpenGLES.GLEnum)target,
                (Silk.NET.OpenGLES.GLEnum)pname,
                value
            );
        }
        else
        {
            desktopGl!.TexParameter(
                (Silk.NET.OpenGL.GLEnum)target,
                (Silk.NET.OpenGL.GLEnum)pname,
                value
            );
        }
    }

    public void TexImage2D(
        uint target,
        int level,
        int internalFormat,
        uint width,
        uint height,
        int border,
        uint format,
        uint type
    )
    {
        if (gles)
        {
            glesGl!.TexImage2D(
                (Silk.NET.OpenGLES.GLEnum)target,
                level,
                internalFormat,
                width,
                height,
                border,
                (Silk.NET.OpenGLES.GLEnum)format,
                (Silk.NET.OpenGLES.GLEnum)type,
                null
            );
        }
        else
        {
            desktopGl!.TexImage2D(
                (Silk.NET.OpenGL.GLEnum)target,
                level,
                internalFormat,
                width,
                height,
                border,
                (Silk.NET.OpenGL.GLEnum)format,
                (Silk.NET.OpenGL.GLEnum)type,
                null
            );
        }
    }

    public uint CreateShader(uint kind) =>
        gles
            ? glesGl!.CreateShader((Silk.NET.OpenGLES.GLEnum)kind)
            : desktopGl!.CreateShader((Silk.NET.OpenGL.GLEnum)kind);

    public void ShaderSource(uint shader, string source)
    {
        if (gles)
        {
            glesGl!.ShaderSource(shader, source);
        }
        else
        {
            desktopGl!.ShaderSource(shader, source);
        }
    }

    public void CompileShader(uint shader)
    {
        if (gles)
        {
            glesGl!.CompileShader(shader);
        }
        else
        {
            desktopGl!.CompileShader(shader);
        }
    }

    public int GetShader(uint shader, uint pname) =>
        gles
            ? glesGl!.GetShader(shader, (Silk.NET.OpenGLES.GLEnum)pname)
            : desktopGl!.GetShader(shader, (Silk.NET.OpenGL.GLEnum)pname);

    public string GetShaderInfoLog(uint shader) =>
        gles ? glesGl!.GetShaderInfoLog(shader) : desktopGl!.GetShaderInfoLog(shader);

    public void DeleteShader(uint shader)
    {
        if (gles)
        {
            glesGl!.DeleteShader(shader);
        }
        else
        {
            desktopGl!.DeleteShader(shader);
        }
    }

    public uint CreateProgram() => gles ? glesGl!.CreateProgram() : desktopGl!.CreateProgram();

    public void AttachShader(uint program, uint shader)
    {
        if (gles)
        {
            glesGl!.AttachShader(program, shader);
        }
        else
        {
            desktopGl!.AttachShader(program, shader);
        }
    }

    public void DetachShader(uint program, uint shader)
    {
        if (gles)
        {
            glesGl!.DetachShader(program, shader);
        }
        else
        {
            desktopGl!.DetachShader(program, shader);
        }
    }

    public void LinkProgram(uint program)
    {
        if (gles)
        {
            glesGl!.LinkProgram(program);
        }
        else
        {
            desktopGl!.LinkProgram(program);
        }
    }

    public int GetProgram(uint program, uint pname) =>
        gles
            ? glesGl!.GetProgram(program, (Silk.NET.OpenGLES.GLEnum)pname)
            : desktopGl!.GetProgram(program, (Silk.NET.OpenGL.GLEnum)pname);

    public string GetProgramInfoLog(uint program) =>
        gles ? glesGl!.GetProgramInfoLog(program) : desktopGl!.GetProgramInfoLog(program);

    public void DeleteProgram(uint program)
    {
        if (gles)
        {
            glesGl!.DeleteProgram(program);
        }
        else
        {
            desktopGl!.DeleteProgram(program);
        }
    }

    public uint GenVertexArray() => gles ? glesGl!.GenVertexArray() : desktopGl!.GenVertexArray();

    public void BindVertexArray(uint vertexArray)
    {
        if (gles)
        {
            glesGl!.BindVertexArray(vertexArray);
        }
        else
        {
            desktopGl!.BindVertexArray(vertexArray);
        }
    }

    public void DeleteVertexArray(uint vertexArray)
    {
        if (gles)
        {
            glesGl!.DeleteVertexArray(vertexArray);
        }
        else
        {
            desktopGl!.DeleteVertexArray(vertexArray);
        }
    }

    public void UseProgram(uint program)
    {
        if (gles)
        {
            glesGl!.UseProgram(program);
        }
        else
        {
            desktopGl!.UseProgram(program);
        }
    }

    public int GetUniformLocation(uint program, string name) =>
        gles
            ? glesGl!.GetUniformLocation(program, name)
            : desktopGl!.GetUniformLocation(program, name);

    public void Uniform1(int location, int value)
    {
        if (gles)
        {
            glesGl!.Uniform1(location, value);
        }
        else
        {
            desktopGl!.Uniform1(location, value);
        }
    }

    public void BindFramebuffer(uint target, uint framebuffer)
    {
        if (gles)
        {
            glesGl!.BindFramebuffer((Silk.NET.OpenGLES.GLEnum)target, framebuffer);
        }
        else
        {
            desktopGl!.BindFramebuffer((Silk.NET.OpenGL.GLEnum)target, framebuffer);
        }
    }

    public void Disable(uint capability)
    {
        if (gles)
        {
            glesGl!.Disable((Silk.NET.OpenGLES.GLEnum)capability);
        }
        else
        {
            desktopGl!.Disable((Silk.NET.OpenGL.GLEnum)capability);
        }
    }

    public void Viewport(int x, int y, uint width, uint height)
    {
        if (gles)
        {
            glesGl!.Viewport(x, y, width, height);
        }
        else
        {
            desktopGl!.Viewport(x, y, width, height);
        }
    }

    public void ClearColor(float red, float green, float blue, float alpha)
    {
        if (gles)
        {
            glesGl!.ClearColor(red, green, blue, alpha);
        }
        else
        {
            desktopGl!.ClearColor(red, green, blue, alpha);
        }
    }

    public void Clear(uint mask)
    {
        if (gles)
        {
            glesGl!.Clear(mask);
        }
        else
        {
            desktopGl!.Clear(mask);
        }
    }

    public void ActiveTexture(uint texture)
    {
        if (gles)
        {
            glesGl!.ActiveTexture((Silk.NET.OpenGLES.GLEnum)texture);
        }
        else
        {
            desktopGl!.ActiveTexture((Silk.NET.OpenGL.GLEnum)texture);
        }
    }

    public void DrawArrays(uint mode, int first, uint count)
    {
        if (gles)
        {
            glesGl!.DrawArrays((Silk.NET.OpenGLES.GLEnum)mode, first, count);
        }
        else
        {
            desktopGl!.DrawArrays((Silk.NET.OpenGL.GLEnum)mode, first, count);
        }
    }

    public uint GetError() => gles ? (uint)glesGl!.GetError() : (uint)desktopGl!.GetError();

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
            var display = GlfwNativeAccess.GetEglDisplay();
            var eglContext = GlfwNativeAccess.GetEglContext(window.Handle);
            var surface = GlfwNativeAccess.GetEglSurface(window.Handle);
            if (display == 0 || eglContext == 0 || surface == 0)
            {
                throw new InvalidOperationException("GLFW did not expose EGL handles.");
            }

            _ = EglNative.GetSurfaceConfig(display, surface);
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
            if (hwnd == 0 || hglrc == 0)
            {
                throw new InvalidOperationException("GLFW did not expose WGL handles.");
            }

            var hdc = WindowsNative.GetDeviceContext(hwnd);
            if (hdc == 0)
            {
                throw new InvalidOperationException("Failed to acquire a WGL device context.");
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

    private nint EglConfig(bool requirePbufferConfig)
    {
        var display = GlfwNativeAccess.GetEglDisplay();
        var surface = GlfwNativeAccess.GetEglSurface(window.Handle);
        return requirePbufferConfig
            ? EglNative.GetTextureConfig(display, surface)
            : EglNative.GetSurfaceConfig(display, surface);
    }
}
