// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

public sealed record StyleImageInfo
{
    public uint Width { get; set; }
    public uint Height { get; set; }
    public uint Stride { get; set; }
    public ulong ByteLength { get; set; }
    public ulong StretchXCount { get; set; }
    public ulong StretchYCount { get; set; }
    public ImageContent? Content { get; set; }
    public StyleImageTextFit? TextFitWidth { get; set; }
    public StyleImageTextFit? TextFitHeight { get; set; }
    public float PixelRatio { get; set; }
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
