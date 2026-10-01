// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Style;

public readonly record struct StyleImageStretchesResult
{
    public StyleImageStretchesResult(ImageStretch[] StretchX, ImageStretch[] StretchY)
    {
        this.StretchX = StretchX;
        this.StretchY = StretchY;
    }

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
