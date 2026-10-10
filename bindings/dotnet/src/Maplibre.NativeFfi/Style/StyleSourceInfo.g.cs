// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Fixed source metadata included in <c>mln_style_source_result</c>.
/// </summary>
/// <remarks>
/// See <c>mln_style_source_info</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html">C API reference</see>.
/// </remarks>
public sealed record StyleSourceInfo
{
    /// <summary>
    /// One of <c>mln_style_source_type</c>.
    /// </summary>
    public StyleSourceType Type { get; set; }

    /// <summary>
    /// Source ID byte length, excluding any null terminator.
    /// </summary>
    public ulong IdSize { get; set; }

    /// <summary>
    /// Whether the source is marked volatile.
    /// </summary>
    public bool IsVolatile { get; set; }

    /// <summary>
    /// Attribution byte length, excluding any null terminator, meaningful when
    /// fields contains ATTRIBUTION.
    /// </summary>
    public ulong? AttributionSize { get; set; }

    /// <summary>
    /// URL byte length, meaningful when fields contains URL.
    /// </summary>
    public ulong? UrlSize { get; set; }
    public StyleSourceTileInfo? Tilejson { get; set; }

    /// <summary>
    /// Geographic bounds, meaningful when fields contains BOUNDS.
    /// </summary>
    public LatLngBounds? Bounds { get; set; }

    /// <summary>
    /// Tile size in pixels, meaningful when fields contains TILE_SIZE.
    /// </summary>
    public uint? TileSize { get; set; }

    /// <summary>
    /// Vector encoding, meaningful when fields contains VECTOR_ENCODING.
    /// </summary>
    public StyleVectorTileEncoding? VectorEncoding { get; set; }

    /// <summary>
    /// DEM encoding, meaningful when fields contains RASTER_ENCODING.
    /// </summary>
    public StyleRasterDemEncoding? RasterEncoding { get; set; }
}
