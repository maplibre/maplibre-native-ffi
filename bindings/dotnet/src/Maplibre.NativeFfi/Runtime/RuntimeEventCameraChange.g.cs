// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The details of <c>RuntimeEventType.MapCameraWillChange</c> and
/// <c>RuntimeEventType.MapCameraDidChange</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_camera_change</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RuntimeEventCameraChange(
    CameraChangeMode Mode,
    CameraChangeSource Source
);
