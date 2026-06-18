package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.runtime.RuntimeHandle

@OptIn(ExperimentalForeignApi::class)
class MapProjectionHandleTest {
  // BND-043, BND-103: projection handles are independent snapshots with projection helpers.

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
        Geometry.LineString(listOf(LatLng(0.0, 0.0), LatLng(1.0, 1.0))),
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
      assertFailsWith<InvalidStateException> { projection.nativeHandle() }
    } finally {
      runtime.close()
    }
  }
}
