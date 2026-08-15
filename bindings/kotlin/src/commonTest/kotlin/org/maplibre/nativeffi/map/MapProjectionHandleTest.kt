package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.runtime.RuntimeHandle

class MapProjectionHandleTest {
  // BND-043, BND-103.

  @Test
  fun projectionOwnsStandaloneSnapshotAndClosesIndependently(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
            scaleFactor = 1.0
          },
        )
      val projection = map.createProjection()

      assertFalse(projection.isClosed)
      // Every projection call is synchronous: a setter is applied before it returns, so the
      // next read or conversion observes it.
      projection.setCamera(
        CameraOptions().apply {
          center = LatLng(0.0, 0.0)
          zoom = 2.0
        }
      )
      val camera = projection.camera()
      assertNotNull(camera.center)
      assertEquals(2.0, camera.zoom)
      val zoomedIn = projection.pixelForLatLng(LatLng(10.0, 10.0))
      projection.setVisibleCoordinates(
        listOf(LatLng(-60.0, -170.0), LatLng(60.0, 170.0)),
        EdgeInsets.ZERO,
      )
      val zoomedOut = projection.pixelForLatLng(LatLng(10.0, 10.0))
      assertFalse(zoomedIn == zoomedOut, "a setter changes later conversions")
      projection.setVisibleGeometry(
        "{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]}".encodeToByteArray(),
        EdgeInsets.ZERO,
      )
      val point = projection.pixelForLatLng(LatLng(0.0, 0.0))
      val coordinate = projection.latLngForPixel(point)
      assertEquals(0.0, coordinate.latitude, 0.000001)
      assertEquals(0.0, coordinate.longitude, 0.000001)
      val meters = Maplibre.projectedMetersForLatLng(LatLng(0.0, 0.0))
      val projectedCoordinate = Maplibre.latLngForProjectedMeters(meters)
      assertEquals(0.0, projectedCoordinate.latitude, 0.000001)
      assertEquals(0.0, projectedCoordinate.longitude, 0.000001)

      // A projection is usable from another thread.
      val offThreadRoundTrip =
        withContext(Dispatchers.Default) {
          projection.latLngForPixel(projection.pixelForLatLng(LatLng(0.0, 0.0)))
        }
      assertEquals(0.0, offThreadRoundTrip.latitude, 0.000001)
      assertEquals(0.0, offThreadRoundTrip.longitude, 0.000001)

      // A live projection prevents map close.
      assertFailsWith<InvalidStateException> {
        org.maplibre.nativeffi.runtime.runSuspendTest { map.close() }
      }
      assertNotNull(projection.camera().zoom)
      projection.close()
      map.close()
      runtime.close()

      assertTrue(projection.isClosed)
      projection.close()
      assertFailsWith<InvalidStateException> { projection.camera() }
    }

  @Test
  fun projectionObservesMapCommandsAcceptedBeforeCreation(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
      val map =
        MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
            scaleFactor = 1.0
          },
        )
      try {
        map.updateCamera(
          CameraUpdate(
            camera =
              CameraOptions().apply {
                center = LatLng(30.0, 40.0)
                zoom = 5.0
              }
          )
        )

        // Creation copies the map transform after every earlier map command.
        val projection = map.createProjection()
        try {
          assertEquals(5.0, projection.camera().zoom)
          assertEquals(30.0, assertNotNull(projection.camera().center).latitude, 0.000001)

          // A later map camera command is never observed by the existing projection.
          map.updateCamera(CameraUpdate(camera = CameraOptions().apply { zoom = 9.0 }))
          map.queryCamera()
          assertEquals(5.0, projection.camera().zoom)

          // The commands accepted before this call reached the map: the center maps back to
          // itself through the projection's own frozen transform.
          val center = projection.latLngForPixel(ScreenPoint(32.0, 32.0))
          assertEquals(30.0, center.latitude, 0.5)
          assertEquals(40.0, center.longitude, 0.5)
        } finally {
          projection.close()
        }
      } finally {
        map.close()
        runtime.close()
      }
    }
}
