// Snippets for the docs site's Render images page. They compile with
// the tests, which never run
// them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.CameraOptions
import org.maplibre.nativeffi.CameraUpdate
import org.maplibre.nativeffi.CameraUpdateMode
import org.maplibre.nativeffi.LatLng
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.MapOptions
import org.maplibre.nativeffi.OffscreenDescriptor
import org.maplibre.nativeffi.RenderDriverKind
import org.maplibre.nativeffi.RenderSessionAttachOptions
import org.maplibre.nativeffi.RuntimeHandle

// The center of tile z/x/y. The snippet leaves the tile math to the
// reader.
private fun tileCenter(z: Int, x: Int, y: Int): LatLng = LatLng()

private fun copyPixels(data: ByteArray, stride: UInt) {}

private suspend fun images(runtime: RuntimeHandle) {
  // #region map
  val extent =
    LogicalExtent(width = 800u, height = 600u, scaleFactor = 2.0)
  val (map, mapReady) =
    runtime.createMap(
      MapOptions(initialExtent = extent, mapMode = MapMode.STATIC),
    )
  mapReady.await()
  // #endregion map

  // #region style
  map.discarding.setStyleUrl(
    "https://demotiles.maplibre.org/style.json",
  )
  // #endregion style

  // #region attach
  val (session, sessionReady) =
    map.attachOffscreen(
      OffscreenDescriptor(extent),
      // The library renders on its own thread.
      RenderSessionAttachOptions(
        driver = RenderDriverKind.CORE_WORKER,
      ),
    )
  sessionReady.await()
  // #endregion attach

  // #region image
  map.discarding.updateCamera(
    CameraUpdate(
      mode = CameraUpdateMode.JUMP,
      camera =
        CameraOptions(
          center = LatLng(latitude = 48.8566, longitude = 2.3522),
          zoom = 4.0,
        ),
    ),
  )

  map.requestStillImage().await()
  // #endregion image

  // #region read
  val image = session.readTexture().await()
  copyPixels(image.data, image.info.stride)
  // #endregion read

  session.close()
  map.release().await()
}

private suspend fun imagesTile(
  runtime: RuntimeHandle,
  z: Int,
  x: Int,
  y: Int,
) {
  // #region tile
  val (map, ready) =
    runtime.createMap(
      MapOptions(
        initialExtent =
          LogicalExtent(
            width = 512u,
            height = 512u,
            scaleFactor = 1.0,
          ),
        mapMode = MapMode.TILE,
      ),
    )
  ready.await()

  map.discarding.updateCamera(
    CameraUpdate(
      mode = CameraUpdateMode.JUMP,
      camera =
        CameraOptions(
          center = tileCenter(z, x, y),
          // A 512-pixel map shows tile z/x/y at zoom z.
          zoom = z.toDouble(),
        ),
    ),
  )
  // #endregion tile

  map.release().await()
}
