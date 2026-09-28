// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
using Maplibre.NativeFfi.Base;
using Maplibre.NativeFfi.Internal.C;
using Maplibre.NativeFfi.Logging;
using Maplibre.NativeFfi.Query;
using Maplibre.NativeFfi.Render;
using Maplibre.NativeFfi.Runtime;
using Maplibre.NativeFfi.Style;

namespace Maplibre.NativeFfi.Map;

public readonly partial record struct MapSnapshot(
    MapDebugOption DebugOptions,
    ulong Generation,
    CameraOptions Camera,
    LogicalExtent LogicalExtent,
    ProjectionMode ProjectionMode,
    MapViewportOptions Viewport,
    bool FullyLoaded,
    bool RenderingStatsViewEnabled,
    bool RepaintDemand,
    bool GestureInProgress,
    RuntimeEventMask EventMask,
    ulong LatestRenderUpdateGeneration,
    MapTileOptions Tile,
    BoundOptions Bounds,
    FreeCameraOptions FreeCamera
);
