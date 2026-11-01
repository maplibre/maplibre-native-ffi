// Snippets for the docs site's Offline page. They are analyzed with the tests,
// which never run them.

import 'dart:convert';
import 'dart:typed_data';

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void recordProgress(int regionId, int completed, int required, bool complete) {}

void recordError(int regionId, String message) {}

void recordRegion(int regionId, Uint8List metadata) {}

Future<void> offline() async {
  // #region cache-file
  final runtime = RuntimeHandle.create(
    const RuntimeOptions(cachePath: 'maplibre-cache.db'),
  );
  // #endregion cache-file

  // #region progress
  runtime.discarding.setEventHandler((events) {
    for (final RuntimeEvent(:payload, :message) in events) {
      switch (payload) {
        case RuntimeEventPayloadOfflineRegionStatus(:final value):
          final RuntimeEventOfflineRegionStatus(:regionId, :status) = value;
          recordProgress(
            regionId,
            status.completedResourceCount,
            status.requiredResourceCount,
            status.complete,
          );
        case RuntimeEventPayloadOfflineRegionResponseError(:final value):
          recordError(value.regionId, message);
        default:
      }
    }
  });
  // #endregion progress

  // #region ambient
  // 50 MiB
  await runtime.setMaximumAmbientCacheSize(50 * 1024 * 1024);
  await runtime.runAmbientCacheOperation(AmbientCacheOperation.clear);
  // #endregion ambient

  // #region download
  final region = await runtime.createOfflineRegion(
    const OfflineRegionDefinition.tilePyramid(
      OfflineTilePyramidRegionDefinition(
        styleUrl: 'https://demotiles.maplibre.org/style.json',
        bounds: LatLngBounds(
          southwest: LatLng(latitude: 47.5, longitude: -122.5),
          northeast: LatLng(latitude: 47.8, longitude: -122.2),
        ),
        minZoom: 0,
        maxZoom: 6,
        pixelRatio: 1,
      ),
    ),
    utf8.encode('Seattle'),
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
  for (final stored in await runtime.listOfflineRegions()) {
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
