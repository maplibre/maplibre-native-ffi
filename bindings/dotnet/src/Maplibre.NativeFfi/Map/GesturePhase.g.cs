// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Where a camera change falls in a gesture that the user makes.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_phase</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum GesturePhase : uint
{
    /// <summary>
    /// The change isn't part of a gesture.
    /// </summary>
    None = 0,
    Begin = 1,
    Update = 2,
    End = 3,

    /// <summary>
    /// The platform cancelled the gesture. This also cancels camera changes.
    /// </summary>
    Cancel = 4,
}
