package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.Geometry
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
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 128
            height = 128
            scaleFactor = 1.0
          },
        )
      try {
        map.debugOptions = setOf(DebugOption.TILE_BORDERS, DebugOption.COLLISION)
        assertEquals(setOf(DebugOption.TILE_BORDERS, DebugOption.COLLISION), map.debugOptions)

        map.isRenderingStatsViewEnabled = true
        assertTrue(map.isRenderingStatsViewEnabled)
        map.isRenderingStatsViewEnabled = false
        assertFalse(map.isRenderingStatsViewEnabled)

        map.viewportOptions =
          ViewportOptions().apply {
            viewportMode = ViewportMode.DEFAULT
            frustumOffset = EdgeInsets.ZERO
          }
        val viewport = map.viewportOptions
        assertNotNull(viewport.viewportMode)
        assertNotNull(viewport.frustumOffset)

        map.tileOptions =
          TileOptions().apply {
            prefetchZoomDelta = 1
            lodMode = TileLodMode.DEFAULT
          }
        val tileOptions = map.tileOptions
        assertNotNull(tileOptions.prefetchZoomDelta)
        assertNotNull(tileOptions.lodMode)

        val cameraOptions =
          CameraOptions().apply {
            center = LatLng(0.0, 0.0)
            zoom = 1.0
          }
        val animation = AnimationOptions().apply { durationMs = 0.0 }
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
        val camera = map.camera
        assertNotNull(camera.center)
        assertNotNull(camera.zoom)
        map.cancelTransitions()
        val fitOptions =
          CameraFitOptions().apply {
            padding = EdgeInsets.ZERO
            bearing = 0.0
            pitch = 0.0
          }
        val bounds = LatLngBounds(LatLng(-10.0, -10.0), LatLng(10.0, 10.0))
        map.cameraForLatLngBounds(bounds)
        map.cameraForLatLngBounds(bounds, fitOptions)
        map.cameraForLatLngs(listOf(LatLng(-1.0, -1.0), LatLng(1.0, 1.0)), fitOptions)
        map.cameraForGeometry(Geometry.point(LatLng(0.0, 0.0)), fitOptions)
        map.latLngBoundsForCamera(cameraOptions)
        map.latLngBoundsForCameraUnwrapped(cameraOptions)
        map.bounds = BoundOptions().apply { this.bounds = bounds }
        assertNotNull(map.bounds.bounds)
        map.freeCameraOptions =
          FreeCameraOptions().apply {
            position = Vec3(0.0, 0.0, 0.0)
            orientation = Quaternion(0.0, 0.0, 0.0, 1.0)
          }
        val freeCamera = map.freeCameraOptions
        assertNotNull(freeCamera.position)
        assertNotNull(freeCamera.orientation)
        map.projectionMode = ProjectionModeOptions().apply { axonometric = false }
        assertNotNull(map.projectionMode.axonometric)
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
