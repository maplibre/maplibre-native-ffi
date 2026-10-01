// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Style;

public readonly record struct StyleImageStretchesResult
{
    public StyleImageStretchesResult(ImageStretch[] StretchX, ImageStretch[] StretchY)
        : this(StretchX, StretchY, false) { }

    internal StyleImageStretchesResult(ImageStretch[] StretchX, ImageStretch[] StretchY, bool adopt)
    {
        this.storageStretchX = adopt ? StretchX : StretchX?.ToArray() ?? [];
        this.storageStretchY = adopt ? StretchY : StretchY?.ToArray() ?? [];
    }

    private readonly ImageStretch[]? storageStretchX;
    public ImageStretch[] StretchX
    {
        get => storageStretchX?.ToArray() ?? [];
        init => storageStretchX = value?.ToArray() ?? [];
    }
    internal ImageStretch[] StretchXStorage
    {
        get => storageStretchX ?? [];
        init => storageStretchX = value;
    }
    private readonly ImageStretch[]? storageStretchY;
    public ImageStretch[] StretchY
    {
        get => storageStretchY?.ToArray() ?? [];
        init => storageStretchY = value?.ToArray() ?? [];
    }
    internal ImageStretch[] StretchYStorage
    {
        get => storageStretchY ?? [];
        init => storageStretchY = value;
    }

    public bool Equals(StyleImageStretchesResult other) =>
        global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            StretchXStorage,
            other.StretchXStorage
        )
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            StretchYStorage,
            other.StretchYStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(StretchXStorage)
        );
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(StretchYStorage)
        );
        return hash.ToHashCode();
    }
}
