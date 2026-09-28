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
            global::Maplibre.NativeFfi.Internal.Callback.NativeCallbackGuard.EnsureAllowed(
                null,
                "mln_custom_geometry_source_options_default"
            );
            global::Maplibre.NativeFfi.Internal.Loader.NativeLibraryLoader.EnsureLoaded();
            return global::Maplibre.NativeFfi.Internal.Struct.GeneratedValues.CopyCustomGeometrySourceOptions(
                global::Maplibre.NativeFfi.Internal.C.NativeMethods.mln_custom_geometry_source_options_default()
            );
        }
    }
}
