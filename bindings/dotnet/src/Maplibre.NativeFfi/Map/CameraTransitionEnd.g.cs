// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The end of one camera command's transitions, borrowed for the callback.
/// </summary>
/// <remarks>
/// See <c>mln_camera_transition_end</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Outcome">
/// One of <c>mln_camera_transition_outcome</c>.
/// </param>
/// <param name="Generation">
/// Generation of the published snapshot that shows the camera where the
/// transitions left it. The map events of the change that ended them carry this
/// generation and are queued before the callback runs. Zero for
/// <c>MLN_CAMERA_TRANSITION_OUTCOME_CLOSED</c>.
/// </param>
public readonly partial record struct CameraTransitionEnd(
    CameraTransitionOutcome Outcome,
    ulong Generation
);
