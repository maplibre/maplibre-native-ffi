// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One complete runtime style image.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleImageInfo
{
    /// <summary>
    /// Image width in pixels.
    /// </summary>
    public uint Width { get; init; }

    /// <summary>
    /// Image height in pixels.
    /// </summary>
    public uint Height { get; init; }

    /// <summary>
    /// Premultiplied RGBA8 pixels in tightly packed rows of width * 4 bytes,
    /// top row first.
    /// </summary>
    public ImmutableArray<byte> Pixels
    {
        get => PixelsStorage.AsImmutable();
        init => PixelsStorage = ValueArray.From(value);
    }
    internal ValueArray<byte> PixelsStorage { get; init; }

    /// <summary>
    /// Horizontally stretchable intervals.
    /// </summary>
    public ImmutableArray<ImageStretch> StretchX
    {
        get => StretchXStorage.AsImmutable();
        init => StretchXStorage = ValueArray.From(value);
    }
    internal ValueArray<ImageStretch> StretchXStorage { get; init; }

    /// <summary>
    /// Vertically stretchable intervals.
    /// </summary>
    public ImmutableArray<ImageStretch> StretchY
    {
        get => StretchYStorage.AsImmutable();
        init => StretchYStorage = ValueArray.From(value);
    }
    internal ValueArray<ImageStretch> StretchYStorage { get; init; }

    /// <summary>
    /// Content box, when the image sets one.
    /// </summary>
    public ImageContent? Content { get; init; }

    /// <summary>
    /// How the image fits text horizontally, when it sets this.
    /// </summary>
    public StyleImageTextFit? TextFitWidth { get; init; }

    /// <summary>
    /// How the image fits text vertically, when it sets this.
    /// </summary>
    public StyleImageTextFit? TextFitHeight { get; init; }

    /// <summary>
    /// Sprite pixel ratio.
    /// </summary>
    public float PixelRatio { get; init; }

    /// <summary>
    /// Whether the image is a signed distance field icon.
    /// </summary>
    public bool Sdf { get; init; }
}
