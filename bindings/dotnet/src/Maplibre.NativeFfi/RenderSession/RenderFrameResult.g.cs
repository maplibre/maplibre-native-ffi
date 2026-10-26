// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Terminal result of one frame demand, held by an owned frame-result batch and
/// copied by <c>AcquiredFrameHandle.GetResult()</c>.
/// </summary>
/// <remarks>
/// See <c>mln_render_frame_result</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
/// <param name="MapUpdateGeneration">
/// Generation of the map render update the demand evaluated. When disposition
/// is <c>RenderResult.Rendered</c>, the frame drew that update; compare it with
/// <c>CommandCompletion.RenderUpdateGeneration</c> to find the first frame that
/// includes a command.
/// </param>
/// <param name="Generation">
/// The session's target generation when the demand reached its result. See
/// <c>RenderSessionSnapshot.Generation</c>.
/// </param>
/// <param name="FrameGeneration">
/// Position of the frame among the frames that the session rendered, counting
/// from one. Zero unless disposition is <c>RenderResult.Rendered</c>.
/// </param>
/// <param name="NeedsRepaint">
/// Whether the map asked for another frame while it rendered this one, as
/// during an ongoing paint transition. Set only when disposition is
/// <c>RenderResult.Rendered</c>, and false for every other outcome. It matches
/// the needs_repaint field of <c>RuntimeEventType.MapRenderFrameFinished</c>. A
/// camera transition does not set it: the map publishes an update for each of
/// the transition's frames instead, which a render-if-needed demand renders.
/// </param>
/// <param name="View">
/// The view that this frame drew, from the transform of the map update that it
/// rendered. A host that displays the frame builds its projection with
/// <c>MapProjectionHandle.Create()</c> to place overlays where the frame drew
/// their coordinates. Zero, with a size of zero, unless disposition is
/// <c>RenderResult.Rendered</c>.
/// </param>
public readonly partial record struct RenderFrameResult(
    RenderResult Disposition,
    ulong Token,
    ulong MapUpdateGeneration,
    ulong Generation,
    ulong FrameGeneration,
    bool NeedsRepaint,
    MapView View
);
