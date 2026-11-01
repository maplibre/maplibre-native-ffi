// Snippets for the docs site's Create a map page. They compile with the tests, which never run
// them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.GraphicsDeviceHandle
import org.maplibre.nativeffi.GraphicsDeviceTextureDescriptor
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.MapOptions
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.RuntimeOptions

private fun copyPixels(data: ByteArray, stride: UInt) {}

private suspend fun createAMap() {
  // #region runtime
  val runtime = RuntimeHandle.create(RuntimeOptions())
  // #endregion runtime

  // #region map
  val extent = LogicalExtent(width = 512u, height = 512u, scaleFactor = 1.0)
  val (map, created) =
    runtime.createMap(MapOptions(initialExtent = extent, mapMode = MapMode.STATIC))
  created.await()
  // #endregion map

  // #region style
  map.discarding.setStyleUrl("https://demotiles.maplibre.org/style.json")
  // #endregion style

  // #region attach
  val device = GraphicsDeviceHandle.create()

  val (session, attached) =
    map.attachGraphicsDeviceTexture(
      device,
      GraphicsDeviceTextureDescriptor(extent),
      // The library renders on its own thread.
      RenderSessionAttachOptions(driver = RenderDriverKind.CORE_WORKER),
    )
  attached.await()
  // #endregion attach

  // #region render
  map.requestStillImage().await()
  // #endregion render

  // #region render
  val readback = session.readTexture().await()
  // #endregion render

  // #region render
  copyPixels(readback.data, readback.info.stride)
  // #endregion render

  // #region close
  session.close()
  device.close()
  map.release().await()
  runtime.release().await()
  // #endregion close
}
