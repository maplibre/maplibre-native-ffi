// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A map's whole camera.
/// </summary>
/// <remarks>
/// <para>Its members mean the same as those of <c>CameraOptions</c>.</para>
/// <para>See <c>mln_camera</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
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
