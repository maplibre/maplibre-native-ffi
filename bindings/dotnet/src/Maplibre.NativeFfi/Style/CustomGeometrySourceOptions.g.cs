// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Style;

public sealed record CustomGeometrySourceOptions
{
    public Action<CanonicalTileId>? FetchTile { get; set; }
    public Action<CanonicalTileId>? CancelTile { get; set; }
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public double? Tolerance { get; set; }
    public uint? TileSize { get; set; }
    public uint? Buffer { get; set; }
    public bool? Clip { get; set; }
    public bool? Wrap { get; set; }
    public static CustomGeometrySourceOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_custom_geometry_source_options_default");
            return CopyCustomGeometrySourceOptions(
                NativeMethods.mln_custom_geometry_source_options_default()
            );
        }
    }
}
