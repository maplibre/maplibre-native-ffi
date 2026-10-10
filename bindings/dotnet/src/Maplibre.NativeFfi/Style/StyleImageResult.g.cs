// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Complete style image borrowed for a completion callback.
/// </summary>
/// <remarks>
/// See <c>mln_style_image_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct StyleImageResult
{
    public StyleImageResult()
        : this(new StyleImageInfo(), default!, default!, default!) { }

    public StyleImageResult(
        StyleImageInfo Info,
        byte[] Pixels,
        ImageStretch[] StretchX,
        ImageStretch[] StretchY
    )
    {
        this.Info = Info;
        this.Pixels = Pixels;
        this.StretchX = StretchX;
        this.StretchY = StretchY;
    }

    public StyleImageInfo Info { get; init; }
    public byte[] Pixels
    {
        get => PixelsStorage.ToArray();
        init => PixelsStorage = ValueArray.Copy(value);
    }
    internal ValueArray<byte> PixelsStorage { get; init; }
    public ImageStretch[] StretchX
    {
        get => StretchXStorage.ToArray();
        init => StretchXStorage = ValueArray.Copy(value);
    }
    internal ValueArray<ImageStretch> StretchXStorage { get; init; }
    public ImageStretch[] StretchY
    {
        get => StretchYStorage.ToArray();
        init => StretchYStorage = ValueArray.Copy(value);
    }
    internal ValueArray<ImageStretch> StretchYStorage { get; init; }
}
