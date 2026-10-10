// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// One complete runtime style image, borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleImageInfo
{
    public uint Width { get; set; }
    public uint Height { get; set; }
    public byte[] Pixels
    {
        get => PixelsStorage.ToArray();
        set => PixelsStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> PixelsStorage { get; set; }
    public ImageStretch[] StretchX
    {
        get => StretchXStorage.ToArray();
        set => StretchXStorage = ValueArray.Copy(value);
    }
    internal ValueArray<ImageStretch> StretchXStorage { get; set; }
    public ImageStretch[] StretchY
    {
        get => StretchYStorage.ToArray();
        set => StretchYStorage = ValueArray.Copy(value);
    }
    internal ValueArray<ImageStretch> StretchYStorage { get; set; }
    public ImageContent? Content { get; set; }
    public StyleImageTextFit? TextFitWidth { get; set; }
    public StyleImageTextFit? TextFitHeight { get; set; }
    public float PixelRatio { get; set; }
    public bool Sdf { get; set; }
}
