// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload for <c>MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_camera_transition_finished</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="TransitionId">
/// The transition_id the caller set on the <c>mln_animation_options</c> that
/// started this transition.
/// </param>
public readonly partial record struct RuntimeEventCameraTransitionFinished(ulong TransitionId);
