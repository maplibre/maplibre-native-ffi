using System.Runtime.InteropServices;
using Silk.NET.GLFW;

namespace Maplibre.Native.Examples.DotnetMap;

internal static unsafe partial class GlfwNativeAccess
{
    private const string GlfwLibrary = "glfw";

    static GlfwNativeAccess()
    {
        NativeLibraryResolver.Register();
    }

    [LibraryImport(GlfwLibrary, EntryPoint = "glfwGetEGLDisplay")]
    public static partial nint GetEglDisplay();

    [LibraryImport(GlfwLibrary, EntryPoint = "glfwGetEGLContext")]
    public static partial nint GetEglContext(WindowHandle* window);

    [LibraryImport(GlfwLibrary, EntryPoint = "glfwGetEGLSurface")]
    public static partial nint GetEglSurface(WindowHandle* window);

    [LibraryImport(GlfwLibrary, EntryPoint = "glfwGetWGLContext")]
    public static partial nint GetWglContext(WindowHandle* window);

    [LibraryImport(GlfwLibrary, EntryPoint = "glfwGetWin32Window")]
    public static partial nint GetWin32Window(WindowHandle* window);

    [LibraryImport(GlfwLibrary, EntryPoint = "glfwGetCocoaView")]
    public static partial nint GetCocoaView(WindowHandle* window);

    [LibraryImport(GlfwLibrary, EntryPoint = "glfwSetWindowContentScaleCallback")]
    public static partial nint SetWindowContentScaleCallback(
        WindowHandle* window,
        delegate* unmanaged[Cdecl]<WindowHandle*, float, float, void> callback
    );

    [LibraryImport(GlfwLibrary, EntryPoint = "glfwInitHint")]
    public static partial void InitHint(int hint, int value);

    [LibraryImport(GlfwLibrary, EntryPoint = "glfwGetPlatform")]
    public static partial int GetPlatform();
}
