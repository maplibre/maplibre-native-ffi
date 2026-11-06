// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A source in the map's style.
/// </summary>
/// <remarks>
/// See <c>mln_style_source_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleSourceInfo
{
    public string Id { get; init; } = "";
    public StyleSourceType Type { get; init; }
    public bool IsVolatile { get; init; }
    public string Attribution { get; init; } = "";
    public string Url { get; init; } = "";

    /// <summary>
    /// The tiles that the style lists in the source itself, instead of at a
    /// URL.
    /// </summary>
    public StyleSourceTileInfo? TileJson { get; init; }
    public LatLngBounds? Bounds { get; init; }
    public uint? TileSize { get; init; }
    public StyleVectorTileEncoding? VectorEncoding { get; init; }
    public StyleRasterDemEncoding? RasterEncoding { get; init; }

    /// <summary>
    /// A GeoJSON source's options, with every field set.
    /// </summary>
    public GeoJsonSourceOptions? GeoJson { get; init; }
}
