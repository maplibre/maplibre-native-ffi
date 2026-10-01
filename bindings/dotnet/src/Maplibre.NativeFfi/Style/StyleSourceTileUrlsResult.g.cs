// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Style;

public readonly record struct StyleSourceTileUrlsResult
{
    public StyleSourceTileUrlsResult(string[] TileUrls)
        : this(TileUrls, false) { }

    internal StyleSourceTileUrlsResult(string[] TileUrls, bool adopt)
    {
        this.storageTileUrls = adopt ? TileUrls : TileUrls?.ToArray() ?? [];
    }

    private readonly string[]? storageTileUrls;
    public string[] TileUrls
    {
        get => storageTileUrls?.ToArray() ?? [];
        init => storageTileUrls = value?.ToArray() ?? [];
    }
    internal string[] TileUrlsStorage
    {
        get => storageTileUrls ?? [];
        init => storageTileUrls = value;
    }

    public bool Equals(StyleSourceTileUrlsResult other) =>
        global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            TileUrlsStorage,
            other.TileUrlsStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(TileUrlsStorage)
        );
        return hash.ToHashCode();
    }
}
