package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

class CameraSnapshotAndroidTest {
  @Test
  fun cameraSnapshotsCopyEveryReportedField() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 640
            height = 480
          },
        )
        .use { map ->
          val camera =
            CameraOptions().apply {
              center = LatLng(12.0, 34.0)
              centerAltitude = 123.0
              padding = EdgeInsets(5.0, 10.0, 15.0, 20.0)
              zoom = 4.0
              bearing = 25.0
              pitch = 30.0
              roll = 7.0
              fieldOfView = 40.0
            }
          map.jumpTo(camera)
          val snapshot = map.camera
          assertCamera(camera, snapshot)
          map.createProjection().use { projection ->
            assertCamera(camera, projection.camera)
            map.jumpTo(CameraOptions().apply { zoom = 6.0 })
            assertCamera(camera, snapshot)
            assertCamera(camera, projection.camera)
          }
        }
    }
  }

  private fun assertCamera(expected: CameraOptions, actual: CameraOptions) {
    assertEquals(expected.center!!.latitude, assertNotNull(actual.center).latitude, 1e-6)
    assertEquals(expected.center!!.longitude, assertNotNull(actual.center).longitude, 1e-6)
    assertEquals(expected.centerAltitude!!, assertNotNull(actual.centerAltitude), 1e-6)
    assertEquals(expected.padding, actual.padding)
    assertNull(actual.anchor)
    assertEquals(expected.zoom!!, assertNotNull(actual.zoom), 1e-6)
    assertEquals(expected.bearing!!, assertNotNull(actual.bearing), 1e-6)
    assertEquals(expected.pitch!!, assertNotNull(actual.pitch), 1e-6)
    assertEquals(expected.roll!!, assertNotNull(actual.roll), 1e-6)
    assertEquals(expected.fieldOfView!!, assertNotNull(actual.fieldOfView), 1e-6)
  }
}
