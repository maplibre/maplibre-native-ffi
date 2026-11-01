// Snippets for the docs site's Offline page. They compile with the tests,
// which never run them.

using System.Collections.Immutable;

namespace Maplibre.NativeFfi.Tests.DocSnippets;

internal static class Offline
{
    private static void RecordProgress(
        long regionId,
        ulong completed,
        ulong required,
        bool complete
    ) { }

    private static void RecordError(long regionId, string message) { }

    private static void RecordRegion(long regionId, ImmutableArray<byte> metadata) { }

    // #region progress
    // Runs on a library thread.
    private static void OnEvents(RuntimeEvent[] events)
    {
        foreach (var e in events)
        {
            switch (e.Payload)
            {
                case RuntimeEvent.PayloadValue.OfflineRegionStatus { Value: var progress }:
                    RecordProgress(
                        progress.RegionId,
                        progress.Status.CompletedResourceCount,
                        progress.Status.RequiredResourceCount,
                        progress.Status.Complete
                    );
                    break;
                case RuntimeEvent.PayloadValue.OfflineRegionResponseError { Value: var error }:
                    RecordError(error.RegionId, e.Message);
                    break;
            }
        }
    }

    // #endregion progress

    internal static async Task Run()
    {
        // #region cache-file
        var runtime = RuntimeHandle.Create(new RuntimeOptions { CachePath = "maplibre-cache.db" });
        // #endregion cache-file

        // #region progress
        runtime.Discarding.SetEventHandler(new RuntimeEventHandler(OnEvents));
        // #endregion progress

        // #region ambient
        // 50 MiB
        await runtime.SetMaximumAmbientCacheSizeAsync(50 * 1024 * 1024);
        await runtime.RunAmbientCacheOperationAsync(AmbientCacheOperation.Clear);
        // #endregion ambient

        // #region download
        var definition = new OfflineRegionDefinition.TilePyramid(
            new OfflineTilePyramidRegionDefinition
            {
                StyleUrl = "https://demotiles.maplibre.org/style.json",
                Bounds = new LatLngBounds(new LatLng(47.5, -122.5), new LatLng(47.8, -122.2)),
                MinZoom = 0,
                MaxZoom = 6,
                PixelRatio = 1,
            }
        );
        var region = await runtime.CreateOfflineRegionAsync(definition, "Seattle"u8);

        await runtime.SetOfflineRegionDownloadStateAsync(
            region.Id,
            OfflineRegionDownloadState.Active
        );
        // #endregion download

        // #region progress
        await runtime.SetOfflineRegionObservedAsync(region.Id, true);
        // #endregion progress

        // #region manage
        foreach (var stored in await runtime.ListOfflineRegionsAsync())
        {
            RecordRegion(stored.Id, stored.Metadata);
        }

        // Resume a download that stopped when the runtime closed.
        await runtime.SetOfflineRegionDownloadStateAsync(
            region.Id,
            OfflineRegionDownloadState.Active
        );

        await runtime.InvalidateOfflineRegionAsync(region.Id);

        await runtime.DeleteOfflineRegionAsync(region.Id);
        // #endregion manage

        await runtime.CloseAsync();
    }
}
