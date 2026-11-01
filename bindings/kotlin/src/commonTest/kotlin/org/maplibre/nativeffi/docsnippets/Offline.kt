// Snippets for the docs site's Offline page. They compile with the tests, which never run them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.AmbientCacheOperation
import org.maplibre.nativeffi.LatLng
import org.maplibre.nativeffi.LatLngBounds
import org.maplibre.nativeffi.OfflineRegionDefinition
import org.maplibre.nativeffi.OfflineRegionDownloadState
import org.maplibre.nativeffi.OfflineTilePyramidRegionDefinition
import org.maplibre.nativeffi.RuntimeEventHandler
import org.maplibre.nativeffi.RuntimeEventPayload
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.RuntimeOptions

private fun recordProgress(regionId: Long, completed: ULong, required: ULong, complete: Boolean) {}

private fun recordError(regionId: Long, message: String) {}

private fun recordRegion(regionId: Long, metadata: ByteArray) {}

private suspend fun offline() {
  // #region cache-file
  val runtime = RuntimeHandle.create(RuntimeOptions(cachePath = "maplibre-cache.db"))
  // #endregion cache-file

  // #region progress
  runtime.discarding.setEventHandler(
    RuntimeEventHandler { events ->
      for (event in events) {
        when (val payload = event.payload) {
          is RuntimeEventPayload.OfflineRegionStatus -> {
            val (regionId, status) = payload.value
            recordProgress(
              regionId,
              status.completedResourceCount,
              status.requiredResourceCount,
              status.complete,
            )
          }
          is RuntimeEventPayload.OfflineRegionResponseError ->
            recordError(payload.value.regionId, event.message)
          else -> {}
        }
      }
    },
  )
  // #endregion progress

  // #region ambient
  // 50 MiB
  runtime.setMaximumAmbientCacheSize(50uL * 1024uL * 1024uL).await()
  runtime.runAmbientCacheOperation(AmbientCacheOperation.CLEAR).await()
  // #endregion ambient

  // #region download
  val region =
    runtime
      .createOfflineRegion(
        OfflineRegionDefinition.TilePyramid(
          OfflineTilePyramidRegionDefinition(
            styleUrl = "https://demotiles.maplibre.org/style.json",
            bounds =
              LatLngBounds(
                southwest = LatLng(latitude = 47.5, longitude = -122.5),
                northeast = LatLng(latitude = 47.8, longitude = -122.2),
              ),
            minZoom = 0.0,
            maxZoom = 6.0,
            pixelRatio = 1f,
          ),
        ),
        "Seattle".encodeToByteArray(),
      )
      .await()

  runtime.setOfflineRegionDownloadState(region.id, OfflineRegionDownloadState.ACTIVE).await()
  // #endregion download

  // #region progress
  runtime.setOfflineRegionObserved(region.id, true).await()
  // #endregion progress

  // #region manage
  for (stored in runtime.listOfflineRegions().await()) {
    recordRegion(stored.id, stored.metadata)
  }

  // Resume a download that stopped when the runtime closed.
  runtime.setOfflineRegionDownloadState(region.id, OfflineRegionDownloadState.ACTIVE).await()

  runtime.invalidateOfflineRegion(region.id).await()

  runtime.deleteOfflineRegion(region.id).await()
  // #endregion manage

  runtime.release().await()
}
