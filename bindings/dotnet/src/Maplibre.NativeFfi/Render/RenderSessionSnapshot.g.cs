// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

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
