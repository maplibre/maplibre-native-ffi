// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Geographic coordinate in degrees used by map and projection APIs.
/// </summary>
/// <remarks>
/// See <c>mln_lat_lng</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Latitude">
/// Latitude in degrees. Input latitude must be finite and within [-90, 90].
/// </param>
/// <param name="Longitude">
/// Longitude in degrees. Input longitude must be finite.
/// </param>
public readonly partial record struct LatLng(double Latitude, double Longitude);
