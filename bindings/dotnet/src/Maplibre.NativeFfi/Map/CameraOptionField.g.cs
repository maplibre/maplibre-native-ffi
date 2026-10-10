// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_camera_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_camera_option_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum CameraOptionField : uint
{
    Center = 1,
    Zoom = 2,
    Bearing = 4,
    Pitch = 8,
    CenterAltitude = 16,
    Padding = 32,
    Anchor = 64,
    Roll = 128,
    Fov = 256,
}
