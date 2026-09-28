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
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_style_tile_source_options_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyStyleTileSourceOptions(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_style_tile_source_options_default()
            );
        }
    }
}
