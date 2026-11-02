// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// The download progress of an offline region.
/// </summary>
/// <remarks>
/// See <c>mln_offline_region_status</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="CompletedResourceCount">
/// The resources downloaded so far, tiles included.
/// </param>
/// <param name="CompletedResourceSize">
/// The size of the downloaded resources in bytes, tiles included.
/// </param>
/// <param name="CompletedTileSize">
/// The size of the downloaded tiles in bytes.
/// </param>
/// <param name="RequiredResourceCount">
/// The resources that the region needs, tiles included. Until
/// <c>required_resource_count_is_precise</c> is true, this is only a lower
/// bound.
/// </param>
/// <param name="Complete">
/// Whether the region has downloaded every resource it needs.
/// </param>
public readonly partial record struct OfflineRegionStatus(
    OfflineRegionDownloadState DownloadState,
    ulong CompletedResourceCount,
    ulong CompletedResourceSize,
    ulong CompletedTileCount,
    ulong RequiredTileCount,
    ulong CompletedTileSize,
    ulong RequiredResourceCount,
    bool RequiredResourceCountIsPrecise,
    bool Complete
);
