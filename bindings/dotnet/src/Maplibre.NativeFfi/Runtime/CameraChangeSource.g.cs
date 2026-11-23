// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// What moved the camera.
/// </summary>
/// <remarks>
/// See <c>mln_camera_change_source</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum CameraChangeSource : uint
{
    /// <summary>
    /// A camera command outside a gesture, or the map itself, as when a resize
    /// keeps the camera within its bounds.
    /// </summary>
    Command = 0,

    /// <summary>
    /// A gesture that a gesture recognizer recognized.
    /// </summary>
    Gesture = 1,

    /// <summary>
    /// The glide and snap after a gesture recognizer's gesture.
    /// </summary>
    Momentum = 2,
}
