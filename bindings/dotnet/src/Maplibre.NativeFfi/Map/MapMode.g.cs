// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Map rendering modes used when creating a map.
/// </summary>
/// <remarks>
/// See <c>mln_map_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum MapMode : uint
{
    /// <summary>
    /// Continuously updates as data arrives and map state changes.
    /// </summary>
    Continuous = 0,

    /// <summary>
    /// Produces one-off still images of an arbitrary viewport.
    /// </summary>
    Static = 1,

    /// <summary>
    /// Produces one-off still images for a single tile.
    /// </summary>
    Tile = 2,
}
