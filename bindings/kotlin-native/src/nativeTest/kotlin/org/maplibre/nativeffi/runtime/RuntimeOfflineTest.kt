package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState

class RuntimeOfflineTest {
  @Test
  fun processGlobalNetworkStatusRoundTrips() {
    val original = Maplibre.networkStatus
    try {
      Maplibre.networkStatus = NetworkStatus.OFFLINE
      assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus)
      Maplibre.networkStatus = NetworkStatus.ONLINE
      assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus)
    } finally {
      Maplibre.networkStatus = original
    }
  }

  @Test
  fun offlineDownloadStateUnknownSentinelUsesNativeValidation() {
    val runtime = RuntimeHandle.create()
    try {
      assertFailsWith<InvalidArgumentException> {
        runtime.startSetOfflineRegionDownloadState(1, OfflineRegionDownloadState.UNKNOWN)
      }
    } finally {
      runtime.close()
    }
  }

  @Test
  fun ambientCacheOperationHandleDiscardsOnce() {
    val runtime = RuntimeHandle.create()
    try {
      val operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.INVALIDATE)
      assertFalse(operation.isClosed)
      assertEquals(OfflineOperationKind.AMBIENT_CACHE, operation.kind)
      assertEquals(OfflineOperationResultKind.NONE, operation.resultKind)

      operation.close()
      assertTrue(operation.isClosed)
      operation.close()
      assertTrue(operation.isClosed)
    } finally {
      runtime.close()
    }
  }
}
