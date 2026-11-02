// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// An image in the map's style.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleImageInfo
{
    public uint Width { get; init; }
    public uint Height { get; init; }

    /// <summary>
    /// The premultiplied RGBA pixels, in rows of <c>width</c> * 4 bytes, from
    /// the top row down.
    /// </summary>
    public ImmutableArray<byte> Pixels
    {
        get => PixelsStorage.AsImmutable();
        init => PixelsStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> PixelsStorage { get; init; }
    public ImmutableArray<ImageStretch> StretchX
    {
        get => StretchXStorage.AsImmutable();
        init => StretchXStorage = ValueArray.From(value);
    }
    internal ValueArray<ImageStretch> StretchXStorage { get; init; }
    public ImmutableArray<ImageStretch> StretchY
    {
        get => StretchYStorage.AsImmutable();
        init => StretchYStorage = ValueArray.From(value);
    }
    internal ValueArray<ImageStretch> StretchYStorage { get; init; }
    public ImageContent? Content { get; init; }
    public StyleImageTextFit? TextFitWidth { get; init; }
    public StyleImageTextFit? TextFitHeight { get; init; }
    public float PixelRatio { get; init; }
    public bool Sdf { get; init; }
    public bool KeepUnused { get; init; }
}
