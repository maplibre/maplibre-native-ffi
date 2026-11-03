// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The kind of an event.
/// </summary>
/// <remarks>
/// <para>Only maps in <c>MapMode.Continuous</c> raise
/// <c>RuntimeEventType.MapLoadingFinished</c>, <c>RuntimeEventType.MapIdle</c>,
/// and the render frame and render map events. Offline region events come only
/// for the regions and types that
/// <c>RuntimeHandle.SetOfflineRegionObservedAsync()</c> selects.</para>
/// <para>See <c>mln_runtime_event_type</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.</para>
/// </remarks>
public enum RuntimeEventType : uint
{
    MapCameraWillChange = 1,
    MapCameraIsChanging = 2,
    MapCameraDidChange = 3,
    MapStyleLoaded = 4,

    /// <summary>
    /// The map started to load a style.
    /// </summary>
    MapLoadingStarted = 5,

    /// <summary>
    /// The map loaded its style and everything that the current view needs, and
    /// rendered the result.
    /// </summary>
    MapLoadingFinished = 6,
    MapLoadingFailed = 7,

    /// <summary>
    /// The map rendered a complete frame and has nothing left to load, animate,
    /// or redraw.
    /// </summary>
    MapIdle = 8,
    MapRenderError = 9,
    MapRenderFrameStarted = 10,
    MapRenderFrameFinished = 11,
    MapRenderMapStarted = 12,
    MapRenderMapFinished = 13,
    MapStyleImageMissing = 14,

    /// <summary>
    /// A tile moved to another step of loading.
    /// </summary>
    MapTileAction = 15,
    MapSourceChanged = 16,
    OfflineRegionStatusChanged = 17,

    /// <summary>
    /// A resource that the region needs failed to download.
    /// </summary>
    OfflineRegionResponseError = 18,

    /// <summary>
    /// The region's download became inactive because the cache file's offline
    /// regions reached their limit of 6000 tiles from MapLibre's demo tile
    /// server.
    /// </summary>
    /// <remarks>
    /// Only tiles of sources on the demo server count toward the limit: sources
    /// with a <c>maplibre://</c> URL, which resolves to
    /// https://demotiles.maplibre.org. Tiles from other servers never count.
    /// </remarks>
    OfflineRegionTileCountLimitExceeded = 19,
    MapGlyphsRequested = 20,
    MapGlyphsLoaded = 21,
    MapGlyphsError = 22,
    MapSpriteRequested = 23,
    MapSpriteLoaded = 24,
    MapSpriteError = 25,
}
