// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Render;

public readonly record struct TextureReadbackResult
{
    public TextureReadbackResult(byte[] Data, TextureImageInfo Info)
        : this(Data, Info, false) { }

    internal TextureReadbackResult(byte[] Data, TextureImageInfo Info, bool adopt)
    {
        this.storageData = adopt ? Data : Data?.ToArray() ?? [];
        this.Info = Info;
    }

    private readonly byte[]? storageData;
    public byte[] Data
    {
        get => storageData?.ToArray() ?? [];
        init => storageData = value?.ToArray() ?? [];
    }
    internal byte[] DataStorage
    {
        get => storageData ?? [];
        init => storageData = value;
    }
    public TextureImageInfo Info { get; init; }

    public bool Equals(TextureReadbackResult other) =>
        global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            DataStorage,
            other.DataStorage
        ) && EqualityComparer<TextureImageInfo>.Default.Equals(Info, other.Info);

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(DataStorage));
        hash.Add(Info);
        return hash.ToHashCode();
    }
}
