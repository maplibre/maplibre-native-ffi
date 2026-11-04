// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a camera change ended.
/// </summary>
/// <remarks>
/// <para>A change that ends in more than one way reports
/// <c>CameraTransitionOutcome.Cancelled</c> if any of it was cancelled, and
/// otherwise <c>CameraTransitionOutcome.Replaced</c> if any of it was
/// replaced.</para>
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
    /// <c>MapHandle.SetFreeCameraOptionsAsync()</c>, or a resize that moved the
    /// camera back inside its bounds stopped the change. A command that fails
    /// before it moves the camera also reports this.
    /// </summary>
    Cancelled = 1,

    /// <summary>
    /// The map closed before the change ended.
    /// </summary>
    Closed = 2,

    /// <summary>
    /// A later camera change set every field that a part of the change was
    /// still animating, so that part stopped. Its other parts keep animating,
    /// and the change reports this outcome when they end, unless one was
    /// cancelled.
    /// </summary>
    /// <remarks>
    /// A later change that sets only some of those fields leaves the change
    /// animating the rest. A fly's center and zoom count as one field, and so
    /// do an anchored change's center and the fields that the anchor holds in
    /// place.
    /// </remarks>
    Replaced = 3,
}
