// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Callback that one camera command copies to report the end of its
/// transitions. The callback runs on the runtime worker.
/// </summary>
/// <remarks>
/// See <c>mln_camera_transition_handler</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct CameraTransitionHandler(
    CameraTransitionEndCallback? Callback
);
