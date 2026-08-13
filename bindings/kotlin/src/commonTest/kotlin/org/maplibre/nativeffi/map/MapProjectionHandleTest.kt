package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.EdgeInsets
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.runtime.CommandDisposition
import org.maplibre.nativeffi.runtime.RuntimeEventPayload
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
      val cameraCommandId =
        projection
          .setCamera(
            CameraOptions().apply {
              center = LatLng(0.0, 0.0)
              zoom = 2.0
            }
          )
          .toULong()
      assertCommandCommitted(runtime, cameraCommandId)
      val camera = projection.camera()
      kotlin.test.assertNotNull(camera.center)
      kotlin.test.assertNotNull(camera.zoom)
      val coordinatesCommandId =
        projection
          .setVisibleCoordinates(listOf(LatLng(0.0, 0.0), LatLng(1.0, 1.0)), EdgeInsets.ZERO)
          .toULong()
      assertCommandCommitted(runtime, coordinatesCommandId)
      val geometryCommandId =
        projection
          .setVisibleGeometry(
            "{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]}".encodeToByteArray(),
            EdgeInsets.ZERO,
          )
          .toULong()
      assertCommandCommitted(runtime, geometryCommandId)
      assertTrue(cameraCommandId < coordinatesCommandId)
      assertTrue(coordinatesCommandId < geometryCommandId)
      val point = projection.pixelForLatLng(LatLng(0.0, 0.0))
      val coordinate = projection.latLngForPixel(point)
      assertEquals(0.0, coordinate.latitude, 0.000001)
      assertEquals(0.0, coordinate.longitude, 0.000001)
      val meters = Maplibre.projectedMetersForLatLng(LatLng(0.0, 0.0))
      val projectedCoordinate = Maplibre.latLngForProjectedMeters(meters)
      assertEquals(0.0, projectedCoordinate.latitude, 0.000001)
      assertEquals(0.0, projectedCoordinate.longitude, 0.000001)

      assertFailsWith<InvalidStateException> {
        org.maplibre.nativeffi.runtime.runSuspendTest { map.close() }
      }
      kotlin.test.assertNotNull(projection.camera().zoom)
      projection.close()
      map.close()
      runtime.close()

      assertTrue(projection.isClosed)
      projection.close()
    }

  private suspend fun assertCommandCommitted(runtime: RuntimeHandle, commandId: ULong) {
    runtime.barrier()
    val matches =
      runtime
        .drainEvents()
        .events
        .mapNotNull { it.payload as? RuntimeEventPayload.CommandFinished }
        .filter { it.commandId == commandId }
    assertEquals(1, matches.size, "terminal outcome count for command $commandId")
    assertEquals(CommandDisposition.COMMITTED, matches.single().disposition)
  }
}
