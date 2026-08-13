package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.runtime.NetworkStatus

class MaplibreConformanceTest {
  @Test
  fun globalOperationsReachTheNativeLibrary(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      Maplibre.loadNativeLibrary()
      assertEquals(Maplibre.EXPECTED_C_ABI_VERSION, Maplibre.cVersion())
      Maplibre.supportedRenderBackends()
      Maplibre.supportedOpenGLContextProviders()

      val original = Maplibre.networkStatus
      try {
        Maplibre.setNetworkStatus(NetworkStatus.OFFLINE)
        assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus)
        Maplibre.setNetworkStatus(NetworkStatus.ONLINE)
        assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus)
      } finally {
        Maplibre.setNetworkStatus(original)
      }

      val meters = Maplibre.projectedMetersForLatLng(LatLng(0.0, 0.0))
      assertEquals(LatLng(0.0, 0.0), Maplibre.latLngForProjectedMeters(meters))
    }

  @Test
  fun networkStatusRejectsAnUnknownInputBeforeNativeCall(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      assertFailsWith<InvalidArgumentException> { Maplibre.setNetworkStatus(NetworkStatus(999)) }
    }
}
