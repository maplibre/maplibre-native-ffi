package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint

@OptIn(ExperimentalForeignApi::class)
class MapStructsTest {
  @Test
  fun cameraOptionsRoundTripOnlyPresentFields() {
    val camera =
      CameraOptions().apply {
        center = LatLng(12.0, 34.0)
        zoom = 5.0
        padding = EdgeInsets(1.0, 2.0, 3.0, 4.0)
        anchor = ScreenPoint(8.0, 9.0)
      }

    val copy = memScoped {
      MapStructs.cameraOptions(MapStructs.cameraOptions(camera, this).pointed)
    }

    assertNotNull(copy.center)
    assertEquals(12.0, copy.center?.latitude)
    assertEquals(34.0, copy.center?.longitude)
    assertNotNull(copy.zoom)
    assertEquals(5.0, copy.zoom)
    assertNotNull(copy.padding)
    assertEquals(EdgeInsets(1.0, 2.0, 3.0, 4.0), copy.padding)
    assertNotNull(copy.anchor)
    assertEquals(ScreenPoint(8.0, 9.0), copy.anchor)
    assertFalse(copy.bearing != null)
  }
}
