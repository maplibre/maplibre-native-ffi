// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

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
            using var call = NativeCall.Enter(null, "mln_map_tile_options_default");
            return GeneratedValues.CopyMapTileOptions(NativeMethods.mln_map_tile_options_default());
        }
    }
}
