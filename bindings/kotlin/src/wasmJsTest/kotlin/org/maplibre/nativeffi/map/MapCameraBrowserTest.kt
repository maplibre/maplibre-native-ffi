package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertIs
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.BoundsConstraint
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.pumpUntil
import org.maplibre.nativeffi.runtime.CameraChangeMode
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.withMap
import org.maplibre.nativeffi.withRuntime

/**
 * The camera, its transitions, and the projections that read from it.
 *
 * Camera state is the densest descriptor traffic in the API: optional fields with their own field
 * masks, nested insets, and arrays of coordinates that go out and come back. A round trip is the
 * only thing that catches a field written at the wrong offset, because native accepts the
 * descriptor either way.
 */
class MapCameraBrowserTest {
  // Spec coverage: BND-043, BND-060, BND-061, BND-070, BND-087, BND-102, BND-103.

  @Test
  fun cameraAndViewportControlsRoundTripThroughTheOwnerThread() {
    withMap { _, map ->
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
      assertEquals(ViewportMode.DEFAULT, map.viewportOptions.viewportMode)
      assertEquals(EdgeInsets.ZERO, map.viewportOptions.frustumOffset)

      map.tileOptions =
        TileOptions().apply {
          prefetchZoomDelta = 1
          lodMode = TileLodMode.DEFAULT
        }
      assertEquals(1, map.tileOptions.prefetchZoomDelta)
      assertEquals(TileLodMode.DEFAULT, map.tileOptions.lodMode)

      val camera =
        CameraOptions().apply {
          center = LatLng(0.0, 0.0)
          zoom = 1.0
        }
      val animation = AnimationOptions().apply { durationMs = 0.0 }

      map.jumpTo(camera)
      val snapshot = map.camera
      assertEquals(0.0, assertNotNull(snapshot.center).latitude, TOLERANCE)
      assertEquals(1.0, assertNotNull(snapshot.zoom), TOLERANCE)
      // Two reads of an unchanged camera compare equal, so the snapshot is a value rather than a
      // handle onto whatever the map holds now.
      assertEquals(snapshot, map.camera)

      map.easeTo(camera, animation)
      map.flyTo(camera, animation)
      map.moveBy(0.0, 0.0)
      map.moveByAnimated(0.0, 0.0, animation)
      map.scaleBy(1.0, null)
      map.scaleByAnimated(1.0, null, animation)
      map.rotateBy(ScreenPoint(0.0, 0.0), ScreenPoint(0.0, 0.0))
      map.rotateByAnimated(ScreenPoint(0.0, 0.0), ScreenPoint(0.0, 0.0), animation)
      map.pitchBy(0.0)
      map.pitchByAnimated(0.0, animation)
      map.cancelTransitions()

      // A gesture brackets the camera changes inside it.
      assertFalse(map.isGestureInProgress)
      map.isGestureInProgress = true
      map.moveBy(8.0, -4.0)
      assertTrue(map.isGestureInProgress)
      map.isGestureInProgress = false
      assertFalse(map.isGestureInProgress)

      map.jumpTo(
        CameraOptions().apply {
          center = LatLng(1.0, 1.0)
          zoom = 2.0
        }
      )
      assertEquals(1.0, assertNotNull(map.camera.center).latitude, TOLERANCE)
      assertEquals(2.0, assertNotNull(map.camera.zoom), TOLERANCE)

      val fitOptions =
        CameraFitOptions().apply {
          padding = EdgeInsets.ZERO
          bearing = 0.0
          pitch = 0.0
        }
      val bounds = LatLngBounds(LatLng(-10.0, -10.0), LatLng(10.0, 10.0))
      // A null options descriptor and a present one take different branches through the same
      // entry point.
      map.cameraForLatLngBounds(bounds, null)
      map.cameraForLatLngBounds(bounds, fitOptions)
      map.cameraForLatLngs(listOf(LatLng(-1.0, -1.0), LatLng(1.0, 1.0)), fitOptions)
      map.cameraForGeometry(Geometry.Point(LatLng(0.0, 0.0)), fitOptions)
      map.latLngBoundsForCamera(camera)
      map.latLngBoundsForCameraUnwrapped(camera)

      // A constraint is a sum type over the same descriptor, so both cases have to read back.
      map.bounds = BoundOptions().apply { this.bounds = BoundsConstraint.Bounded(bounds) }
      assertEquals(BoundsConstraint.Bounded(bounds), map.bounds.bounds)
      map.bounds = BoundOptions().apply { this.bounds = BoundsConstraint.Unbounded }
      assertEquals(BoundsConstraint.Unbounded, map.bounds.bounds)

      map.freeCameraOptions =
        FreeCameraOptions().apply {
          position = Vec3(0.0, 0.0, 0.0)
          orientation = Quaternion(0.0, 0.0, 0.0, 1.0)
        }
      assertNotNull(map.freeCameraOptions.position)
      assertNotNull(map.freeCameraOptions.orientation)

      map.projectionMode = ProjectionModeOptions().apply { axonometric = false }
      assertEquals(false, map.projectionMode.axonometric)

      map.dumpDebugLogs()
    }
  }

