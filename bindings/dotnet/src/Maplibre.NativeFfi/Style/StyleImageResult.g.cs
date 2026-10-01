// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Style;

public readonly record struct StyleImageResult
{
    public StyleImageResult(
        StyleImageInfo Info,
        byte[] Pixels,
        ImageStretch[] StretchX,
        ImageStretch[] StretchY
    )
        : this(Info, Pixels, StretchX, StretchY, false) { }

    internal StyleImageResult(
        StyleImageInfo Info,
        byte[] Pixels,
        ImageStretch[] StretchX,
        ImageStretch[] StretchY,
        bool adopt
    )
    {
        this.Info = Info;
        this.storagePixels = adopt ? Pixels : Pixels?.ToArray() ?? [];
        this.storageStretchX = adopt ? StretchX : StretchX?.ToArray() ?? [];
        this.storageStretchY = adopt ? StretchY : StretchY?.ToArray() ?? [];
    }

    public StyleImageInfo Info { get; init; }
    private readonly byte[]? storagePixels;
    public byte[] Pixels
    {
        get => storagePixels?.ToArray() ?? [];
        init => storagePixels = value?.ToArray() ?? [];
    }
    internal byte[] PixelsStorage
    {
        get => storagePixels ?? [];
        init => storagePixels = value;
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

    public bool Equals(StyleImageResult other) =>
        EqualityComparer<StyleImageInfo>.Default.Equals(Info, other.Info)
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            PixelsStorage,
            other.PixelsStorage
        )
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
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
        hash.Add(Info);
        hash.Add(global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(PixelsStorage));
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(StretchXStorage)
        );
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(StretchYStorage)
        );
        return hash.ToHashCode();
    }
}
