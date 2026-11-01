// Snippets for the docs site's Create a map page. They compile with the
// tests, which never run them.

import Foundation
import MaplibreNativeFFI

private func copyPixels(_: Data, stride _: UInt32) {}

private func createAMap() async throws {
  // #region runtime
  let runtime = try RuntimeHandle.create(options: RuntimeOptions())
  // #endregion runtime

  // #region map
  let extent = LogicalExtent(width: 512, height: 512, scaleFactor: 1)
  let created = try runtime.createMap(
    options: MapOptions(initialExtent: extent, mapMode: .static)
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

  // #region render
  try await map.requestStillImage()
  // #endregion render

  // #region render
  let readback = try await session.readTexture()
  // #endregion render

  // #region render
  copyPixels(readback.data, stride: readback.info.stride)
  // #endregion render

  // #region close
  try session.dispose()
  device.close()
  try await map.close()
  try await runtime.close()
  // #endregion close
}
