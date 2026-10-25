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
    MapRenderError = 9,
    MapRenderFrameStarted = 10,
    MapRenderFrameFinished = 11,
    MapRenderMapStarted = 12,
    MapRenderMapFinished = 13,
    MapStyleImageMissing = 14,
    MapTileAction = 15,
    MapSourceChanged = 16,
    OfflineRegionStatusChanged = 17,
    OfflineRegionResponseError = 18,
    OfflineRegionTileCountLimitExceeded = 19,
}
