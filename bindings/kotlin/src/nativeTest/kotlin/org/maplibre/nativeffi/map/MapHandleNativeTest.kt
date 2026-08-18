package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.runSuspendTest

@OptIn(ExperimentalForeignApi::class)
class MapHandleNativeTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun mapCreationOptionsMaterializeExtentScaleAndMode(): Unit = runSuspendTest {
    MapHandle.mapOptionsForTesting(
      MapOptions().apply {
        width = 320
        height = 240
        scaleFactor = 2.0
        mapMode = MapMode.STATIC
        fastPforEnabled = true
      }
    ) { native ->
      assertEquals(320U, native.initial_extent.width)
      assertEquals(240U, native.initial_extent.height)
      assertEquals(2.0, native.initial_extent.scale_factor)
      assertEquals(MapMode.STATIC.nativeValue.toUInt(), native.map_mode)
      assertTrue(native.fast_pfor_enabled)
    }
  }

  @Test
  fun commandAndSnapshotProgressAcrossCoroutineResumption(): Unit = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
        .await()
    val command =
      map.updateCamera(CameraUpdate(camera = CameraOptions().apply { zoom = 2.0 })).await()
    assertTrue(command.generation > 0uL)
    runtime.barrier().await()
    assertEquals(2.0, map.queryCamera().await().camera.zoom)
    map.close()
    runtime.close()
    assertTrue(map.isClosed)
    assertFalse(runtime.isClosed.not())
  }
}
