// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A callback for the end of a camera change, for <c>AnimationOptions</c>.
/// </summary>
/// <remarks>
/// <para>The callback runs once for each camera command, after every part of
/// the change that the command started has ended.</para>
/// <para>See <c>mln_camera_transition_handler</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.</para>
/// </remarks>
public readonly partial record struct CameraTransitionHandler(
    CameraTransitionEndCallback? Callback
);
