// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// When a gesture recognizer next needs <c>GestureRecognizerHandle.Tick()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_gesture_next_tick</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/gesture_8h.html">C API reference</see>.
/// </remarks>
public enum GestureNextTick : uint
{
    /// <summary>
    /// Not until the next input.
    /// </summary>
    None = 0,

    /// <summary>
    /// Every display frame, while a delta eases or a gesture, its glide, or its
    /// snap may reach a notch.
    /// </summary>
    Frame = 1,

    /// <summary>
    /// At <c>next_tick_ms</c>, on the inputs' clock: when a press becomes a
    /// long press, or idle deltas end.
    /// </summary>
    At = 2,
}
