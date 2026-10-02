// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi.Render;

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
);
