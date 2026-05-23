package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.offline.OfflineRegionInfo

class RuntimeHandleTest {
  @Test
  fun closeReleasesRuntimeOnceAndInvalidatesWrapper() {
    val runtime = RuntimeHandle.create()

    assertFalse(runtime.isClosed())
    runtime.runOnce()
    runtime.pollEvent()
    runtime.close()

    assertTrue(runtime.isClosed())
    runtime.close()
    assertFailsWith<InvalidStateException> { runtime.runOnce() }
  }

  @Test
  fun offlineOperationTakeMethodsValidateExpectedOperationKindBeforeNativeCall() {
    val runtime = RuntimeHandle.create()
    val operation =
      OfflineOperationHandle<OfflineRegionInfo>(
        runtime,
        1UL,
        OfflineOperationKind.AMBIENT_CACHE,
        OfflineOperationResultKind.NONE,
      )
    try {
      assertFailsWith<InvalidStateException> { runtime.takeCreateOfflineRegionResult(operation) }
    } finally {
      operation.markConsumed()
      runtime.close()
    }
  }
}
