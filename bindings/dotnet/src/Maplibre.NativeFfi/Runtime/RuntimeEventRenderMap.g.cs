// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload for <c>RuntimeEventType.MapRenderMapFinished</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_render_map</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RuntimeEventRenderMap(RenderMode Mode);
