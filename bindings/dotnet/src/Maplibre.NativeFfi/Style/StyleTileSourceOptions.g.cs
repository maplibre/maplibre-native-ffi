// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Style;

public sealed record StyleTileSourceOptions
{
    public double? MinZoom { get; set; }
    public double? MaxZoom { get; set; }
    public string? Attribution { get; set; }
    public StyleTileScheme? Scheme { get; set; }
    public LatLngBounds? Bounds { get; set; }
    public uint? TileSize { get; set; }
    public StyleVectorTileEncoding? VectorEncoding { get; set; }
    public StyleRasterDemEncoding? RasterEncoding { get; set; }
    public static StyleTileSourceOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_style_tile_source_options_default");
            return CopyStyleTileSourceOptions(
                NativeMethods.mln_style_tile_source_options_default()
            );
        }
    }
}
