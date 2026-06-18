package org.maplibre.nativeffi

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.set
import kotlinx.cinterop.toKString
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.runtime.NetworkStatus
import platform.posix.getenv

@OptIn(ExperimentalForeignApi::class)
class MaplibreTest {
  @Test
  fun processGlobalNetworkStatusAndProjectionHelpersMatchNativeAbi() {
    Maplibre.loadNativeLibrary()
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION, Maplibre.cVersion())
    Maplibre.supportedRenderBackends()

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
    val coordinate = Maplibre.latLngForProjectedMeters(meters)
    assertEquals(0.0, coordinate.latitude)
    assertEquals(0.0, coordinate.longitude)
  }

  @Test
  fun configuredRenderBackendIsReportedSupported() {
    // Inapplicable when the test target is not configured for a specific render backend.
    val configured = getenv("MLN_FFI_RENDER_BACKEND")?.toKString() ?: return
    val expected =
      when (configured) {
        "metal" -> RenderBackend.METAL
        "vulkan" -> RenderBackend.VULKAN
        "opengl" -> RenderBackend.OPENGL
        else -> error("Unknown MLN_FFI_RENDER_BACKEND=$configured")
      }

    assertTrue(expected in Maplibre.supportedRenderBackends())
    if (expected == RenderBackend.OPENGL) {
      assertTrue(Maplibre.supportedOpenGLContextProviders().isNotEmpty())
    }
  }

  @Test
  fun animationOptionsUseNullableDurationMsShape() {
    val options = AnimationOptions().apply { durationMs = 12.0 }
    assertEquals(12.0, options.durationMs)
    options.durationMs = null
    assertEquals(null, options.durationMs)
  }

  @Test
  fun unknownNetworkStatusPreservesRawValue() {
    val status = Maplibre.networkStatusForTesting { outStatus ->
      outStatus[0] = 999U
      MaplibreStatus.OK.nativeCode
    }

    assertEquals(NetworkStatus(999), status)
    assertEquals(999, status.nativeValue)
  }

  @Test
  fun unknownNetworkStatusIsOutputOnly() {
    assertFailsWith<IllegalArgumentException> { Maplibre.setNetworkStatus(NetworkStatus(999)) }
  }

  @Test
  fun abiVersionMismatchUsesStableBindingError() {
    val error =
      assertFailsWith<AbiVersionMismatchException> {
        Maplibre.checkCompatibleCAbi(Maplibre.EXPECTED_C_ABI_VERSION + 1L)
      }

    assertEquals(MaplibreStatus.NATIVE_ERROR, error.status)
    assertIs<NativeErrorException>(error)
    assertEquals(MaplibreStatus.NATIVE_ERROR.nativeCode, error.nativeStatusCode)
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION + 1L, error.actualVersion)
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION, error.expectedVersion)
    assertTrue(error.diagnostic.contains("C ABI version"))
    assertTrue(error.diagnostic.contains("expected ${Maplibre.EXPECTED_C_ABI_VERSION}"))
  }
}
