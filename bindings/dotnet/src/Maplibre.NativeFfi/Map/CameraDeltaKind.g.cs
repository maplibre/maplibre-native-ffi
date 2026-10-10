// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Relative camera operation carried by <c>mln_camera_delta</c>.
/// </summary>
/// <remarks>
/// See <c>mln_camera_delta_kind</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum CameraDeltaKind : uint
{
    Move = 0,
    Scale = 1,
    Bearing = 2,
    Pitch = 3,
}
