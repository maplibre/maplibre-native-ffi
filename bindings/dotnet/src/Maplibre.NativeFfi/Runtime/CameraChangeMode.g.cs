// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Whether a camera change jumps or animates.
/// </summary>
/// <remarks>
/// See <c>mln_camera_change_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum CameraChangeMode : uint
{
    Immediate = 0,
    Animated = 1,
}
