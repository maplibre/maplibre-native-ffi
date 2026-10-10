// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Inline TileJSON metadata of a tile source.
/// </summary>
/// <remarks>
/// See <c>mln_style_source_tile_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public readonly record struct StyleSourceTileInfo
{
    public StyleSourceTileInfo(
        string[] TileUrls,
        double MinZoom,
        double MaxZoom,
        StyleTileScheme Scheme
    )
    {
        this.TileUrls = TileUrls;
        this.MinZoom = MinZoom;
        this.MaxZoom = MaxZoom;
        this.Scheme = Scheme;
    }

    public string[] TileUrls
    {
        get => TileUrlsStorage.ToArray();
        init => TileUrlsStorage = ValueArray.Copy(value);
    }
    internal ValueArray<string> TileUrlsStorage { get; init; }
    public double MinZoom { get; init; }
    public double MaxZoom { get; init; }
    public StyleTileScheme Scheme { get; init; }
}
