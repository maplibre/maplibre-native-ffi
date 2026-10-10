// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload for <c>MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_render_frame</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RuntimeEventRenderFrame(
    RenderMode Mode,
    bool NeedsRepaint,
    bool PlacementChanged,
    RenderingStats Stats
);
