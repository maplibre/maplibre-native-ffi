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
public readonly record struct MetalBorrowedTextureDescriptor
{
    public MetalBorrowedTextureDescriptor()
        : this(new LogicalExtent(), 256, 256, default!) { }

    public MetalBorrowedTextureDescriptor(
        LogicalExtent Extent,
        uint PhysicalWidth,
        uint PhysicalHeight,
        MetalBorrowedTexture[] Textures
    )
    {
        this.Extent = Extent;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
    }

    public LogicalExtent Extent { get; init; }
    public uint PhysicalWidth { get; init; }
    public uint PhysicalHeight { get; init; }
    public MetalBorrowedTexture[] Textures
    {
        get => TexturesStorage.ToArray();
        init => TexturesStorage = ValueArray.Copy(value);
    }
    internal ValueArray<MetalBorrowedTexture> TexturesStorage { get; init; }
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
