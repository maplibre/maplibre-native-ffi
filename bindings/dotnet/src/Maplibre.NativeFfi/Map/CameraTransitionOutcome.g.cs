// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a camera change ended.
/// </summary>
/// <remarks>
/// <para>A camera delta that pans and also scales, rotates, or pitches animates
/// as two parts, and any other change as one. When the parts of a change end in
/// different ways, the change reports <c>CameraTransitionOutcome.Cancelled</c>
/// if any part was cancelled, then <c>CameraTransitionOutcome.Replaced</c> if
/// any part was replaced.</para>
/// <para>See <c>mln_camera_transition_outcome</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public enum CameraTransitionOutcome : uint
{
    /// <summary>
    /// The camera finished the change.
    /// </summary>
    Completed = 0,

    /// <summary>
    /// <c>MapHandle.CancelCameraTransitionsAsync()</c>,
    /// <c>MapHandle.CancelCameraTransitionsWithIdAsync()</c>, a gesture's
    /// <c>GesturePhase.Begin</c> or <c>GesturePhase.Cancel</c>,
    /// <c>MapHandle.SetFreeCameraOptionsAsync()</c>, or a resize that moves the
    /// camera back inside its bounds stopped the change, or the command failed
    /// before it moved the camera.
    /// </summary>
    Cancelled = 1,

    /// <summary>
    /// The map closed before the change ended.
    /// </summary>
    Closed = 2,

    /// <summary>
    /// A later camera change set every camera field that a part of the change
    /// still animated, so that part stopped.
    /// </summary>
    /// <remarks>
    /// A fly animates its center and zoom together, and an anchor animates the
    /// center together with the fields it keeps in place, so a later change of
    /// any of them replaces them all. A part that a later change took only some
    /// fields from keeps animating the others, and ends as they end.
    /// </remarks>
    Replaced = 3,
}
