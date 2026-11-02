// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The details of <c>RuntimeEventType.OfflineRegionTileCountLimitExceeded</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_offline_region_tile_count_limit</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Limit">
/// The number of demo server tiles that the cache file's offline regions may
/// hold together.
/// </param>
public readonly partial record struct RuntimeEventOfflineRegionTileCountLimit(
    long RegionId,
    ulong Limit
);