  @Test
  fun projectionHelpersRoundTripSingleAndBatchedCoordinates() {
    withMap { _, map ->
      val centre = LatLng(37.7749, -122.4194)
      map.jumpTo(
        CameraOptions().apply {
          center = centre
          zoom = 10.0
        }
      )

      val point = map.pixelForLatLng(centre)
      val returned = map.latLngForPixel(point)
      assertEquals(centre.latitude, returned.latitude, TOLERANCE)
      assertEquals(centre.longitude, returned.longitude, TOLERANCE)

      // The batched form writes an array out and reads an array back, which is a different
      // descriptor shape from the single one above.
      val coordinates = listOf(centre, LatLng(0.0, 0.0))
      val points = map.pixelsForLatLngs(coordinates)
      assertEquals(2, points.size)
      assertTrue(points.all { it.x.isFinite() && it.y.isFinite() })
      val returnedAll = map.latLngsForPixels(points)
      assertEquals(2, returnedAll.size)
      assertEquals(coordinates[0].latitude, returnedAll[0].latitude, TOLERANCE)
      assertEquals(coordinates[0].longitude, returnedAll[0].longitude, TOLERANCE)

      // A projection handle is a standalone snapshot: it keeps working after the map it came
      // from has gone.
      val projection = map.createProjection()
      try {
        projection.setCamera(
          CameraOptions().apply {
            center = LatLng(0.0, 0.0)
            zoom = 2.0
          }
        )
        assertNotNull(projection.camera.center)
        projection.setVisibleCoordinates(
          listOf(LatLng(0.0, 0.0), LatLng(1.0, 1.0)),
          EdgeInsets.ZERO,
        )
        projection.setVisibleGeometry(
          Geometry.LineString(listOf(LatLng(0.0, 0.0), LatLng(1.0, 1.0))),
          EdgeInsets.ZERO,
        )
        val projected = projection.latLngForPixel(projection.pixelForLatLng(LatLng(0.0, 0.0)))
        assertEquals(0.0, projected.latitude, TOLERANCE)
        assertEquals(0.0, projected.longitude, TOLERANCE)
      } finally {
        projection.close()
      }
    }
  }

  @Test
  fun aProjectionOutlivesTheMapItWasTakenFrom() {
    withRuntime { runtime ->
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 128
            height = 128
          },
        )
      map.jumpTo(
        CameraOptions().apply {
          center = LatLng(10.0, 20.0)
          zoom = 4.0
        }
      )
      val projection = map.createProjection()

      // A snapshot rather than a view: closing the map does not take the projection with it, and
      // it is not a child that holds the map open either.
      map.close()
      assertTrue(map.isClosed)
      assertFalse(projection.isClosed)

      assertEquals(4.0, assertNotNull(projection.camera.zoom), TOLERANCE)
      val returned = projection.latLngForPixel(projection.pixelForLatLng(LatLng(10.0, 20.0)))
      assertEquals(10.0, returned.latitude, TOLERANCE)
      assertEquals(20.0, returned.longitude, TOLERANCE)

