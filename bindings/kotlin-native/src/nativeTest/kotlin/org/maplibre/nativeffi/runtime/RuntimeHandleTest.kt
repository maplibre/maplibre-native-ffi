package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.offline.OfflineRegionInfo
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceTransformCallback

class RuntimeHandleTest {
  @Test
  fun closeReleasesRuntimeOnceAndInvalidatesWrapper() {
    val runtime = RuntimeHandle.create()

    assertFalse(runtime.isClosed)
    runtime.runOnce()
    runtime.pollEvent()
    runtime.close()

    assertTrue(runtime.isClosed)
    runtime.close()
    assertFailsWith<InvalidStateException> { runtime.runOnce() }
  }

  @Test
  fun resourceProviderAndTransformReplacementPathsRetainAndClearCallbackState() {
    val runtime = RuntimeHandle.create()
    try {
      runtime.setResourceTransform(ResourceTransformCallback { request -> request.url })
      runtime.setResourceTransform(ResourceTransformCallback { null })
      runtime.clearResourceTransform()
      runtime.clearResourceTransform()

      runtime.setResourceProvider(
        ResourceProviderCallback { _, _ -> ResourceProviderDecision.PASS_THROUGH }
      )
      runtime.setResourceProvider(
        ResourceProviderCallback { _, _ -> ResourceProviderDecision.PASS_THROUGH }
      )
    } finally {
      runtime.close()
    }
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

  @Test
  fun consumedOfflineOperationCloseIsNoOp() {
    val runtime = RuntimeHandle.create()
    val operation =
      OfflineOperationHandle<Unit>(
        runtime,
        1UL,
        OfflineOperationKind.AMBIENT_CACHE,
        OfflineOperationResultKind.NONE,
      )
    try {
      operation.markConsumed()
      operation.close()
      operation.close()
      assertTrue(operation.isClosed)
    } finally {
      runtime.close()
    }
  }

  @Test
  fun closingOfflineOperationAfterRuntimeCloseConsumesHandle() {
    val runtime = RuntimeHandle.create()
    val operation =
      OfflineOperationHandle<Unit>(
        runtime,
        1UL,
        OfflineOperationKind.AMBIENT_CACHE,
        OfflineOperationResultKind.NONE,
      )

    runtime.close()

    assertFailsWith<InvalidStateException> { operation.close() }
    assertTrue(operation.isClosed)
    operation.close()
  }
}
