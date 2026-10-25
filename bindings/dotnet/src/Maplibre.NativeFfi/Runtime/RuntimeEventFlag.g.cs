// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Bit values for the map and runtime event subscription masks.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_flag</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
[Flags]
public enum RuntimeEventFlag : ulong
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

    /// <summary>
    /// Selects every map-originated event type this version defines.
    /// </summary>
    AllMapEvents = 131070,

    /// <summary>
    /// Selects every runtime-originated event type this version defines.
    /// </summary>
    AllRuntimeEvents = 917504,

    /// <summary>
    /// Selects every event type this version defines.
    /// </summary>
    All = 1048574,
}
