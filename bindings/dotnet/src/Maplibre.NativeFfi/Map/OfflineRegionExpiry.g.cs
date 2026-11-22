// Generated from the C headers by tools/bindgen. Do not edit.
#nullable enable
namespace Maplibre.NativeFfi;

/// <summary>
/// How long the resources that an offline region holds stay usable.
/// </summary>
/// <remarks>
/// See <c>mln_offline_region_expiry</c> in the <see
/// href="https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html">C API reference</see>.
/// </remarks>
public sealed record OfflineRegionExpiry
{
    /// <summary>
    /// When the first of the region's resources goes stale, which may have
    /// passed. Maps show a stale resource while they check it with the server,
    /// unless it must be checked first, and the region's download refreshes it.
    /// Without it, no resource ever goes stale.
    /// </summary>
    public long? StaleAfterUnixMs { get; init; }

    /// <summary>
    /// When the first of the region's resources that the server requires
    /// checking once stale stops being usable without the server, which may
    /// have passed. Refresh the region before then to keep all of it usable
    /// offline. Without it, every resource stays usable offline.
    /// </summary>
    public long? UsableOfflineUntilUnixMs { get; init; }

    /// <summary>
    /// The resources that are stale now, tiles included.
    /// </summary>
    public ulong StaleResourceCount { get; init; }

    /// <summary>
    /// The stale resources that maps can't use now without the server.
    /// </summary>
    public ulong UnusableResourceCount { get; init; }
}
