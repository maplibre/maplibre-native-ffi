// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One caller-owned Metal texture of a borrowed texture ring.
/// </summary>
/// <remarks>
/// See <c>mln_metal_borrowed_texture</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct MetalBorrowedTexture
{
    [System.Diagnostics.CodeAnalysis.SetsRequiredMembers]
    public MetalBorrowedTexture(NativePointer Texture)
    {
        this.Texture = Texture;
    }

    /// <summary>
    /// Borrowed <c>id&lt;MTLTexture&gt;</c> / <c>MTL::Texture*</c>. Required.
    /// </summary>
    public required NativePointer Texture { get; init; }
}
