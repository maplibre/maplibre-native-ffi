package org.maplibre.nativeffi

import kotlin.math.abs
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.runtime.NetworkStatus

/**
 * The entry points a host reaches before any runtime exists.
 *
 * The module is already instantiated when Kotlin starts — it is what imported this module, on the
 * thread it gave it — so loading is an attach rather than a fetch. These are the first calls that
 * cross into it, and so also what says whether it is reachable at all.
 */
class MaplibreBrowserTest {
  // Spec coverage: BND-001, BND-160.

  @Test
  fun anAttachedModuleAgreesWithTheBindingAboutWhatItIs() {
    // Reaching this line means the attach found the module on this thread's global scope and named
    // it where every generated extern reads it. What follows is the first answer read back out.
    Maplibre.loadNativeLibrary()
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION, Maplibre.cVersion())
    assertEquals(setOf(RenderBackend.OPENGL), Maplibre.supportedRenderBackends())
    assertEquals(setOf(OpenGLContextProvider.WEBGL), Maplibre.supportedOpenGLContextProviders())
  }

  @Test
  fun aModuleReportingAnotherAbiVersionIsRefusedBeforeAnyHandleExists() {
    // No loadable module reports a version other than the one this binding was generated for, so
    // the guard is driven through the version seam the attach itself calls. What it protects is
    // ahead of every handle: a module that disagreed about the ABI would have been accepted and
    // then read descriptors at offsets that are not its own.
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

    // The attached module is unaffected, so the guard rejected rather than tore anything down.
    Maplibre.loadNativeLibrary()
  }

  @Test
  fun aCoordinateSurvivesAProjectionRoundTrip() {
    // Spec coverage: BND-103.
    // Two descriptors written at generated offsets, handed to native as pointers into the module's
    // heap, and read back. A layout that disagreed with the module would return a different
    // coordinate rather than fail, so the round trip is what checks it.
    val coordinate = LatLng(latitude = 37.8199, longitude = -122.4783)

    val meters = Maplibre.projectedMetersForLatLng(coordinate)
    val returned = Maplibre.latLngForProjectedMeters(meters)

    assertTrue(meters.northing > 0.0 && meters.easting < 0.0, "unexpected meters $meters")
    assertTrue(
      abs(returned.latitude - coordinate.latitude) < TOLERANCE_DEGREES &&
        abs(returned.longitude - coordinate.longitude) < TOLERANCE_DEGREES,
      "$returned is not $coordinate",
    )
  }

  @Test
  fun networkStatusRoundTripsAndRejectsAValueNativeCannotBeGiven() {
    // Spec coverage: BND-068.
    // Process-global state written and read back through an out-parameter, which is the shape most
    // of this API takes.
    try {
      Maplibre.setNetworkStatus(NetworkStatus.OFFLINE)
      assertEquals(NetworkStatus.OFFLINE, Maplibre.networkStatus)
    } finally {
      Maplibre.setNetworkStatus(NetworkStatus.ONLINE)
    }
    assertEquals(NetworkStatus.ONLINE, Maplibre.networkStatus)

    // Binding-owned validation, which fails before anything crosses into the module.
    assertFailsWith<InvalidArgumentException> { Maplibre.setNetworkStatus(NetworkStatus(900)) }
  }

  private companion object {
    /** Both conversions are double precision, so a round trip loses far less than this. */
    const val TOLERANCE_DEGREES = 1e-9
  }
}
