// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

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
)
{
    public MapSnapshot()
        : this(
            default,
            default,
            default!,
            new LogicalExtent(),
            default!,
            default!,
            default,
            default,
            default,
            default,
            default,
            default,
            default!,
            default!,
            default!
        ) { }
}
