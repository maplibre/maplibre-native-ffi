// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

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
