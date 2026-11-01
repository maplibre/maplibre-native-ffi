// Snippets for the docs site's Render images page. They compile with the tests, which never run
// them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.CameraOptions
import org.maplibre.nativeffi.CameraUpdate
import org.maplibre.nativeffi.CameraUpdateMode
import org.maplibre.nativeffi.GraphicsDeviceHandle
import org.maplibre.nativeffi.GraphicsDeviceTextureDescriptor
import org.maplibre.nativeffi.LatLng
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.MapOptions
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RuntimeHandle

// The center of tile z/x/y. The snippet leaves the tile math to the reader.
private fun tileCenter(z: Int, x: Int, y: Int): LatLng = LatLng()

private fun copyPixels(data: ByteArray, stride: UInt) {}

private suspend fun images(runtime: RuntimeHandle, z: Int, x: Int, y: Int) {
  // #region map
  val extent = LogicalExtent(width = 512u, height = 512u, scaleFactor = 1.0)
  val (map, mapReady) =
    runtime.createMap(MapOptions(initialExtent = extent, mapMode = MapMode.TILE))
  mapReady.await()
  // #endregion map

  // #region style
  map.discarding.setStyleUrl("https://demotiles.maplibre.org/style.json")
  // #endregion style

  // #region attach
  val device = GraphicsDeviceHandle.create()

  val (session, sessionReady) =
    map.attachGraphicsDeviceTexture(
      device,
      GraphicsDeviceTextureDescriptor(extent),
      // The library renders on its own thread.
      RenderSessionAttachOptions(driver = RenderDriverKind.CORE_WORKER),
    )
  sessionReady.await()
  // #endregion attach

  // #region image
  map.discarding.updateCamera(
    CameraUpdate(
      mode = CameraUpdateMode.JUMP,
      camera = CameraOptions(center = tileCenter(z, x, y), zoom = z.toDouble()),
    ),
  )

  map.requestStillImage().await()
  // #endregion image

  // #region read
  val image = session.readTexture().await()
  copyPixels(image.data, image.info.stride)
  // #endregion read

  // #region close
  session.close()
  device.close()
  map.release().await()
  // #endregion close
}
