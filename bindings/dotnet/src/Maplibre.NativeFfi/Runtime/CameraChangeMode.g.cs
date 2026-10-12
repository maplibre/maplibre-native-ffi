// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Camera change kinds reported by camera will-change and did-change events.
/// </summary>
/// <remarks>
/// See <c>mln_camera_change_mode</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum CameraChangeMode : uint
{
    /// <summary>
    /// The camera reached its new value without an animated transition.
    /// </summary>
    Immediate = 0,

    /// <summary>
    /// The camera moved as part of an animated transition.
    /// </summary>
    Animated = 1,
}
