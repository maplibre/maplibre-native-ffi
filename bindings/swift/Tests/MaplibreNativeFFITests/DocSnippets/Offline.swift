// Snippets for the docs site's Offline page. They compile with the tests,
// which never run them.

import Foundation
import MaplibreNativeFFI

private func recordProgress(
  _: Int64,
  completed _: UInt64,
  required _: UInt64,
  complete _: Bool
) {}

private func recordError(_: Int64, _: String) {}

private func recordRegion(_: Int64, _: Data) {}

private func offline() async throws {
  // #region cache-file
  let runtime = try RuntimeHandle.create(
    options: RuntimeOptions(cachePath: "maplibre-cache.db")
  )
  // #endregion cache-file

  // #region progress
  try runtime.discarding.setEventHandler(RuntimeEventHandler { events in
    for event in events {
      switch event.payload {
      case let .offlineRegionStatus(payload):
        recordProgress(
          payload.regionId,
          completed: payload.status.completedResourceCount,
          required: payload.status.requiredResourceCount,
          complete: payload.status.complete
        )
      case let .offlineRegionResponseError(payload):
        recordError(payload.regionId, event.message)
      default:
        break
      }
    }
  })
  // #endregion progress

  // #region ambient
  // 50 MiB
  try await runtime.setMaximumAmbientCacheSize(50 * 1024 * 1024)
  try await runtime.runAmbientCacheOperation(.clear)
  // #endregion ambient

  // #region download
  let region = try await runtime.createOfflineRegion(
    definition: .tilePyramid(OfflineTilePyramidRegionDefinition(
      styleUrl: "https://demotiles.maplibre.org/style.json",
      bounds: LatLngBounds(
        southwest: LatLng(latitude: 47.5, longitude: -122.5),
        northeast: LatLng(latitude: 47.8, longitude: -122.2)
      ),
      minZoom: 0,
      maxZoom: 6,
      pixelRatio: 1
    )),
    metadata: Data("Seattle".utf8)
  )

  try await runtime.setOfflineRegionDownloadState(region.id, state: .active)
  // #endregion download

  // #region progress
  try await runtime.setOfflineRegionObserved(region.id, observed: true)
  // #endregion progress

  // #region manage
  for stored in try await runtime.listOfflineRegions() {
    recordRegion(stored.id, stored.metadata)
  }

  // Resume a download that stopped when the runtime closed.
  try await runtime.setOfflineRegionDownloadState(region.id, state: .active)

  try await runtime.invalidateOfflineRegion(region.id)

  try await runtime.deleteOfflineRegion(region.id)
  // #endregion manage

  try await runtime.close()
}
