package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.generated.GeneratedApi

@OptIn(ExperimentalForeignApi::class)
class RuntimeHandleNativeTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun abiMismatchPreventsNativeRuntimeCreation(): Unit = runSuspendTest {
    assertFailsWith<AbiVersionMismatchException> {
      Maplibre.checkCompatibleCAbi(Maplibre.EXPECTED_C_ABI_VERSION + 1L)
    }
  }

  @Test
  fun closingAMapAndItsRuntimeRetiresBothWrappers(): Unit = runSuspendTest {
    val runtime = GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault())
    assertFalse(runtime.isClosed)
    val map =
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 128u, height = 128u)
            )
        )
        .await()
    map.close().await()
    assertTrue(map.isClosed)
    runtime.close().await()
    assertTrue(runtime.isClosed)
  }
}
