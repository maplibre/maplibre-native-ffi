// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_camera_fit_options</c>.
/// </summary>
/// <remarks>
/// See <c>mln_camera_fit_option_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum CameraFitOptionField : uint
{
    Padding = 1,
    Bearing = 2,
    Pitch = 4,
}
