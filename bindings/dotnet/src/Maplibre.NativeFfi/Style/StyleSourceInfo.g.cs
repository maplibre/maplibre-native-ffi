// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Complete metadata of one style source, borrowed for a completion callback.
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
    /// Whether the source is marked volatile.
    /// </summary>
    public bool IsVolatile { get; set; }

    /// <summary>
    /// Attribution string, when the source sets one. It may be empty.
    /// </summary>
    public string? Attribution { get; set; }

    /// <summary>
    /// URL that the source loads from, when it has one.
    /// </summary>
    public string? Url { get; set; }

    /// <summary>
    /// Inline TileJSON metadata, when the source was defined with it.
    /// </summary>
    public StyleSourceTileInfo? Tilejson { get; set; }

    /// <summary>
    /// Geographic bounds, when inline TileJSON sets them.
    /// </summary>
    public LatLngBounds? Bounds { get; set; }

    /// <summary>
    /// Tile size in pixels, for a tile source.
    /// </summary>
    public uint? TileSize { get; set; }

    /// <summary>
    /// Vector tile encoding, for a vector source.
    /// </summary>
    public StyleVectorTileEncoding? VectorEncoding { get; set; }

    /// <summary>
    /// DEM raster encoding, when inline TileJSON sets one.
    /// </summary>
    public StyleRasterDemEncoding? RasterEncoding { get; set; }
}
