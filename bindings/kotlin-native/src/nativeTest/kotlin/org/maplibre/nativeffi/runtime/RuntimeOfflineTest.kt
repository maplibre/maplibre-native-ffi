package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidStateException

class RuntimeOfflineTest {
  @Test
  fun processGlobalNetworkStatusRoundTrips() {
    val original = RuntimeHandle.networkStatus()
    try {
      RuntimeHandle.setNetworkStatus(NetworkStatus.OFFLINE)
      assertEquals(NetworkStatus.OFFLINE, RuntimeHandle.networkStatus())
      RuntimeHandle.setNetworkStatus(NetworkStatus.ONLINE)
      assertEquals(NetworkStatus.ONLINE, RuntimeHandle.networkStatus())
    } finally {
      RuntimeHandle.setNetworkStatus(original)
    }
  }

  @Test
  fun ambientCacheOperationHandleDiscardsOnce() {
    val runtime = RuntimeHandle.create()
    try {
      val operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.INVALIDATE)
      assertFalse(operation.isClosed())
      assertEquals(OfflineOperationKind.AMBIENT_CACHE, operation.kind)
      assertEquals(OfflineOperationResultKind.NONE, operation.resultKind)

      operation.close()
      assertTrue(operation.isClosed())
      assertFailsWith<InvalidStateException> { operation.close() }
    } finally {
      runtime.close()
    }
  }
}
