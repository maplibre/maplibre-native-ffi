// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;
using static Maplibre.NativeFfi.Internal.Struct.NativeValues;

namespace Maplibre.NativeFfi.Map;

public sealed record MapTileOptions
{
    public uint? PrefetchZoomDelta { get; set; }
    public double? LodMinRadius { get; set; }
    public double? LodScale { get; set; }
    public double? LodPitchThreshold { get; set; }
    public double? LodZoomShift { get; set; }
    public TileLodMode? LodMode { get; set; }
    public static MapTileOptions Default
    {
        get
        {
            using var call = Enter(null, "mln_map_tile_options_default");
            return CopyMapTileOptions(NativeMethods.mln_map_tile_options_default());
        }
    }
}
