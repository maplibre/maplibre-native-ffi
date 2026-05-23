package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.Vec3
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

        val cameraOptions = CameraOptions().center(0.0, 0.0).zoom(1.0)
        val animation = AnimationOptions().durationMillis(0.0)
        map.jumpTo(cameraOptions)
        map.easeTo(cameraOptions, animation)
        map.flyTo(cameraOptions, animation)
        map.moveBy(0.0, 0.0)
        map.moveByAnimated(0.0, 0.0, animation)
        map.scaleBy(1.0)
        map.scaleByAnimated(1.0, animation)
        map.rotateBy(
          org.maplibre.nativeffi.geo.ScreenPoint(0.0, 0.0),
          org.maplibre.nativeffi.geo.ScreenPoint(0.0, 0.0),
        )
        map.rotateByAnimated(
          org.maplibre.nativeffi.geo.ScreenPoint(0.0, 0.0),
          org.maplibre.nativeffi.geo.ScreenPoint(0.0, 0.0),
          animation,
        )
        map.pitchBy(0.0)
        map.pitchByAnimated(0.0, animation)
        val camera = map.camera()
        assertTrue(camera.hasCenter())
        assertTrue(camera.hasZoom())
        map.cancelTransitions()
        map.setBounds(BoundOptions().bounds(LatLngBounds(LatLng(-10.0, -10.0), LatLng(10.0, 10.0))))
        assertTrue(map.bounds().hasBounds())
        map.setFreeCameraOptions(
          FreeCameraOptions()
            .position(Vec3(0.0, 0.0, 0.0))
            .orientation(Quaternion(0.0, 0.0, 0.0, 1.0))
        )
        val freeCamera = map.freeCameraOptions()
        assertTrue(freeCamera.hasPosition())
        assertTrue(freeCamera.hasOrientation())
        map.setProjectionMode(ProjectionModeOptions().axonometric(false))
        assertTrue(map.projectionMode().hasAxonometric())
        val point = map.pixelForLatLng(LatLng(0.0, 0.0))
        map.latLngForPixel(point)
        assertEquals(2, map.pixelsForLatLngs(listOf(LatLng(0.0, 0.0), LatLng(1.0, 1.0))).size)
        assertEquals(2, map.latLngsForPixels(listOf(point, point)).size)
        map.createProjection().close()
        map.dumpDebugLogs()
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }
}
