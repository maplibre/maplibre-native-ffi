// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL attachment options for a native surface.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_surface_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Extent">
/// Logical surface extent.
/// </param>
/// <param name="Context">
/// Borrowed OpenGL context provider data.
/// </param>
/// <param name="Surface">
/// Borrowed platform surface handle: an HDC for WGL and an EGLSurface for EGL,
/// both required. Null for WebGL, whose context carries its canvas binding.
/// </param>
public readonly partial record struct OpenglSurfaceDescriptor(
    RenderTargetExtent Extent,
    OpenglContextDescriptor Context,
    NativePointer Surface
)
{
    public OpenglSurfaceDescriptor()
        : this(new RenderTargetExtent(), default, default) { }

    public static OpenglSurfaceDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_opengl_surface_descriptor_default");
            return GeneratedValues.CopyOpenglSurfaceDescriptor(
                NativeMethods.mln_opengl_surface_descriptor_default()
            );
        }
    }
}
