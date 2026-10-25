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
public readonly partial record struct OpenGlSurfaceDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenGlSurfaceDescriptor(
        LogicalExtent Extent,
        OpenGlContextDescriptor Context,
        NativePointer Surface
    )
    {
        this.Extent = Extent;
        this.Context = Context;
        this.Surface = Surface;
    }

    /// <summary>
    /// Logical surface extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    /// </summary>
    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// Borrowed OpenGL context provider data.
    /// </summary>
    public required OpenGlContextDescriptor Context { get; init; }

    /// <summary>
    /// Borrowed platform surface handle: an HDC for WGL and an EGLSurface for
    /// EGL, both required. Null for WebGL, whose context carries its canvas
    /// binding.
    /// </summary>
    public NativePointer Surface { get; init; }
}