      projection.close()
      assertTrue(projection.isClosed)
      projection.close()
      assertFailsWith<InvalidStateException> { projection.pixelForLatLng(LatLng(0.0, 0.0)) }
    }
  }

  @Test
  fun aTransitionReportsItsOwnIdOnceThroughTheEventQueue() {
    withRuntime { runtime ->
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 128
            height = 128
          },
        )
      try {
        // A zero-duration ease resolves inside the call and reports its end right away. An id
        // above Long.MAX_VALUE round-trips as the unsigned bit pattern the caller passed in,
        // which is what says the payload is read at its own width.
        val instantId = (Long.MAX_VALUE.toULong() + 1UL).toLong()
        map.easeTo(CameraOptions().apply { zoom = 2.0 }, transition(instantId, 0.0))
        val instant = drainCameraEvents(runtime)
        assertEquals(listOf(instantId), instant.finished)
        assertEquals(CameraChangeMode.IMMEDIATE, instant.lastChangeMode)

        // A running transition stays silent until it releases the camera.
        map.easeTo(CameraOptions().apply { zoom = 12.0 }, transition(11L, 5_000.0))
        assertEquals(emptyList(), drainCameraEvents(runtime).finished)

        // A later camera command supersedes it, ending the transition it replaced.
        map.easeTo(CameraOptions().apply { zoom = 13.0 }, transition(12L, 5_000.0))
        val superseded = drainCameraEvents(runtime)
        assertEquals(listOf(11L), superseded.finished)
        assertEquals(CameraChangeMode.ANIMATED, superseded.lastChangeMode)

        // Cancellation ends the superseding transition.
        map.cancelTransitions()
        assertEquals(listOf(12L), drainCameraEvents(runtime).finished)

        // Omitting the id leaves the transition silent.
        map.easeTo(
          CameraOptions().apply { zoom = 14.0 },
          AnimationOptions().apply { durationMs = 0.0 },
        )
        assertEquals(emptyList(), drainCameraEvents(runtime).finished)
      } finally {
        map.close()
      }
    }
  }

  @Test
  fun aCompletedTransitionReachesItsCameraAndReportsItsIdOnce() {
    withRuntime { runtime ->
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
        map.easeTo(CameraOptions().apply { zoom = 5.0 }, transition(21L, 5_000.0))
        // A still-image request runs a static map's pending transitions to their end.
        map.requestStillImage()

        val finished = mutableListOf<Long>()
        pumpUntil(
          runtime,
          onEvent = {
            if (it.type == RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED) {
              finished +=
                assertIs<RuntimeEventPayload.CameraTransitionFinished>(it.payload).transitionId
            }
          },
        ) {
          finished.isNotEmpty()
        }

        assertEquals(listOf(21L), finished)
        assertEquals(5.0, assertNotNull(map.camera.zoom), TOLERANCE)

        // The completed transition reports its end once; later pumping adds nothing.
        repeat(50) {
          runtime.pump(1)
          while (true) {
            val event = runtime.pollEvent() ?: break
            if (event.type == RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED) {
              finished +=
                assertIs<RuntimeEventPayload.CameraTransitionFinished>(event.payload).transitionId
            }
          }
        }
        assertEquals(listOf(21L), finished)
      } finally {
        map.close()
      }
    }
  }

  private fun transition(transitionId: Long, durationMs: Double): AnimationOptions =
    AnimationOptions().apply {
      this.transitionId = transitionId
      this.durationMs = durationMs
    }

  private class CameraEvents(val finished: List<Long>, val lastChangeMode: CameraChangeMode?)

  private fun drainCameraEvents(runtime: RuntimeHandle): CameraEvents {
    val finished = mutableListOf<Long>()
    var lastChangeMode: CameraChangeMode? = null
    runtime.pump(0)
    while (true) {
      val event = runtime.pollEvent() ?: return CameraEvents(finished, lastChangeMode)
      when (event.type) {
        RuntimeEventType.MAP_CAMERA_TRANSITION_FINISHED ->
          finished +=
            assertIs<RuntimeEventPayload.CameraTransitionFinished>(event.payload).transitionId
        RuntimeEventType.MAP_CAMERA_DID_CHANGE -> lastChangeMode = CameraChangeMode(event.code)
        else -> Unit
      }
    }
  }

  private companion object {
    /** Both directions are double precision, so a round trip loses far less than this. */
    const val TOLERANCE = 1e-6
  }
}
