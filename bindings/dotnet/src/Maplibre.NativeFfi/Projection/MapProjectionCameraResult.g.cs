// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A projection's camera, as <c>MapProjectionHandle.GetCamera()</c> reports it.
/// </summary>
/// <remarks>
/// See <c>mln_map_projection_camera_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct MapProjectionCameraResult(Camera Camera);
