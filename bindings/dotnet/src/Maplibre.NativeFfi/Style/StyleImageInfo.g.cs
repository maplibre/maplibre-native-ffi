// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Fixed metadata for one runtime style image.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleImageInfo
{
    public uint Width { get; set; }
    public uint Height { get; set; }

    /// <summary>
    /// Native copied images are exposed as tightly packed premultiplied RGBA8.
    /// </summary>
    public uint Stride { get; set; }
    public ulong ByteLength { get; set; }

    /// <summary>
    /// Interval counts for the stretchable axes.
    /// </summary>
    public ulong StretchXCount { get; set; }
    public ulong StretchYCount { get; set; }

    /// <summary>
    /// Content box, meaningful only when has_content is true.
    /// </summary>
    public ImageContent? Content { get; set; }

    /// <summary>
    /// One of <c>mln_style_image_text_fit</c>, meaningful only when its flag is
    /// true.
    /// </summary>
    public StyleImageTextFit? TextFitWidth { get; set; }

    /// <summary>
    /// One of <c>mln_style_image_text_fit</c>, meaningful only when its flag is
    /// true.
    /// </summary>
    public StyleImageTextFit? TextFitHeight { get; set; }

    /// <summary>
    /// Sprite pixel ratio. Defaults to 1.0.
    /// </summary>
    public float PixelRatio { get; set; } = 1.0f;
    public bool Sdf { get; set; }
    public static StyleImageInfo Default
    {
        get
        {
            using var call = NativeCall.Enter(null, "mln_style_image_info_default");
            return GeneratedValues.CopyStyleImageInfo(NativeMethods.mln_style_image_info_default());
        }
    }
}
