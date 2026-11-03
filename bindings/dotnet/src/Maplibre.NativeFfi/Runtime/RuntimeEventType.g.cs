// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// The kind of an event.
/// </summary>
/// <remarks>
/// <para>An event's <c>payload_type</c> names the member of its payload that
/// holds the event's details. An event that reports a failure carries the
/// error's text in <c>message</c>.</para>
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
    /// <summary>
    /// The camera is about to change. Its details are in <c>camera_change</c>.
    /// </summary>
    MapCameraWillChange = 1,
    MapCameraIsChanging = 2,

    /// <summary>
    /// The camera changed. Its details are in <c>camera_change</c>.
    /// </summary>
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

    /// <summary>
    /// The map failed to load a style. Its details are in
    /// <c>loading_failed</c>, and the error's text is in <c>message</c>.
    /// </summary>
    MapLoadingFailed = 7,

    /// <summary>
    /// The map rendered a complete frame and has nothing left to load, animate,
    /// or redraw.
    /// </summary>
    MapIdle = 8,

    /// <summary>
    /// The map failed to render. The error's text is in <c>message</c>.
    /// </summary>
    MapRenderError = 9,
    MapRenderFrameStarted = 10,

    /// <summary>
    /// The map rendered a frame. Its details are in <c>render_frame</c>.
    /// </summary>
    MapRenderFrameFinished = 11,
    MapRenderMapStarted = 12,
    MapRenderMapFinished = 13,

    /// <summary>
    /// The style uses an image that it doesn't have. Its details are in
    /// <c>style_image</c>.
    /// </summary>
    MapStyleImageMissing = 14,

    /// <summary>
    /// A tile moved to another step of loading. Its details are in
    /// <c>tile_action</c>.
    /// </summary>
    MapTileAction = 15,

    /// <summary>
    /// A source changed. Its details are in <c>source_changed</c>.
    /// </summary>
    MapSourceChanged = 16,

    /// <summary>
    /// The region's status changed. Its details are in
    /// <c>offline_region_status</c>.
    /// </summary>
    OfflineRegionStatusChanged = 17,

    /// <summary>
    /// A resource that the region needs failed to download. Its details are in
    /// <c>offline_region_response_error</c>, and the error's text is in
    /// <c>message</c>.
    /// </summary>
    OfflineRegionResponseError = 18,

    /// <summary>
    /// The region's download became inactive because the cache file's offline
    /// regions reached their limit of 6000 tiles from MapLibre's demo tile
    /// server. Its details are in <c>offline_region_tile_count_limit</c>.
    /// </summary>
    /// <remarks>
    /// Only tiles of sources on the demo server count toward the limit: sources
    /// with a <c>maplibre://</c> URL, which resolves to
    /// https://demotiles.maplibre.org. Tiles from other servers never count.
    /// </remarks>
    OfflineRegionTileCountLimitExceeded = 19,

    /// <summary>
    /// The map requested a range of glyphs. Its details are in <c>glyphs</c>.
    /// </summary>
    MapGlyphsRequested = 20,

    /// <summary>
    /// A range of glyphs loaded. Its details are in <c>glyphs</c>.
    /// </summary>
    MapGlyphsLoaded = 21,

    /// <summary>
    /// A range of glyphs failed to load. Its details are in <c>glyphs</c>, and
    /// the error's text is in <c>message</c>.
    /// </summary>
    MapGlyphsError = 22,

    /// <summary>
    /// The map requested a sprite. Its details are in <c>sprite</c>.
    /// </summary>
    MapSpriteRequested = 23,

    /// <summary>
    /// A sprite loaded. Its details are in <c>sprite</c>.
    /// </summary>
    MapSpriteLoaded = 24,

    /// <summary>
    /// A sprite failed to load. Its details are in <c>sprite</c>, and the
    /// error's text is in <c>message</c>.
    /// </summary>
    MapSpriteError = 25,
}
