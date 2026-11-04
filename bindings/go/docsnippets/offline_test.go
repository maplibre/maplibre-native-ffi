// Snippets for the docs site's Offline page. They compile with the tests,
// which never run them.
package docsnippets

import (
	"context"

	"maplibre.org/maplibre-native-ffi/bindings/go/maplibre"
)

func recordProgress(regionID int64, completed, required uint64, complete bool) {}

func recordError(regionID int64, message string) {}

func recordRegion(regionID int64, metadata []byte) {}

func offline(ctx context.Context) error {
	// #region cache-file
	runtime, err := maplibre.NewRuntime(&maplibre.RuntimeOptions{
		CachePath: "maplibre-cache.db",
	})
	if err != nil {
		return err
	}
	// #endregion cache-file

	// #region progress
	handleEvents := func(events []maplibre.RuntimeEvent) {
		for _, event := range events {
			switch payload := event.Payload.(type) {
			case maplibre.RuntimeEventPayloadOfflineRegionStatusVariant:
				status := payload.Value.Status
				recordProgress(
					payload.Value.RegionID,
					status.CompletedResourceCount,
					status.RequiredResourceCount,
					status.Complete,
				)
			case maplibre.RuntimeEventPayloadOfflineRegionResponseErrorVariant:
				recordError(payload.Value.RegionID, event.Message)
			}
		}
	}
	err = runtime.Discarding().SetEventHandler(
		maplibre.RuntimeEventHandler{Callback: handleEvents},
	)
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

	cleared, err := runtime.RunCacheDatabaseOperation(
		maplibre.CacheDatabaseOperationClear,
	)
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
			StyleURL: "https://demotiles.maplibre.org/style.json",
			Bounds: maplibre.LatLngBounds{
				Southwest: maplibre.LatLng{Latitude: 47.5, Longitude: -122.5},
				Northeast: maplibre.LatLng{Latitude: 47.8, Longitude: -122.2},
			},
			MinZoom:    0,
			MaxZoom:    6,
			PixelRatio: 1,
		},
	}

	created, err := runtime.CreateOfflineRegion(
		definition, []byte("Seattle"),
	)
	if err != nil {
		return err
	}
	region, err := created.Await(ctx)
	if err != nil {
		return err
	}

	activated, err := runtime.SetOfflineRegionDownloadState(
		region.ID, maplibre.OfflineRegionDownloadStateActive,
	)
	if err != nil {
		return err
	}
	if _, err := activated.Await(ctx); err != nil {
		return err
	}
	// #endregion download

	// #region progress
	observed, err := runtime.SetOfflineRegionObserved(region.ID, maplibre.RuntimeEventFlagAllRuntimeEvents)
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
		recordRegion(stored.ID, stored.Metadata)
	}

	// Resume a download that stopped when the runtime was released.
	resumed, err := runtime.SetOfflineRegionDownloadState(
		region.ID, maplibre.OfflineRegionDownloadStateActive,
	)
	if err != nil {
		return err
	}
	if _, err := resumed.Await(ctx); err != nil {
		return err
	}

	invalidated, err := runtime.InvalidateOfflineRegion(region.ID)
	if err != nil {
		return err
	}
	if _, err := invalidated.Await(ctx); err != nil {
		return err
	}

	deleted, err := runtime.DeleteOfflineRegion(region.ID)
	if err != nil {
		return err
	}
	if _, err := deleted.Await(ctx); err != nil {
		return err
	}
	// #endregion manage

	released, err := runtime.Release()
	if err != nil {
		return err
	}
	_, err = released.Await(ctx)
	return err
}

// An example without an output comment compiles but never runs.
func Example_offline() {
	_ = offline(context.Background())
}
