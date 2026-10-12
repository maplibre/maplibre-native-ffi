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
