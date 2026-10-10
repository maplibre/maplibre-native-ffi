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
    /// Content box, meaningful when fields contains CONTENT.
    /// </summary>
    public ImageContent? Content { get; set; }

    /// <summary>
    /// One of <c>mln_style_image_text_fit</c>, meaningful when fields contains
    /// TEXT_FIT_WIDTH.
    /// </summary>
    public StyleImageTextFit? TextFitWidth { get; set; }

    /// <summary>
    /// One of <c>mln_style_image_text_fit</c>, meaningful when fields contains
    /// TEXT_FIT_HEIGHT.
    /// </summary>
    public StyleImageTextFit? TextFitHeight { get; set; }

    /// <summary>
    /// Sprite pixel ratio.
    /// </summary>
    public float PixelRatio { get; set; }
    public bool Sdf { get; set; }
}
