// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Gesture boundary carried atomically with a camera update.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_phase</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum GesturePhase : uint
{
    /// <summary>
    /// The update carries no gesture boundary and leaves the flag as it is.
    /// </summary>
    None = 0,

    /// <summary>
    /// Marks a gesture as in progress before the camera write. It does not
    /// cancel running transitions; use <c>mln_map_cancel_transitions()</c> for
    /// that.
    /// </summary>
    Begin = 1,

    /// <summary>
    /// Keeps the gesture marked as in progress before the camera write.
    /// </summary>
    Update = 2,

    /// <summary>
    /// Clears the gesture flag after the camera write.
    /// </summary>
    End = 3,

    /// <summary>
    /// Cancels transitions running after the camera write, then clears the
    /// gesture flag.
    /// </summary>
    Cancel = 4,
}
