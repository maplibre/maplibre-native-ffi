// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Field mask values for <c>mln_camera_delta</c>.
/// </summary>
/// <remarks>
/// See <c>mln_camera_delta_field</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum CameraDeltaField : uint
{
    Offset = 1,
    Scale = 2,
    Bearing = 4,
    Pitch = 8,
    Anchor = 16,
}
