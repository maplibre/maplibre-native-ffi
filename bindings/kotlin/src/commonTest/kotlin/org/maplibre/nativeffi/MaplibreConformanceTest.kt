package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LatLng
import org.maplibre.nativeffi.generated.NetworkStatus
import org.maplibre.nativeffi.generated.RenderBackendFlag
import org.maplibre.nativeffi.runtime.runSuspendTest

class MaplibreConformanceTest {
  @Test
  fun globalOperationsReachTheNativeLibrary(): Unit = runSuspendTest {
    Maplibre.loadNativeLibrary()
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION.toUInt(), GeneratedApi.cVersion())
    assertTrue(
      (GeneratedApi.supportedRenderBackendMask().rawValue != 0u),
      "a loaded library builds at least one render backend",
    )
    assertTrue(
      (GeneratedApi.openglSupportedContextProviderMask().rawValue != 0u) ||
        RenderBackendFlag.OPENGL !in GeneratedApi.supportedRenderBackendMask(),
      "an OpenGL build exposes at least one context provider",
    )

    val original = GeneratedApi.networkStatusGet()
    try {
      GeneratedApi.networkStatusSet(NetworkStatus.OFFLINE)
      assertEquals(NetworkStatus.OFFLINE, GeneratedApi.networkStatusGet())
      GeneratedApi.networkStatusSet(NetworkStatus.ONLINE)
      assertEquals(NetworkStatus.ONLINE, GeneratedApi.networkStatusGet())
    } finally {
      GeneratedApi.networkStatusSet(original)
    }

    val meters = GeneratedApi.projectedMetersForLatLng(LatLng(0.0, 0.0))
    assertEquals(LatLng(0.0, 0.0), GeneratedApi.latLngForProjectedMeters(meters))
  }

  @Test
  fun networkStatusRejectsAnUnknownInputBeforeNativeCall(): Unit = runSuspendTest {
    assertFailsWith<InvalidArgumentException> { GeneratedApi.networkStatusSet(NetworkStatus(999u)) }
  }
}
