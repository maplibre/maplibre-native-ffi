// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// All the settings for how a map picks the tiles to load.
/// </summary>
/// <remarks>
/// <para>Its members mean the same as those of <c>MapTileOptions</c>.</para>
/// <para>See <c>mln_map_tile_settings</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
/// <param name="LodPitchThreshold">
/// The pitch, in degrees, above which the map lowers the detail.
/// </param>
public readonly partial record struct MapTileSettings(
    uint PrefetchZoomDelta,
    double LodMinRadius,
    double LodScale,
    double LodPitchThreshold,
    double LodZoomShift,
    TileLodMode LodMode
);
