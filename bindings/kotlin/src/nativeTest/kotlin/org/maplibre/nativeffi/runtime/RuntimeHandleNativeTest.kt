package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions

@OptIn(ExperimentalForeignApi::class)
class RuntimeHandleNativeTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun abiMismatchPreventsNativeRuntimeCreation(): Unit = runSuspendTest {
    var creates = 0
    assertFailsWith<AbiVersionMismatchException> {
      RuntimeHandle.createForTesting(
        actualAbiVersion = Maplibre.EXPECTED_C_ABI_VERSION + 1L,
        creator = { _, _ ->
          creates += 1
          MaplibreStatus.OK.nativeCode
        },
      )
    }
    assertEquals(0, creates)
  }

  @Test
  fun lifecycleAndOrderedQueryResumeWithoutThreadAffinity(): Unit = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    assertFalse(runtime.isClosed)
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 128
          height = 128
        },
      )
    val snapshot = map.snapshot()
    runtime.barrier()
    assertTrue(map.queryCamera().generation >= snapshot.generation)
    map.close()
    assertTrue(map.isClosed)
    runtime.close()
    assertTrue(runtime.isClosed)
  }
}
