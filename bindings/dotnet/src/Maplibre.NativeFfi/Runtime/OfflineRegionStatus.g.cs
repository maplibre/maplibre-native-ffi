// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Map;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Runtime;

public readonly partial record struct OfflineRegionStatus(
    OfflineRegionDownloadState DownloadState,
    ulong CompletedResourceCount,
    ulong CompletedResourceSize,
    ulong CompletedTileCount,
    ulong RequiredTileCount,
    ulong CompletedTileSize,
    ulong RequiredResourceCount,
    bool RequiredResourceCountIsPrecise,
    bool Complete
);
