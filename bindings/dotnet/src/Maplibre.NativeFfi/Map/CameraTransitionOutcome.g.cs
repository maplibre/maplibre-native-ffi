// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How the transitions of one camera command ended.
/// </summary>
/// <remarks>
/// See <c>mln_camera_transition_outcome</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum CameraTransitionOutcome : uint
{
    /// <summary>
    /// Every property of the command reached its target, or a later camera
    /// write replaced it.
    /// </summary>
    Completed = 0,

    /// <summary>
    /// <c>mln_map_cancel_transitions()</c>,
    /// <c>mln_map_cancel_camera_transition()</c>, or
    /// <c>MLN_GESTURE_PHASE_CANCEL</c> ended the transitions, or the command
    /// failed before it started them.
    /// </summary>
    Cancelled = 1,

    /// <summary>
    /// The map closed before the transitions ended.
    /// </summary>
    Closed = 2,
}
