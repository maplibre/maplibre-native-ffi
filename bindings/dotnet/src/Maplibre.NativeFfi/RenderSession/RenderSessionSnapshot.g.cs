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
public readonly partial record struct RenderSessionSnapshot(
    RenderSessionState State,
    RenderDriverKind Driver,
    RenderResult LatestResult,
    RenderTargetExtent Extent,
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
            new RenderTargetExtent(),
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
