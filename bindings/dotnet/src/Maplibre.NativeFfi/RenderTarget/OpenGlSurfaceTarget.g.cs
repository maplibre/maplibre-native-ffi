// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Your OpenGL surface to present to, with the context that
/// <c>GpuDeviceHandle.WrapOpenGl()</c> wrapped.
/// </summary>
/// <remarks>
/// <para>The session makes its context current on the surface for each frame,
/// and swaps the surface's buffers to present.</para>
/// <para>See <c>mln_open_gl_surface_target</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct OpenGlSurfaceTarget
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenGlSurfaceTarget(LogicalSize LogicalSize, NativePointer Surface)
    {
        this.LogicalSize = LogicalSize;
        this.Surface = Surface;
    }

    public required LogicalSize LogicalSize { get; init; }

    /// <summary>
    /// Your <c>EGLSurface</c> on the context's display, with its config, or
    /// your <c>HDC</c> with the context's pixel format.
    /// </summary>
    public required NativePointer Surface { get; init; }
}
