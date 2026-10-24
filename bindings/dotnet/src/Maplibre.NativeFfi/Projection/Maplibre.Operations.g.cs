// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using static Maplibre.NativeFfi.Internal.NativeCall;
using static Maplibre.NativeFfi.Internal.Struct.GeneratedValues;

namespace Maplibre.NativeFfi;

public static unsafe partial class Maplibre
{
    /// <summary>
    /// Converts spherical Mercator projected meters to a geographic coordinate.
    /// </summary>
    /// <remarks>
    /// See <c>mln_lat_lng_for_projected_meters</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public static LatLng LatLngForProjectedMeters(ProjectedMeters meters)
    {
        using var call = Enter(null);
        var outCoordinate = default(mln_lat_lng);
        Check(
            NativeMethods.mln_lat_lng_for_projected_meters(
                NativeProjectedMeters(meters),
                &outCoordinate,
                Diagnostic
            )
        );
        return CopyLatLng(outCoordinate);
    }

    /// <summary>
    /// Converts a geographic coordinate to spherical Mercator projected meters.
    /// </summary>
    /// <remarks>
    /// See <c>mln_projected_meters_for_lat_lng</c> in the <see
    /// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
    /// </remarks>
    public static ProjectedMeters ProjectedMetersForLatLng(LatLng coordinate)
    {
        using var call = Enter(null);
        var outMeters = default(mln_projected_meters);
        Check(
            NativeMethods.mln_projected_meters_for_lat_lng(
                NativeLatLng(coordinate),
                &outMeters,
                Diagnostic
            )
        );
        return CopyProjectedMeters(outMeters);
    }
}
