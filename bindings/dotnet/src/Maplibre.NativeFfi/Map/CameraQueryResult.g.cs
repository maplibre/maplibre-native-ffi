// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Camera result borrowed for an ordered camera-query completion.
/// </summary>
/// <remarks>
/// See <c>mln_camera_query_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct CameraQueryResult(ulong Generation, CameraOptions Camera);
