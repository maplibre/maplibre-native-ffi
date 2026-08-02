using Maplibre.NativeFfi.Geo;
using Maplibre.NativeFfi.Internal.C;

namespace Maplibre.NativeFfi.Internal.Struct;

internal static class CoreStructs
{
    internal static mln_lat_lng ToNative(LatLng value) =>
        new() { latitude = value.Latitude, longitude = value.Longitude };

    internal static LatLng FromNative(mln_lat_lng value) => new(value.latitude, value.longitude);

    internal static mln_projected_meters ToNative(ProjectedMeters value) =>
        new() { northing = value.Northing, easting = value.Easting };

    internal static ProjectedMeters FromNative(mln_projected_meters value) =>
        new(value.northing, value.easting);
}
