package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.runtime.NetworkStatus
import org.maplibre.nativeffi.runtime.runSuspendTest

class MaplibreConformanceTest {
  @Test
  fun globalOperationsReachTheNativeLibrary(): Unit = runSuspendTest {
    Maplibre.loadNativeLibrary()
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION, Maplibre.cVersion())
    assertTrue(
      Maplibre.supportedRenderBackends().isNotEmpty(),
      "a loaded library builds at least one render backend",
    )
    assertTrue(
      Maplibre.supportedOpenGLContextProviders().isNotEmpty() ||
        RenderBackend.OPENGL !in Maplibre.supportedRenderBackends(),
      "an OpenGL build exposes at least one context provider",
    )

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
  fun networkStatusRejectsAnUnknownInputBeforeNativeCall(): Unit = runSuspendTest {
    assertFailsWith<InvalidArgumentException> { Maplibre.setNetworkStatus(NetworkStatus(999)) }
  }
}
