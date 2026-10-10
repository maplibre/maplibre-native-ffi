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
    public ImageStretch[]? StretchX
    {
        get => StretchXStorage?.ToArray();
        set => StretchXStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<ImageStretch>? StretchXStorage { get; set; }
    public ImageStretch[]? StretchY
    {
        get => StretchYStorage?.ToArray();
        set => StretchYStorage = ValueArray.CopyOptional(value);
    }
    internal ValueArray<ImageStretch>? StretchYStorage { get; set; }
    public ImageContent? Content { get; set; }
    public StyleImageTextFit? TextFitWidth { get; set; }
    public StyleImageTextFit? TextFitHeight { get; set; }
    public float? PixelRatio { get; set; }
    public bool? Sdf { get; set; }
    public static StyleImageOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_style_image_options_default");
            return GeneratedValues.CopyStyleImageOptions(
                NativeMethods.mln_style_image_options_default()
            );
        }
    }
}
