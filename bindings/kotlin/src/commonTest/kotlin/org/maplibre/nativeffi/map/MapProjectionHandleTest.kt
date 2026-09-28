package org.maplibre.nativeffi.map

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.EdgeInsets
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.ScreenPoint
import org.maplibre.nativeffi.runOnBackgroundThread
import org.maplibre.nativeffi.runtime.runSuspendTest

@OptIn(ExperimentalAtomicApi::class)
class MapProjectionHandleTest {
  @Test
  fun projectionOwnsStandaloneSnapshotAndClosesIndependently(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault()
                  .initialExtent
                  .copy(width = 64u, height = 64u, scaleFactor = 1.0)
            )
        )
        .await()
    val projection = map.projectionCreate().await()

    assertFalse(projection.isClosed)
    // Every projection call is synchronous: a setter is applied before it returns, so the
    // next read or conversion observes it.
    projection.setCamera(CameraOptions(center = LatLng(0.0, 0.0), zoom = 2.0))
    val camera = projection.getCamera()
    assertNotNull(camera.center)
    assertEquals(2.0, camera.zoom)
    val zoomedIn = projection.pixelForLatLng(LatLng(10.0, 10.0))
    projection.setVisibleCoordinates(
      listOf(LatLng(-60.0, -170.0), LatLng(60.0, 170.0)),
      EdgeInsets(),
    )
    val zoomedOut = projection.pixelForLatLng(LatLng(10.0, 10.0))
    assertFalse(zoomedIn == zoomedOut, "a setter changes later conversions")
    projection.setVisibleGeometry(
      "{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]}".encodeToByteArray(),
      EdgeInsets(),
    )
    val point = projection.pixelForLatLng(LatLng(0.0, 0.0))
    val coordinate = projection.latLngForPixel(point)
    assertEquals(0.0, coordinate.latitude, 0.000001)
    assertEquals(0.0, coordinate.longitude, 0.000001)
    val meters = GeneratedApi.projectedMetersForLatLng(LatLng(0.0, 0.0))
    val projectedCoordinate = GeneratedApi.latLngForProjectedMeters(meters)
    assertEquals(0.0, projectedCoordinate.latitude, 0.000001)
    assertEquals(0.0, projectedCoordinate.longitude, 0.000001)

    // A projection is usable from another thread.
    val offThreadRoundTrip =
      withContext(Dispatchers.Default) {
        projection.latLngForPixel(projection.pixelForLatLng(LatLng(0.0, 0.0)))
      }
    assertEquals(0.0, offThreadRoundTrip.latitude, 0.000001)
    assertEquals(0.0, offThreadRoundTrip.longitude, 0.000001)

    map.release().await()
    runtime.release().await()
    // The projection owns its snapshot independently of both source handles.
    assertNotNull(projection.getCamera().zoom)
    projection.close()

    assertTrue(projection.isClosed)
    projection.close()
    assertFailsWith<org.maplibre.nativeffi.error.InvalidStateException> { projection.getCamera() }
  }

  @Test
  fun projectionObservesMapCommandsAcceptedBeforeCreation(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault()
                  .initialExtent
                  .copy(width = 64u, height = 64u, scaleFactor = 1.0)
            )
        )
        .await()
    try {
      map
        .updateCamera(CameraUpdate(camera = CameraOptions(center = LatLng(30.0, 40.0), zoom = 5.0)))
        .await()

      // Creation copies the map transform after every earlier map command.
      val projection = map.projectionCreate().await()
      try {
        assertEquals(5.0, projection.getCamera().zoom)
        assertEquals(30.0, assertNotNull(projection.getCamera().center).latitude, 0.000001)

        // A later map camera command is never observed by the existing projection.
        map.updateCamera(CameraUpdate(camera = CameraOptions(zoom = 9.0))).await()
        map.cameraQuery().await()
        assertEquals(5.0, projection.getCamera().zoom)

        // The commands accepted before this call reached the map: the center maps back to
        // itself through the projection's own frozen transform.
        val center = projection.latLngForPixel(ScreenPoint(32.0, 32.0))
        assertEquals(30.0, center.latitude, 0.5)
        assertEquals(40.0, center.longitude, 0.5)
      } finally {
        projection.close()
      }
    } finally {
      map.release().await()
      runtime.release().await()
    }
  }

  @Test
  fun projectionRemainsUsableOnAnotherThreadAfterMapClose(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault()
                  .initialExtent
                  .copy(width = 64u, height = 64u, scaleFactor = 1.0)
            )
        )
        .await()
    val projection = map.projectionCreate().await()
    map.release().await()
    runtime.release().await()
    val failure = AtomicReference<Throwable?>(null)

    // A projection stays usable, and closable, on a thread that never touched the map.
    runOnBackgroundThread {
      try {
        projection.getCamera()
        projection.close()
      } catch (error: Throwable) {
        failure.store(error)
      }
    }

    failure.load()?.let { throw it }
  }
}
