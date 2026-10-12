// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload for <c>MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_render_map</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Mode">
/// One of <c>mln_render_mode</c>.
/// </param>
public readonly partial record struct RuntimeEventRenderMap(RenderMode Mode);
