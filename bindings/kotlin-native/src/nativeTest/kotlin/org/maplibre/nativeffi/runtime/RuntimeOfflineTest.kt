package org.maplibre.nativeffi.runtime

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.offline.OfflineRegionDownloadState

class RuntimeOfflineTest {
  @Test
  fun processGlobalNetworkStatusRoundTrips() {
    val original = Maplibre.networkStatus()
    try {
      Maplibre.setNetworkStatus(NetworkStatus.OFFLINE)
      assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus())
      Maplibre.setNetworkStatus(NetworkStatus.ONLINE)
      assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus())
    } finally {
      Maplibre.setNetworkStatus(original)
    }
  }

  @Test
  fun offlineDownloadStateRejectsUnknownSentinel() {
    val runtime = RuntimeHandle.create()
    try {
      assertFailsWith<IllegalArgumentException> {
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
