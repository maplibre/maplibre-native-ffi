// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A complete camera, as native reports it.
/// </summary>
/// <remarks>
/// See <c>mln_camera</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct Camera(
    LatLng Center,
    double CenterAltitude,
    EdgeInsets Padding,
    double Zoom,
    double Bearing,
    double Pitch,
    double Roll,
    double FieldOfView
);
