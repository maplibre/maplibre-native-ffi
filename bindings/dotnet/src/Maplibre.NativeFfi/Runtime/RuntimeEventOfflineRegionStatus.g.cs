// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// Payload for <c>MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED</c>.
/// </summary>
/// <remarks>
/// See <c>mln_runtime_event_offline_region_status</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html">C API reference</see>.
/// </remarks>
/// <param name="Status">
/// Region status. This member keeps its own size field because the same struct
/// is also returned by <c>mln_runtime_offline_region_get_status()</c>.
/// </param>
public readonly partial record struct RuntimeEventOfflineRegionStatus(
    long RegionId,
    OfflineRegionStatus Status
);
