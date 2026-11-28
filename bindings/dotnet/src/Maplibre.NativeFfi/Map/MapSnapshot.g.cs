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
/// <param name="GestureInProgress">
/// Whether a gesture recognizer's gesture, or the glide and snap after it,
/// moves the camera.
/// </param>
/// <param name="LatestRenderUpdateGeneration">
/// Counts the render updates that the map has made for its render session.
/// </param>
/// <param name="GlobeRendering">
/// Whether the map draws as a globe, alone or blending into Mercator.
/// </param>
public readonly partial record struct MapSnapshot(
    ulong Generation,
    bool FullyLoaded,
    bool GestureInProgress,
    bool TransitionInProgress,
    RuntimeEventFlag EventMask,
    ulong LatestRenderUpdateGeneration,
    MapTileSettings Tile,
    CameraBounds Bounds,
    FreeCamera FreeCamera,
    MapView View,
    bool GlobeRendering
);
