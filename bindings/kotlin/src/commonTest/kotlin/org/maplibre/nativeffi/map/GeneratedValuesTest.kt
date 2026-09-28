package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.generated.BoundOptions
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.runtime.awaitCommitted
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use

class GeneratedValuesTest {
  @Test
  fun copiedArraysAndPresenceGroupsSurviveAdmission(): Unit = runSuspendTest {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 256u, height = 256u)
            )
        )
        .await()
        .use { map ->
          map.setBounds(BoundOptions(unbounded = true)).awaitCommitted()
          map
            .updateCamera(
              CameraUpdate(camera = CameraOptions(center = LatLng(10.0, 20.0), zoom = 3.0))
            )
            .awaitCommitted()
          val snapshot = map.snapshotGet()
          assertTrue(snapshot.bounds.unbounded)
          assertEquals(3.0, snapshot.camera.zoom)
          assertEquals(10.0, snapshot.camera.center!!.latitude, 1e-8)
          assertEquals(20.0, snapshot.camera.center!!.longitude, 1e-8)
          val coordinates = mutableListOf(LatLng(10.0, 20.0), LatLng(12.0, 21.0))
          val pending = map.pixelsForLatLngs(coordinates)
          coordinates.clear()
          val copied = pending.await()
          assertEquals(2, copied.size)
          val roundTrip = map.latLngsForPixels(copied).await()
          assertEquals(10.0, roundTrip[0].latitude, 1e-8)
          assertEquals(21.0, roundTrip[1].longitude, 1e-8)
          assertTrue(map.pixelsForLatLngs(emptyList()).await().isEmpty())
        }
    }
  }
}
