// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A new OpenGL surface for
/// <c>RenderSessionHandle.SetOpenGlSurfaceTargetAsync()</c>.
/// </summary>
/// <remarks>
/// <para>The session presents to it with the context that it attached with. It
/// uses the surface until a replacement or release of the target completes, a
/// detach or release of the session completes, or an abandon of it
/// returns.</para>
/// <para>See <c>mln_open_gl_surface_replacement</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct OpenGlSurfaceReplacement
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenGlSurfaceReplacement(LogicalSize LogicalSize, NativePointer Surface)
    {
        this.LogicalSize = LogicalSize;
        this.Surface = Surface;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your EGLSurface with EGL, or your HDC with WGL. WebGL doesn't use it, so
    /// leave it null.
    /// </summary>
    public NativePointer Surface { get; init; }
}
