package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.runtime.RuntimeHandle

class MapCameraControlsTest {
  @Test
  fun mapCameraAndViewportControlsRoundTripThroughNativeCalls() {
    val runtime = RuntimeHandle.create()
    try {
      val map = MapHandle.create(runtime, MapOptions().size(128, 128).scaleFactor(1.0))
      try {
        map.setDebugOptions(setOf(DebugOption.TILE_BORDERS, DebugOption.COLLISION))
        assertEquals(setOf(DebugOption.TILE_BORDERS, DebugOption.COLLISION), map.debugOptions())

        map.setRenderingStatsViewEnabled(true)
        assertTrue(map.isRenderingStatsViewEnabled())
        map.setRenderingStatsViewEnabled(false)
        assertFalse(map.isRenderingStatsViewEnabled())

        map.setViewportOptions(
          ViewportOptions().viewportMode(ViewportMode.DEFAULT).frustumOffset(EdgeInsets.ZERO)
        )
        val viewport = map.viewportOptions()
        assertTrue(viewport.hasViewportMode())
        assertTrue(viewport.hasFrustumOffset())

        map.setTileOptions(TileOptions().prefetchZoomDelta(1).lodMode(TileLodMode.DEFAULT))
        val tileOptions = map.tileOptions()
        assertTrue(tileOptions.hasPrefetchZoomDelta())
        assertTrue(tileOptions.hasLodMode())

        map.jumpTo(CameraOptions().center(0.0, 0.0).zoom(1.0))
        val camera = map.camera()
        assertTrue(camera.hasCenter())
        assertTrue(camera.hasZoom())
        map.cancelTransitions()
        map.dumpDebugLogs()
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }
}
