// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Where a camera change falls in a gesture that the user makes.
/// </summary>
/// <remarks>
/// <para><c>GesturePhase.Begin</c> and <c>GesturePhase.Update</c> mark a
/// gesture in progress, and <c>GesturePhase.End</c> and
/// <c>GesturePhase.Cancel</c> end it.</para>
/// <para>See <c>mln_gesture_phase</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public enum GesturePhase : uint
{
    /// <summary>
    /// The change isn't part of a gesture.
    /// </summary>
    None = 0,

    /// <summary>
    /// The gesture starts. This cancels every running camera animation before
    /// the change applies.
    /// </summary>
    Begin = 1,

    /// <summary>
    /// The gesture continues. To start a gesture that keeps the running camera
    /// animations, start it with this phase instead of
    /// <c>GesturePhase.Begin</c>.
    /// </summary>
    Update = 2,
    End = 3,

    /// <summary>
    /// The platform cancelled the gesture. This also cancels every running
    /// camera animation after the change applies.
    /// </summary>
    Cancel = 4,
}
