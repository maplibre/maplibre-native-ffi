// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How the map picks the level of detail of its tiles.
/// </summary>
/// <remarks>
/// See <c>mln_tile_lod_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum TileLodMode : uint
{
    /// <summary>
    /// Uses the most detail at the center of the screen.
    /// </summary>
    Default = 0,

    /// <summary>
    /// Uses the most detail closest to the camera.
    /// </summary>
    Distance = 1,
}
