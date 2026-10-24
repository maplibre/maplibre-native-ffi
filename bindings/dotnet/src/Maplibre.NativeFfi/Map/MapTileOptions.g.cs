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
    /// Prefetch zoom delta, at most 255.
    /// </summary>
    public uint? PrefetchZoomDelta { get; init; }
    public double? LodMinRadius { get; init; }
    public double? LodScale { get; init; }
    public double? LodPitchThreshold { get; init; }
    public double? LodZoomShift { get; init; }

    /// <summary>
    /// One of <c>mln_tile_lod_mode</c>.
    /// </summary>
    public TileLodMode? LodMode { get; init; }
    public static MapTileOptions Default
    {
        get
        {
            using var call = NativeCall.Enter(null);
            return GeneratedValues.CopyMapTileOptions(NativeMethods.mln_map_tile_options_default());
        }
    }
}
