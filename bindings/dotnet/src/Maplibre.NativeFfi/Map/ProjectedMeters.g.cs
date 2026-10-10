// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Lower-level Spherical Mercator projected-meter coordinate.
/// </summary>
/// <remarks>
/// See <c>mln_projected_meters</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Northing">
/// Distance measured northward from the equator, in meters.
/// </param>
/// <param name="Easting">
/// Distance measured eastward from the prime meridian, in meters.
/// </param>
public readonly partial record struct ProjectedMeters(double Northing, double Easting);
