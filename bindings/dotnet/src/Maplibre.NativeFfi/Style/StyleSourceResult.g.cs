// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;

namespace Maplibre.NativeFfi.Style;

public sealed record StyleSourceResult
{
    public required StyleSourceInfo Info { get; set; }
    public string? Attribution { get; set; }
    public string? Url { get; set; }
    private string[]? storageTileUrls;
    public string[]? TileUrls
    {
        get => storageTileUrls?.ToArray();
        set => storageTileUrls = value?.ToArray();
    }
    internal string[]? TileUrlsStorage
    {
        get => storageTileUrls;
        init => storageTileUrls = value;
    }

    public bool Equals(StyleSourceResult? other) =>
        other is not null
        && EqualityComparer<StyleSourceInfo>.Default.Equals(Info, other.Info)
        && EqualityComparer<string?>.Default.Equals(Attribution, other.Attribution)
        && EqualityComparer<string?>.Default.Equals(Url, other.Url)
        && global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceEquals(
            TileUrlsStorage,
            other.TileUrlsStorage
        );

    public override int GetHashCode()
    {
        var hash = new HashCode();
        hash.Add(Info);
        hash.Add(Attribution);
        hash.Add(Url);
        hash.Add(
            global::Maplibre.NativeFfi.Internal.ValueEquality.SequenceHashCode(TileUrlsStorage)
        );
        return hash.ToHashCode();
    }
}
