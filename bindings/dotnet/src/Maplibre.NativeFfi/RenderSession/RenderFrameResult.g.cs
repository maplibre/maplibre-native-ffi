// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Immutable result record copied into an owned frame-result batch.
/// </summary>
/// <remarks>
/// See <c>mln_render_frame_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Disposition">
/// One <c>mln_render_result</c> value.
/// </param>
/// <param name="FrameGeneration">
/// Zero unless disposition is <c>MLN_RENDER_RESULT_RENDERED</c>.
/// </param>
/// <param name="NeedsRepaint">
/// Whether the map asked for another frame while it rendered this one, as
/// during an ongoing paint transition. Set only when disposition is
/// <c>MLN_RENDER_RESULT_RENDERED</c>, and false for every other outcome. This
/// is the same signal that <c>MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED</c>
/// carries in its needs_repaint field, delivered with the frame result so a
/// host can re-arm its frame loop without the runtime event round trip. A
/// camera transition does not set it by itself: the map publishes a new update
/// after each of the transition's frames instead, which a render-if-needed
/// demand renders. A demand with <c>MLN_FRAME_DEMAND_WAIT_FOR_UPDATE</c>
/// renders each transition update without a runtime-event round trip; the host
/// re-arms the demand as each result arrives.
/// </param>
public readonly partial record struct RenderFrameResult(
    RenderResult Disposition,
    ulong Token,
    ulong MapUpdateGeneration,
    ulong ExtentGeneration,
    ulong FrameGeneration,
    bool NeedsRepaint
);
