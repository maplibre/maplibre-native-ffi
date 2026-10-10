// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Geographic bounds in degrees.
/// </summary>
/// <remarks>
/// See <c>mln_lat_lng_bounds</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct LatLngBounds(LatLng Southwest, LatLng Northeast);
