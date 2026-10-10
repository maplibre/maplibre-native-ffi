// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Bit values for the map and runtime event subscription masks.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_mask</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum RuntimeEventMask : ulong
{
    /// <summary>
    /// Selects no event type.
    /// </summary>
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
    OfflineRegionStatusChanged = 524288,
    OfflineRegionResponseError = 1048576,
    OfflineRegionTileCountLimitExceeded = 2097152,

    /// <summary>
    /// Selects every map-originated event type this version defines.
    /// </summary>
    AllMapEvents = 524286,

    /// <summary>
    /// Selects every runtime-originated event type this version defines.
    /// </summary>
    AllRuntimeEvents = 3670016,

    /// <summary>
    /// Selects every event type this version defines.
    /// </summary>
    All = 4194302,
}
