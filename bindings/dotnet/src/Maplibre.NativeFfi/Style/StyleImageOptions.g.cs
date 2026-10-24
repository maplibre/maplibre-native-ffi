// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Options for runtime style images.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleImageOptions
{
    /// <summary>
    /// Horizontally stretchable intervals. Borrowed for the call and copied
    /// before return. May be null only when stretch_x_count is 0.
    /// </summary>
    public ImmutableArray<ImageStretch>? StretchX
    {
        get => StretchXStorage?.AsImmutable();
        init => StretchXStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<ImageStretch>? StretchXStorage { get; init; }

    /// <summary>
    /// Vertically stretchable intervals. Borrowed for the call and copied
    /// before return. May be null only when stretch_y_count is 0.
    /// </summary>
    public ImmutableArray<ImageStretch>? StretchY
    {
        get => StretchYStorage?.AsImmutable();
        init => StretchYStorage = ValueArray.FromOptional(value);
    }
    internal ValueArray<ImageStretch>? StretchYStorage { get; init; }

    /// <summary>
    /// Content box used when icon-text-fit applies.
    /// </summary>
    public ImageContent? Content { get; init; }

    /// <summary>
    /// One of <c>mln_style_image_text_fit</c>. Defaults to STRETCH_OR_SHRINK.
    /// </summary>
    public StyleImageTextFit? TextFitWidth { get; init; }

    /// <summary>
    /// One of <c>mln_style_image_text_fit</c>. Defaults to STRETCH_OR_SHRINK.
    /// </summary>
    public StyleImageTextFit? TextFitHeight { get; init; }

    /// <summary>
    /// Sprite pixel ratio. Defaults to 1.
    /// </summary>
    public float? PixelRatio { get; init; }

    /// <summary>
    /// Whether the image is a signed distance field icon. Defaults to false.
    /// </summary>
    public bool? Sdf { get; init; }
    public static StyleImageOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyStyleImageOptions(
                NativeMethods.mln_style_image_options_default()
            );
        }
    }
}
