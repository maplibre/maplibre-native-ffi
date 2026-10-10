// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Camera transition behavior for <c>mln_camera_update</c>.
/// </summary>
/// <remarks>
/// See <c>mln_camera_update_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum CameraUpdateMode : uint
{
    Jump = 0,
    Ease = 1,
    Fly = 2,
}
