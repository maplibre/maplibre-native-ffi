// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// How a camera change ended.
/// </summary>
/// <remarks>
/// See <c>mln_camera_transition_outcome</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public enum CameraTransitionOutcome : uint
{
    /// <summary>
    /// The camera finished the change, or a later camera change replaced it.
    /// </summary>
    Completed = 0,

    /// <summary>
    /// <c>MapHandle.CancelTransitionsAsync()</c>,
    /// <c>MapHandle.CancelCameraTransitionAsync()</c>, or a gesture's
    /// <c>GesturePhase.Cancel</c> stopped the change.
    /// </summary>
    Cancelled = 1,

    /// <summary>
    /// The map closed before the change ended.
    /// </summary>
    Closed = 2,
}
