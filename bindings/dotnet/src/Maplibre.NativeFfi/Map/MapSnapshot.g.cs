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
/// <param name="DebugOptions">
/// Debug overlay mask of <c>mln_map_debug_option</c> values.
/// </param>
/// <param name="FullyLoaded">
/// True once every requested style and tile resource finished loading.
/// </param>
/// <param name="GestureInProgress">
/// True while the map is inside a gesture.
/// </param>
/// <param name="LatestRenderUpdateGeneration">
/// Generation of the latest render update the map published. A rendered frame
/// at or past it draws map state that includes every command this snapshot
/// observes, though animations and resource loads finish in later frames.
/// </param>
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
