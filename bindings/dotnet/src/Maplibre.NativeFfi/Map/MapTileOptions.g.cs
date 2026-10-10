// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Tile prefetch and LOD tuning controls.
/// </summary>
/// <remarks>
/// See <c>mln_map_tile_options</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record MapTileOptions
{
    /// <summary>
    /// Native uint8_t prefetch zoom delta.
    /// </summary>
    public uint? PrefetchZoomDelta { get; set; }
    public double? LodMinRadius { get; set; }
    public double? LodScale { get; set; }
    public double? LodPitchThreshold { get; set; }
    public double? LodZoomShift { get; set; }

    /// <summary>
    /// One of <c>mln_tile_lod_mode</c>.
    /// </summary>
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
