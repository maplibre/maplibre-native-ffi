// Snippets for the docs site's Offline page. They compile with the tests,
// which never run them.
package docsnippets

import (
	"context"

	"github.com/maplibre/maplibre-native-ffi/bindings/go/maplibre"
)

func recordProgress(regionID int64, completed, required uint64, complete bool) {}

func recordError(regionID int64, message string) {}

func recordRegion(regionID int64, metadata []byte) {}

func offline(ctx context.Context) error {
	// #region cache-file
	runtime, err := maplibre.NewRuntime(maplibre.RuntimeOptions{CachePath: "maplibre-cache.db"})
	if err != nil {
		return err
	}
	// #endregion cache-file

	// #region progress
	err = runtime.Discarding().SetEventHandler(maplibre.RuntimeEventHandler{
		Callback: func(events []maplibre.RuntimeEvent) {
			for _, event := range events {
				switch payload := event.Payload.(type) {
				case maplibre.RuntimeEventPayloadOfflineRegionStatusVariant:
					status := payload.Value.Status
					recordProgress(
						payload.Value.RegionId,
						status.CompletedResourceCount,
						status.RequiredResourceCount,
						status.Complete,
					)
				case maplibre.RuntimeEventPayloadOfflineRegionResponseErrorVariant:
					recordError(payload.Value.RegionId, event.Message)
				}
			}
		},
	})
	if err != nil {
		return err
	}
	// #endregion progress

	// #region ambient
	// 50 MiB
	resized, err := runtime.SetMaximumAmbientCacheSize(50 * 1024 * 1024)
	if err != nil {
		return err
	}
	if _, err := resized.Await(ctx); err != nil {
		return err
	}

	cleared, err := runtime.RunAmbientCacheOperation(maplibre.AmbientCacheOperationClear)
	if err != nil {
		return err
	}
	if _, err := cleared.Await(ctx); err != nil {
		return err
	}
	// #endregion ambient

	// #region download
	definition := maplibre.OfflineRegionDefinitionTilePyramidVariant{
		Value: maplibre.OfflineTilePyramidRegionDefinition{
			StyleUrl: "https://demotiles.maplibre.org/style.json",
			Bounds: maplibre.LatLngBounds{
				Southwest: maplibre.LatLng{Latitude: 47.5, Longitude: -122.5},
				Northeast: maplibre.LatLng{Latitude: 47.8, Longitude: -122.2},
			},
			MinZoom:    0,
			MaxZoom:    6,
			PixelRatio: 1,
		},
	}

	created, err := runtime.CreateOfflineRegion(definition, []byte("Seattle"))
	if err != nil {
		return err
	}
	region, err := created.Await(ctx)
	if err != nil {
		return err
	}

	activated, err := runtime.SetOfflineRegionDownloadState(region.Id, maplibre.OfflineRegionDownloadStateActive)
	if err != nil {
		return err
	}
	if _, err := activated.Await(ctx); err != nil {
		return err
	}
	// #endregion download

	// #region progress
	observed, err := runtime.SetOfflineRegionObserved(region.Id, true)
	if err != nil {
		return err
	}
	if _, err := observed.Await(ctx); err != nil {
		return err
	}
	// #endregion progress

	// #region manage
	listed, err := runtime.ListOfflineRegions()
	if err != nil {
		return err
	}
	regions, err := listed.Await(ctx)
	if err != nil {
		return err
	}
	for _, stored := range regions {
		recordRegion(stored.Id, stored.Metadata)
	}

	// Resume a download that stopped when the runtime closed.
	resumed, err := runtime.SetOfflineRegionDownloadState(region.Id, maplibre.OfflineRegionDownloadStateActive)
	if err != nil {
		return err
	}
	if _, err := resumed.Await(ctx); err != nil {
		return err
	}

	invalidated, err := runtime.InvalidateOfflineRegion(region.Id)
	if err != nil {
		return err
	}
	if _, err := invalidated.Await(ctx); err != nil {
		return err
	}

	deleted, err := runtime.DeleteOfflineRegion(region.Id)
	if err != nil {
		return err
	}
	if _, err := deleted.Await(ctx); err != nil {
		return err
	}
	// #endregion manage

	closed, err := runtime.Close()
	if err != nil {
		return err
	}
	_, err = closed.Await(ctx)
	return err
}

// An example without an output comment compiles but never runs.
func Example_offline() {
	_ = offline(context.Background())
}
