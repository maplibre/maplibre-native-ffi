// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// An event type, as a bit in a set.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum RuntimeEventFlag : ulong
{
    None = 0,
    MapCameraWillChange = 2,
    MapCameraIsChanging = 4,
    MapCameraDidChange = 8,
    MapStyleLoaded = 16,
    MapLoadingStarted = 32,
    MapLoadingFinished = 64,
    MapLoadingFailed = 128,
    MapIdle = 256,
    MapRenderError = 512,
    MapRenderFrameStarted = 1024,
    MapRenderFrameFinished = 2048,
    MapRenderMapStarted = 4096,
    MapRenderMapFinished = 8192,
    MapStyleImageMissing = 16384,
    MapTileAction = 32768,
    MapSourceChanged = 65536,
    OfflineRegionStatusChanged = 131072,
    OfflineRegionResponseError = 262144,
    OfflineRegionTileCountLimitExceeded = 524288,
    MapGlyphsRequested = 1048576,
    MapGlyphsLoaded = 2097152,
    MapGlyphsError = 4194304,
    MapSpriteRequested = 8388608,
    MapSpriteLoaded = 16777216,
    MapSpriteError = 33554432,
    AllMapEvents = 66191358,
    AllRuntimeEvents = 917504,
    All = 67108862,
}
