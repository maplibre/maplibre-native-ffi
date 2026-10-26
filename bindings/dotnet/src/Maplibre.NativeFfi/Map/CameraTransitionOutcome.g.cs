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
    /// <c>MapHandle.CancelTransitionsAsync()</c>,
    /// <c>MapHandle.CancelCameraTransitionAsync()</c>, or
    /// <c>GesturePhase.Cancel</c> ended the transitions, or the command failed
    /// before it started them.
    /// </summary>
    Cancelled = 1,

    /// <summary>
    /// The map closed before the transitions ended.
    /// </summary>
    Closed = 2,
}
