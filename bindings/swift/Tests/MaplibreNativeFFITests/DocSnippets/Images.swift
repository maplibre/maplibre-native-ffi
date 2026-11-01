// Snippets for the docs site's Render images page. They compile with the
// tests, which never run them.

import Foundation
import MaplibreNativeFFI

/// The center of tile z/x/y. The snippet leaves the tile math to the reader.
private func tileCenter(_: UInt32, _: UInt32, _: UInt32) -> LatLng {
  LatLng()
}

private func copyPixels(_: Data, stride _: UInt32) {}

private func images(
  runtime: RuntimeHandle,
  z: UInt32,
  x: UInt32,
  y: UInt32
) async throws {
  // #region map
  let extent = LogicalExtent(width: 512, height: 512, scaleFactor: 1)
  let created = try runtime.createMap(
    options: MapOptions(initialExtent: extent, mapMode: .tile)
  )
  let map = created.map
  try await created.ready.value
  // #endregion map

  // #region style
  try map.discarding.setStyleUrl("https://demotiles.maplibre.org/style.json")
  // #endregion style

  // #region attach
  let device = try GraphicsDeviceHandle.create()

  let attached = try map.attachGraphicsDeviceTexture(
    device,
    descriptor: GraphicsDeviceTextureDescriptor(extent: extent),
    // The library renders on its own thread.
    options: RenderSessionAttachOptions(driver: .coreWorker)
  )
  let session = attached.session
  try await attached.ready.value
  // #endregion attach

  // #region image
  try map.discarding.updateCamera(CameraUpdate(
    mode: .jump,
    camera: CameraOptions(center: tileCenter(z, x, y), zoom: Double(z))
  ))

  try await map.requestStillImage()
  // #endregion image

  // #region read
  let image = try await session.readTexture()
  copyPixels(image.data, stride: image.info.stride)
  // #endregion read

  // #region close
  try session.dispose()
  device.close()
  try await map.close()
  // #endregion close
}
