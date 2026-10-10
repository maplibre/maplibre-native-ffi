// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Runtime event types carried by <c>mln_runtime_event.type</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_type</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum RuntimeEventType : uint
{
    MapCameraWillChange = 1,
    MapCameraIsChanging = 2,
    MapCameraDidChange = 3,
    MapStyleLoaded = 4,
    MapLoadingStarted = 5,
    MapLoadingFinished = 6,
    MapLoadingFailed = 7,
    MapIdle = 8,
    MapRenderUpdateAvailable = 9,
    MapRenderError = 10,
    MapStillImageFinished = 11,
    MapStillImageFailed = 12,
    MapRenderFrameStarted = 13,
    MapRenderFrameFinished = 14,
    MapRenderMapStarted = 15,
    MapRenderMapFinished = 16,
    MapStyleImageMissing = 17,
    MapTileAction = 18,
    OfflineRegionStatusChanged = 19,
    OfflineRegionResponseError = 20,
    OfflineRegionTileCountLimitExceeded = 21,
}
