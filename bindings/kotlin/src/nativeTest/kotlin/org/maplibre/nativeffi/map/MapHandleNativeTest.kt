package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.generated.CameraOptions
import org.maplibre.nativeffi.generated.CameraUpdate
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.runtime.runSuspendTest

@OptIn(ExperimentalForeignApi::class)
class MapHandleNativeTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun commandAndSnapshotProgressAcrossCoroutineResumption(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
        )
        .await()
    val command = map.updateCamera(CameraUpdate(camera = CameraOptions(zoom = 2.0))).await()
    assertTrue(command.generation > 0uL)
    runtime.barrier().await()
    assertEquals(2.0, map.cameraQuery().await().camera.zoom)
    map.close().await()
    runtime.close().await()
    assertTrue(map.isClosed)
    assertTrue(runtime.isClosed)
  }
}
