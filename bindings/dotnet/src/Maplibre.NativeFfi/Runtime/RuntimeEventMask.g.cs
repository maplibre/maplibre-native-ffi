// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi.Runtime;

[Flags]
public enum RuntimeEventMask : ulong
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
    MapRenderUpdateAvailable = 512,
    MapRenderError = 1024,
    MapStillImageFinished = 2048,
    MapStillImageFailed = 4096,
    MapRenderFrameStarted = 8192,
    MapRenderFrameFinished = 16384,
    MapRenderMapStarted = 32768,
    MapRenderMapFinished = 65536,
    MapStyleImageMissing = 131072,
    MapTileAction = 262144,
    MapCameraTransitionFinished = 4194304,
    OfflineRegionStatusChanged = 524288,
    OfflineRegionResponseError = 1048576,
    OfflineRegionTileCountLimitExceeded = 2097152,
    AllMapEvents = 4718590,
    AllRuntimeEvents = 3670016,
    All = 8388606,
}
