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
    {
        this = Default;
    }

    public MetalBorrowedTextureDescriptor(
        LogicalExtent Extent,
        uint PhysicalWidth,
        uint PhysicalHeight,
        ImmutableArray<MetalBorrowedTexture> Textures
    )
    {
        this.Extent = Extent;
        this.PhysicalWidth = PhysicalWidth;
        this.PhysicalHeight = PhysicalHeight;
        this.Textures = Textures;
    }

    /// <summary>
    /// Logical texture extent; physical_width and physical_height give the
    /// texture's size. A scale_factor that differs from the map's is accepted
    /// and logged as a warning.
    /// </summary>
    public LogicalExtent Extent { get; init; }

    /// <summary>
    /// Physical texture width in device pixels. Must be positive. Defaults to
    /// 256.
    /// </summary>
    public uint PhysicalWidth { get; init; }

    /// <summary>
    /// Physical texture height in device pixels. Must be positive. Defaults to
    /// 256.
    /// </summary>
    public uint PhysicalHeight { get; init; }

    /// <summary>
    /// The ring's textures, one per slot, in slot order. Required.
    /// </summary>
    public ImmutableArray<MetalBorrowedTexture> Textures
    {
        get => TexturesStorage.AsImmutable();
        init => TexturesStorage = ValueArray.From(value);
    }
    internal ValueArray<MetalBorrowedTexture> TexturesStorage { get; init; }
    public static MetalBorrowedTextureDescriptor Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyMetalBorrowedTextureDescriptor(
                NativeMethods.mln_metal_borrowed_texture_descriptor_default()
            );
        }
    }
}
