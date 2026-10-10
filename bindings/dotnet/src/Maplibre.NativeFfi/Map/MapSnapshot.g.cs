// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Immutable map state copied from the latest published generation.
/// </summary>
/// <remarks>
/// See <c>mln_map_snapshot</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
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
