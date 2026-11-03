// Snippets for the docs site's Render images page. They compile with
// the
// tests, which never run them.

import Foundation
import MaplibreNativeFFI

private func copyPixels(_: Data, stride _: UInt32) {}

private func images(runtime: RuntimeHandle) async throws {
  // #region map
  let created = try runtime.createMap(
    options: MapOptions(
      initialExtent: LogicalExtent(
        width: 800,
        height: 600,
        scaleFactor: 2
      ),
      mapMode: .static
    )
  )
  let map = created.map
  try await created.ready.value
  // #endregion map

  // #region style
  try map.discarding
    .setStyleUrl("https://demotiles.maplibre.org/style.json")
  // #endregion style

  // #region attach
  let attached = try map.attachOffscreen(
    descriptor: OffscreenDescriptor(
      logicalSize: LogicalSize(width: 800, height: 600)
    ),
    // The library renders on its own thread.
    options: RenderSessionAttachOptions(driver: .coreWorker)
  )
  let session = attached.session
  try await attached.ready.value
  // #endregion attach

  // #region image
  try map.discarding.updateCamera(CameraUpdate(
    mode: .jump,
    camera: CameraOptions(
      center: LatLng(latitude: 48.8566, longitude: 2.3522),
      zoom: 4
    )
  ))

  try await map.requestStillImage()
  // #endregion image

  // #region read
  let image = try await session.readTexture()
  copyPixels(image.data, stride: image.info.stride)
  // #endregion read

  try session.dispose()
  try await map.close()
}

private func imagesTile(
  runtime: RuntimeHandle,
  z: UInt32,
  x: UInt32,
  y: UInt32
) async throws {
  // #region tile
  let created = try runtime.createMap(
    options: MapOptions(
      initialExtent: LogicalExtent(
        width: 512,
        height: 512,
        scaleFactor: 1
      ),
      mapMode: .tile
    )
  )
  let map = created.map
  try await created.ready.value

  try map.discarding.updateCamera(CameraUpdate(
    mode: .jump,
    camera: cameraForTile(
      CanonicalTileId(z: z, x: x, y: y),
      logicalSize: LogicalSize(width: 512, height: 512)
    )
  ))
  // #endregion tile

  try await map.close()
}
