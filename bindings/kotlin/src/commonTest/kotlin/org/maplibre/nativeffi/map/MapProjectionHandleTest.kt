package org.maplibre.nativeffi.map

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.runOnBackgroundThread
import org.maplibre.nativeffi.runtime.RuntimeHandle

@OptIn(ExperimentalAtomicApi::class)
class MapProjectionHandleTest {
  // BND-043, BND-103.

  @Test
  fun projectionOwnsStandaloneSnapshotAndClosesIndependently() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    try {
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
      projection.setCamera(
        CameraOptions().apply {
          center = LatLng(0.0, 0.0)
          zoom = 2.0
        }
      )
      val camera = projection.camera
      kotlin.test.assertNotNull(camera.center)
      kotlin.test.assertNotNull(camera.zoom)
      projection.setVisibleCoordinates(listOf(LatLng(0.0, 0.0), LatLng(1.0, 1.0)), EdgeInsets.ZERO)
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

      map.close()
      kotlin.test.assertNotNull(projection.camera.zoom)
      val pointAfterMapClose = projection.pixelForLatLng(LatLng(0.0, 0.0))
      val coordinateAfterMapClose = projection.latLngForPixel(pointAfterMapClose)
      assertEquals(0.0, coordinateAfterMapClose.latitude, 0.000001)
      assertEquals(0.0, coordinateAfterMapClose.longitude, 0.000001)
      projection.close()

      assertTrue(projection.isClosed)
      projection.close()
      assertFailsWith<InvalidStateException> { projection.camera }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun projectionRemainsUsableOnAnotherThreadAfterMapClose() {
    val runtime = RuntimeHandle.create(org.maplibre.nativeffi.runtime.RuntimeOptions())
    val map = MapHandle.create(runtime, MapOptions())
    val projection = map.createProjection()
    map.close()
    runtime.close()
    val failure = AtomicReference<Throwable?>(null)

    runOnBackgroundThread {
      try {
        projection.camera
        projection.close()
      } catch (error: Throwable) {
        failure.store(error)
      }
    }

    assertNull(failure.load())
  }
}
