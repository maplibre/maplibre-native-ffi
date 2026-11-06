// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How fast each part of a camera change moves.
/// </summary>
/// <remarks>
/// See <c>mln_camera_velocity</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Pan">
/// How fast the map's content moves on the screen, in logical pixels per
/// second.
/// </param>
/// <param name="Zoom">
/// In zoom levels per second.
/// </param>
/// <param name="Bearing">
/// In degrees per second.
/// </param>
/// <param name="Pitch">
/// In degrees per second.
/// </param>
public readonly partial record struct CameraVelocity(
    ScreenPoint Pan,
    double Zoom,
    double Bearing,
    double Pitch
);
