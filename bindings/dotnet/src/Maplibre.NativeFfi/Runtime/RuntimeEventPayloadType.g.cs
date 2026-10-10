// Generated from the C headers by tools/bindgen. Do not edit.
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload kinds used by <c>mln_runtime_event.payload_type</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_payload_type</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public enum RuntimeEventPayloadType : uint
{
    None = 0,
    RenderFrame = 1,
    RenderMap = 2,
    TileAction = 4,
    OfflineRegionStatus = 5,
    OfflineRegionResponseError = 6,
    OfflineRegionTileCountLimit = 7,
}
