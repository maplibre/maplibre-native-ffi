// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_map_tile_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_map_tile_option_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum MapTileOptionField : uint
{
    PrefetchZoomDelta = 1,
    LodMinRadius = 2,
    LodScale = 4,
    LodPitchThreshold = 8,
    LodZoomShift = 16,
    LodMode = 32,
}
