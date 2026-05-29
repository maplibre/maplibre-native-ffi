package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.runtime.RuntimeHandle

@OptIn(ExperimentalForeignApi::class)
class MapProjectionHandleTest {
  @Test
  fun projectionOwnsStandaloneSnapshotAndClosesIndependently() {
    val runtime = RuntimeHandle.create()
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
      val projection = MapProjectionHandle.create(map)

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
        Geometry.lineString(listOf(LatLng(0.0, 0.0), LatLng(1.0, 1.0))),
        EdgeInsets.ZERO,
      )
      val point = projection.pixelForLatLng(LatLng(0.0, 0.0))
      projection.latLngForPixel(point)
      val meters = MapProjectionHandle.projectedMetersForLatLng(LatLng(0.0, 0.0))
      MapProjectionHandle.latLngForProjectedMeters(meters)
      map.close()
      projection.close()

      assertTrue(projection.isClosed)
      projection.close()
      assertFailsWith<InvalidStateException> { projection.nativeHandle() }
    } finally {
      runtime.close()
    }
  }
}
