// Snippets for the docs site's Offline page. They are type-checked with the
// tests, which never run them.

import {
  AmbientCacheOperation,
  OfflineRegionDownloadState,
  RuntimeHandle,
} from "../../src/index.ts";

function recordProgress(
  _regionId: bigint,
  _completed: bigint,
  _required: bigint,
  _complete: boolean,
): void {}

function recordError(_regionId: bigint, _message: string): void {}

function recordRegion(_regionId: bigint, _metadata: Uint8Array): void {}

export async function offline(): Promise<void> {
  // #region cache-file
  const runtime = await RuntimeHandle.create({
    cachePath: "maplibre-cache.db",
  });
  // #endregion cache-file

  // #region progress
  runtime.discarding.setEventHandler((events) => {
    for (const { payload, message } of events) {
      if (payload.kind === "offlineRegionStatus") {
        const { regionId, status } = payload.value;
        recordProgress(
          regionId,
          status.completedResourceCount,
          status.requiredResourceCount,
          status.complete,
        );
      } else if (payload.kind === "offlineRegionResponseError") {
        recordError(payload.value.regionId, message);
      }
    }
  });
  // #endregion progress

  // #region ambient
  // 50 MiB
  await runtime.setMaximumAmbientCacheSize(50n * 1024n * 1024n);
  await runtime.runAmbientCacheOperation(AmbientCacheOperation.clear);
  // #endregion ambient

  // #region download
  const region = await runtime.createOfflineRegion(
    {
      kind: "tilePyramid",
      value: {
        styleUrl: "https://demotiles.maplibre.org/style.json",
        bounds: {
          southwest: { latitude: 47.5, longitude: -122.5 },
          northeast: { latitude: 47.8, longitude: -122.2 },
        },
        minZoom: 0,
        maxZoom: 6,
        pixelRatio: 1,
      },
    },
    new TextEncoder().encode("Seattle"),
  );

  await runtime.setOfflineRegionDownloadState(
    region.id,
    OfflineRegionDownloadState.active,
  );
  // #endregion download

  // #region progress
  await runtime.setOfflineRegionObserved(region.id, true);
  // #endregion progress

  // #region manage
  for (const stored of await runtime.listOfflineRegions()) {
    recordRegion(stored.id, stored.metadata);
  }

  // Resume a download that stopped when the runtime closed.
  await runtime.setOfflineRegionDownloadState(
    region.id,
    OfflineRegionDownloadState.active,
  );

  await runtime.invalidateOfflineRegion(region.id);

  await runtime.deleteOfflineRegion(region.id);
  // #endregion manage

  await runtime.close();
}
