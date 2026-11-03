// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An OpenGL surface to present to, for <c>MapHandle.AttachOpenGlSurface()</c>.
/// </summary>
/// <remarks>
/// <para>The session uses the surface until a replacement or release of the
/// target completes, a detach or release of the session completes, or an
/// abandon of it returns.</para>
/// <para>See <c>mln_open_gl_surface_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct OpenGlSurfaceDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenGlSurfaceDescriptor(
        LogicalSize LogicalSize,
        OpenGlContextDescriptor Context,
        NativePointer Surface
    )
    {
        this.LogicalSize = LogicalSize;
        this.Context = Context;
        this.Surface = Surface;
    }

    public required LogicalSize LogicalSize { get; init; }
    public required OpenGlContextDescriptor Context { get; init; }

    /// <summary>
    /// Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it.
    /// </summary>
    public NativePointer Surface { get; init; }
}
