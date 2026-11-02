// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The end of a camera change.
/// </summary>
/// <remarks>
/// See <c>mln_camera_transition_end</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Generation">
/// The generation of the first map snapshot that shows where the change left
/// the camera, or 0 when the map closed first.
/// </param>
public readonly partial record struct CameraTransitionEnd(
    CameraTransitionOutcome Outcome,
    ulong Generation
);
