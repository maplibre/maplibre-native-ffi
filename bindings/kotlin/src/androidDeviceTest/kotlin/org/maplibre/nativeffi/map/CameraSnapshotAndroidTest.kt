package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.EdgeInsets
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use

class CameraSnapshotAndroidTest {
  @Test
  fun cameraSnapshotsCopyEveryReportedField(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 640u, height = 480u)
            )
        )
        .await()
        .use { map ->
          val camera =
            CameraOptions(
              center = LatLng(12.0, 34.0),
              centerAltitude = 123.0,
              padding = EdgeInsets(5.0, 10.0, 15.0, 20.0),
              zoom = 4.0,
              bearing = 25.0,
              pitch = 30.0,
              roll = 7.0,
              fieldOfView = 40.0,
            )
          map.updateCamera(CameraUpdate(camera = camera)).awaitCommitted()
          val snapshot = map.cameraSnapshotGet().camera
          assertCamera(camera, snapshot)
          map.projectionCreate().await().use { projection ->
            assertCamera(camera, projection.getCamera())
            map.updateCamera(CameraUpdate(camera = CameraOptions(zoom = 6.0))).awaitCommitted()
            assertCamera(camera, snapshot)
            assertCamera(camera, projection.getCamera())
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
