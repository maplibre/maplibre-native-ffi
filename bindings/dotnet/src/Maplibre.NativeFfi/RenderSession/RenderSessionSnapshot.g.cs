// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Any-thread render-session snapshot.
/// </summary>
/// <remarks>
/// See <c>mln_render_session_snapshot</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html">C API reference</see>.
/// </remarks>
/// <param name="State">
/// One <c>mln_render_session_state</c> value.
/// </param>
/// <param name="Driver">
/// One <c>mln_render_driver_kind</c> value.
/// </param>
/// <param name="LatestResult">
/// Most recent terminal <c>mln_render_result</c> value.
/// </param>
/// <param name="Extent">
/// Logical extent, including a resize the driver has not applied yet.
/// </param>
/// <param name="Generation">
/// Increases when the session's lifecycle state, target, or applied extent
/// changes. An acquired texture frame reports the value it rendered under.
/// </param>
/// <param name="MapUpdateGeneration">
/// Generation of the latest map render update that the session has seen.
/// </param>
/// <param name="RenderedUpdateGeneration">
/// Map render update generation that the latest rendered frame drew. Zero
/// before the first frame after attachment or a target replacement.
/// </param>
/// <param name="ExtentGeneration">
/// Starts at one and increases each time the driver applies a resize.
/// </param>
/// <param name="FrameGeneration">
/// Frame generation of the latest rendered frame, zero before the first.
/// </param>
/// <param name="LatestDemandToken">
/// Token of the demand whose terminal result the session queued last.
/// </param>
/// <param name="PendingDemandCount">
/// Accepted demands that have no terminal result yet.
/// </param>
/// <param name="AcquiredFrameCount">
/// Acquired frames that the host has not released or disposed.
/// </param>
/// <param name="TargetReady">
/// False once attachment fails or the session is abandoned.
/// </param>
/// <param name="PendingChanges">
/// True when a map update or an accepted resize arrived after the latest
/// rendered frame.
/// </param>
public readonly partial record struct RenderSessionSnapshot(
    RenderSessionState State,
    RenderDriverKind Driver,
    RenderResult LatestResult,
    LogicalExtent Extent,
    ulong Generation,
    ulong MapUpdateGeneration,
    ulong RenderedUpdateGeneration,
    ulong ExtentGeneration,
    ulong FrameGeneration,
    ulong LatestDemandToken,
    uint PendingDemandCount,
    uint AcquiredFrameCount,
    bool TargetReady,
    bool PendingChanges
)
{
    public RenderSessionSnapshot()
        : this(
            default,
            default,
            default,
            new LogicalExtent(),
            default,
            default,
            default,
            default,
            default,
            default,
            default,
            default,
            default,
            default
        ) { }
}
