// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Metal attachment options for a borrowed texture target.
/// </summary>
/// <remarks>
/// See <c>mln_metal_borrowed_texture_descriptor</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Extent">
/// Logical texture extent. The map viewport uses width and height and the
/// renderer uses scale_factor; the physical size is stated separately below.
/// </param>
/// <param name="PhysicalWidth">
/// Physical texture width in device pixels. Must be positive. Defaults to 256.
/// </param>
/// <param name="PhysicalHeight">
/// Physical texture height in device pixels. Must be positive. Defaults to 256.
/// </param>
/// <param name="Texture">
/// Borrowed <c>id&lt;MTLTexture&gt;</c> / <c>MTL::Texture*</c>. Required.
/// </param>
public readonly partial record struct MetalBorrowedTextureDescriptor(
    RenderTargetExtent Extent,
    uint PhysicalWidth,
    uint PhysicalHeight,
    NativePointer Texture
)
{
    public MetalBorrowedTextureDescriptor()
        : this(new RenderTargetExtent(), 256, 256, default) { }

    public static MetalBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(
                null,
                "mln_metal_borrowed_texture_descriptor_default"
            );
            return GeneratedValues.CopyMetalBorrowedTextureDescriptor(
                NativeMethods.mln_metal_borrowed_texture_descriptor_default()
            );
        }
    }
}
