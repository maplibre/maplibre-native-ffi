// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A map's whole camera.
/// </summary>
/// <remarks>
/// See <c>mln_camera</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="CenterAltitude">
/// The altitude of the center, in meters above sea level.
/// </param>
/// <param name="Padding">
/// Insets from the map's edges. The camera centers within the rest.
/// </param>
/// <param name="Bearing">
/// The rotation from north, in degrees.
/// </param>
/// <param name="Pitch">
/// The tilt toward the horizon, in degrees. 0 looks straight down.
/// </param>
/// <param name="Roll">
/// The roll, in degrees.
/// </param>
/// <param name="FieldOfView">
/// The vertical field of view, in degrees.
/// </param>
public readonly partial record struct Camera(
    LatLng Center,
    double CenterAltitude,
    EdgeInsets Padding,
    double Zoom,
    double Bearing,
    double Pitch,
    double Roll,
    double FieldOfView
)
{
    /// <summary>
    /// Converts this <c>Camera</c> to <c>CameraOptions</c> that set each of its
    /// members.
    /// </summary>
    public CameraOptions ToOptions() =>
        new()
        {
            Center = Center,
            CenterAltitude = CenterAltitude,
            Padding = Padding,
            Zoom = Zoom,
            Bearing = Bearing,
            Pitch = Pitch,
            Roll = Roll,
            FieldOfView = FieldOfView,
        };
}
