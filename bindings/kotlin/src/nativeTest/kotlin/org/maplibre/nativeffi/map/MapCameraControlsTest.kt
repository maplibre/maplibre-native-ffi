package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.runtime.CameraChangeMode
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import platform.posix.usleep

@OptIn(ExperimentalForeignApi::class)
class MapCameraControlsTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-102, BND-103: camera commands, transitions, viewport state, and projection helpers.

  @Test
  fun mapCameraAndViewportControlsRoundTripThroughNativeCalls() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
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
        val camera = map.camera
        assertEquals(0.0, assertNotNull(camera.center).latitude, 0.000001)
        assertEquals(0.0, assertNotNull(camera.center).longitude, 0.000001)
        assertEquals(1.0, assertNotNull(camera.zoom), 0.000001)
        // BND-070: successive snapshots of an unchanged camera compare equal.
        assertEquals(camera, map.camera)
        map.easeTo(cameraOptions, animation)
        map.flyTo(cameraOptions, animation)
        map.moveBy(0.0, 0.0)
        map.moveByAnimated(0.0, 0.0, animation)
        map.scaleBy(1.0, null)
        map.scaleByAnimated(1.0, null, animation)
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
        map.cancelTransitions()
        map.jumpTo(
          CameraOptions().apply {
            center = LatLng(1.0, 1.0)
            zoom = 2.0
          }
        )
        val cameraAfterCancel = map.camera
        assertEquals(1.0, assertNotNull(cameraAfterCancel.center).latitude, 0.000001)
        assertEquals(1.0, assertNotNull(cameraAfterCancel.center).longitude, 0.000001)
        assertEquals(2.0, assertNotNull(cameraAfterCancel.zoom), 0.000001)
        val fitOptions =
          CameraFitOptions().apply {
            padding = EdgeInsets.ZERO
            bearing = 0.0
            pitch = 0.0
          }
        val bounds = LatLngBounds(LatLng(-10.0, -10.0), LatLng(10.0, 10.0))
        map.cameraForLatLngBounds(bounds, null)
        map.cameraForLatLngBounds(bounds, fitOptions)
        map.cameraForLatLngs(listOf(LatLng(-1.0, -1.0), LatLng(1.0, 1.0)), fitOptions)
        map.cameraForGeometry(Geometry.Point(LatLng(0.0, 0.0)), fitOptions)
        map.latLngBoundsForCamera(cameraOptions)
        map.latLngBoundsForCameraUnwrapped(cameraOptions)
        map.bounds = BoundOptions().apply { this.bounds = BoundsConstraint.Bounded(bounds) }
        assertEquals(BoundsConstraint.Bounded(bounds), map.bounds.bounds)
        map.bounds = BoundOptions().apply { this.bounds = BoundsConstraint.Unbounded }
        assertEquals(BoundsConstraint.Unbounded, map.bounds.bounds)
        map.freeCameraOptions =
          FreeCameraOptions().apply {
            position = Vec3(0.0, 0.0, 0.0)
            orientation = Quaternion(0.0, 0.0, 0.0, 1.0)
          }
        val freeCamera = map.freeCameraOptions
        assertNotNull(freeCamera.position)
        assertNotNull(freeCamera.orientation)
        map.projectionMode = ProjectionModeOptions().apply { axonometric = false }
        assertEquals(false, map.projectionMode.axonometric)
        val projectionCenter = LatLng(37.7749, -122.4194)
        map.jumpTo(
          CameraOptions().apply {
            center = projectionCenter
            zoom = 10.0
          }
        )
        val point = map.pixelForLatLng(projectionCenter)
        val coordinate = map.latLngForPixel(point)
        assertEquals(projectionCenter.latitude, coordinate.latitude, 0.000001)
        assertEquals(projectionCenter.longitude, coordinate.longitude, 0.000001)
        val projectedCoordinates = listOf(projectionCenter, LatLng(0.0, 0.0))
        val points = map.pixelsForLatLngs(projectedCoordinates)
        assertEquals(2, points.size)
        assertTrue(points.all { it.x.isFinite() && it.y.isFinite() })
        val coordinates = map.latLngsForPixels(points)
        assertEquals(2, coordinates.size)
        assertEquals(projectedCoordinates[0].latitude, coordinates[0].latitude, 0.000001)
        assertEquals(projectedCoordinates[0].longitude, coordinates[0].longitude, 0.000001)
        assertTrue(coordinates[1].latitude.isFinite())
        assertTrue(coordinates[1].longitude.isFinite())
        map.createProjection().close()
        map.dumpDebugLogs()
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  // BND-087, BND-102: camera transitions report their identity through runtime events, and
  // camera change events type their code.

  @Test
  fun cameraTransitionIdsReportEveryTerminalOutcomeOnce() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 128
            height = 128
          },
        )
      try {
        // A zero-duration ease resolves inside the call and reports its end right away. An id above
        // Long.MAX_VALUE round-trips as the unsigned bit pattern the caller passed in.
        val instantId = (Long.MAX_VALUE.toULong() + 1UL).toLong()
        map.easeTo(CameraOptions().apply { zoom = 2.0 }, transitionAnimation(instantId, 0.0))
        val instant = drainCameraEvents(runtime)
        assertEquals(listOf(instantId), instant.finishedTransitionIds)
        assertEquals(CameraChangeMode.IMMEDIATE, instant.lastChangeMode)

        // A running transition stays silent until it releases the camera.
        map.easeTo(CameraOptions().apply { zoom = 12.0 }, transitionAnimation(11L, 5_000.0))
        assertEquals(emptyList(), drainCameraEvents(runtime).finishedTransitionIds)

        // A later camera command supersedes it, ending the transition it replaced.
        map.easeTo(CameraOptions().apply { zoom = 13.0 }, transitionAnimation(12L, 5_000.0))
        val superseded = drainCameraEvents(runtime)
        assertEquals(listOf(11L), superseded.finishedTransitionIds)
        assertEquals(CameraChangeMode.ANIMATED, superseded.lastChangeMode)

        // Cancellation ends the superseding transition.
        map.cancelTransitions()
        assertEquals(listOf(12L), drainCameraEvents(runtime).finishedTransitionIds)

        // Omitting the id leaves the transition silent.
        map.easeTo(
          CameraOptions().apply { zoom = 14.0 },
          AnimationOptions().apply { durationMs = 0.0 },
        )
        assertEquals(emptyList(), drainCameraEvents(runtime).finishedTransitionIds)
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun completedCameraTransitionReportsItsIdOnceAndReachesTheRequestedCamera() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 128
            height = 128
            mapMode = MapMode.STATIC
          },
        )
      try {
        map.easeTo(CameraOptions().apply { zoom = 5.0 }, transitionAnimation(21L, 5_000.0))
        // A still-image request runs a static map's pending transitions to their end.
        map.requestStillImage()

        val finished = mutableListOf<Long>()
        var rounds = 0
        while (finished.isEmpty() && rounds < 10_000) {
          runtime.runOnce()
          finished += drainCameraEvents(runtime).finishedTransitionIds
          rounds++
          usleep(1_000U)
        }
        assertEquals(listOf(21L), finished)
        assertEquals(5.0, assertNotNull(map.camera.zoom), 0.000001)

        // The completed transition reports its end once; later pumping adds nothing.
        repeat(100) {
          runtime.runOnce()
          finished += drainCameraEvents(runtime).finishedTransitionIds
        }
        assertEquals(listOf(21L), finished)
      } finally {
        map.close()
      }
    } finally {
      runtime.close()
    }
  }

  private fun transitionAnimation(transitionId: Long, durationMs: Double): AnimationOptions =
    AnimationOptions().apply {
      this.transitionId = transitionId
      this.durationMs = durationMs
    }

  private class CameraEvents(
    val finishedTransitionIds: List<Long>,
    val lastChangeMode: CameraChangeMode?,
  )

  private fun drainCameraEvents(runtime: RuntimeHandle): CameraEvents {
    val finished = mutableListOf<Long>()
    var lastChangeMode: CameraChangeMode? = null
    while (true) {
      val event = runtime.pollEvent() ?: return CameraEvents(finished, lastChangeMode)
      when (event.type) {
        RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED ->
          finished +=
            assertIs<RuntimeEventPayload.CameraTransitionFinished>(event.payload).transitionId
        RuntimeEventType.MAP_CAMERA_DID_CHANGE -> lastChangeMode = CameraChangeMode(event.code)
      }
    }
  }
}
