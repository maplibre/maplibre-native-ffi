// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for a style image, as the
/// [sprite](https://maplibre.org/maplibre-style-spec/sprite/) spec describes
/// them.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleImageOptions
{
    /// <summary>
    /// The ranges of columns that stretch, in increasing order without overlap.
    /// </summary>
    public ImmutableArray<ImageStretch>? StretchX
    {
        get => StretchXStorage?.AsImmutable();
        init => StretchXStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<ImageStretch>? StretchXStorage { get; init; }

    /// <summary>
    /// The ranges of rows that stretch, in increasing order without overlap.
    /// </summary>
    public ImmutableArray<ImageStretch>? StretchY
    {
        get => StretchYStorage?.AsImmutable();
        init => StretchYStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<ImageStretch>? StretchYStorage { get; init; }
    public ImageContent? Content { get; init; }
    public StyleImageTextFit? TextFitWidth { get; init; }
    public StyleImageTextFit? TextFitHeight { get; init; }
    public float? PixelRatio { get; init; }
    public bool? Sdf { get; init; }

    /// <summary>
    /// Keeps the image after no layer uses it. Without this, the map can remove
    /// an image that it asked for through
    /// <c>RuntimeEventType.MapStyleImageMissing</c>.
    /// </summary>
    public bool? KeepUnused { get; init; }
}
