// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// A map's state, as <c>MapHandle.GetSnapshot()</c> reads it.
/// </summary>
/// <remarks>
/// See <c>mln_map_snapshot</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Generation">
/// Grows with each change to the map's state.
/// </param>
/// <param name="FullyLoaded">
/// Whether the style and every resource that the view needs have loaded.
/// </param>
/// <param name="LatestRenderUpdateGeneration">
/// Counts the render updates that the map has made for its render session.
/// </param>
public readonly partial record struct MapSnapshot(
    MapDebugFlag DebugFlags,
    ulong Generation,
    bool FullyLoaded,
    bool RenderingStatsViewEnabled,
    bool GestureInProgress,
    bool TransitionInProgress,
    RuntimeEventFlag EventMask,
    ulong LatestRenderUpdateGeneration,
    MapTileOptions Tile,
    BoundOptions Bounds,
    FreeCameraOptions FreeCamera,
    MapView View
);
