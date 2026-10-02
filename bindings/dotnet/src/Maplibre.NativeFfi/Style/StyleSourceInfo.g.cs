// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Style;

public sealed record StyleSourceInfo
{
    public StyleSourceType Type { get; set; }
    public ulong IdSize { get; set; }
    public bool IsVolatile { get; set; }
    public ulong? AttributionSize { get; set; }
    public ulong? UrlSize { get; set; }
    public StyleSourceTileInfo? Tilejson { get; set; }
    public LatLngBounds? Bounds { get; set; }
    public uint? TileSize { get; set; }
    public StyleVectorTileEncoding? VectorEncoding { get; set; }
    public StyleRasterDemEncoding? RasterEncoding { get; set; }
}
