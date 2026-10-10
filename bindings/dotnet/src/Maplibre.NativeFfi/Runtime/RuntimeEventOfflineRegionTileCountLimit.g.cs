// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload for
/// <c>MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_offline_region_tile_count_limit</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
public readonly partial record struct RuntimeEventOfflineRegionTileCountLimit(
    long RegionId,
    ulong Limit
);
