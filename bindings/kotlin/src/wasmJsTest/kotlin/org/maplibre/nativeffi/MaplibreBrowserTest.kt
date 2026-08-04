package org.maplibre.nativeffi

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.math.abs
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.render.OpenGLContextProvider
import org.maplibre.nativeffi.render.RenderBackend
import org.maplibre.nativeffi.runtime.NetworkStatus

/**
 * The entry points a page reaches without a runtime, and the load that has to precede them.
 *
 * Every test here runs on the page's own stack. These are the calls the binding performs in place
 * rather than on the module's owner thread, so they are also what says whether the module is
 * reachable at all before any thread exists.
 */
class MaplibreBrowserTest {
  @Test
  fun aLoadedModuleAgreesWithTheBindingAboutWhatItIs(): Promise<JsAny?> = browserTest {
    // Reaching this line means the loader accepted the module: it matched the headers digest this
    // binding's descriptors were generated from, packed calls for the same protocol, and carried
    // every entry point and runtime helper the binding calls. What follows is the first answer read
    // back out of the module through its call table.
    assertEquals(Maplibre.EXPECTED_C_ABI_VERSION, Maplibre.cVersion())
    assertEquals(setOf(RenderBackend.OPENGL), Maplibre.supportedRenderBackends())
    assertEquals(setOf(OpenGLContextProvider.WEBGL), Maplibre.supportedOpenGLContextProviders())
  }

  @Test
  fun aCoordinateSurvivesAProjectionRoundTrip(): Promise<JsAny?> = browserTest {
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
  fun networkStatusRoundTripsAndRejectsAValueNativeCannotBeGiven(): Promise<JsAny?> = browserTest {
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
