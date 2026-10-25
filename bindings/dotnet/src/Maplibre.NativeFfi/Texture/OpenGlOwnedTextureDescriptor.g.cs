// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// OpenGL attachment options for an owned texture target.
/// </summary>
/// <remarks>
/// See <c>mln_opengl_owned_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct OpenGlOwnedTextureDescriptor
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public OpenGlOwnedTextureDescriptor(LogicalExtent Extent, OpenGlContextDescriptor Context)
    {
        this.Extent = Extent;
        this.Context = Context;
    }

    /// <summary>
    /// Logical texture extent. Required. A scale_factor that differs from the
    /// map's is accepted and logged as a warning.
    /// </summary>
    public required LogicalExtent Extent { get; init; }

    /// <summary>
    /// Borrowed OpenGL context provider data. Shared ownership creates a
    /// context whose texture frames the host can acquire. Dedicated EGL, WGL,
    /// or transferred WebGL ownership creates a private core-worker context for
    /// CPU readback.
    /// </summary>
    public required OpenGlContextDescriptor Context { get; init; }
}
