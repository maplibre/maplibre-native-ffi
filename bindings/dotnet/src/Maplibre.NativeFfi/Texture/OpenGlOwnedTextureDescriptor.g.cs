// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A ring of OpenGL textures that the session creates, for
/// <c>MapHandle.AttachOpenGlOwnedTexture()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_open_gl_owned_texture_descriptor</c> in the <see
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

    public required LogicalExtent Extent { get; init; }
    public required OpenGlContextDescriptor Context { get; init; }
}
