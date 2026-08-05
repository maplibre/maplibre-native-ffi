package org.maplibre.nativeffi

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.math.abs
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.internal.wasm.BrowserModule
import org.maplibre.nativeffi.internal.wasm.NativeCall
import org.maplibre.nativeffi.internal.wasm.generated.StructLayouts
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
  // Spec coverage: BND-001, BND-160.

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
  fun aModuleReportingAnotherAbiVersionIsRefusedBeforeAnyHandleExists(): Promise<JsAny?> =
    browserTest {
      // No loadable module reports a version other than the one this binding was generated for, so
      // the guard is driven through the version seam the loader itself calls. What it protects is
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

      // The loaded module is unaffected, so the guard rejected rather than tore anything down.
      Maplibre.loadNativeLibrary()
    }

  @Test
  fun aModuleAlreadyOnThePageIsCheckedRatherThanTrusted(): Promise<JsAny?> = browserTest {
    // A page carries one module, and it is not always one this binding loaded: a second copy of
    // the binding, or a host driving the module directly, can have put it there first. The C ABI
    // version cannot tell those apart, since it stays 0 for the whole prerelease, so the digest is
    // what says whether such a module is this binding's and the loader compares it before it
    // returns rather than trusting whatever is already there. This suite serves one module and it
    // matches, so the guard is driven through the expectation the loader passes it, the way the C
    // ABI guard above is.
    val digestMismatch =
      assertFailsWith<InvalidStateException> { BrowserModule.checkLoadedModule(FOREIGN_DIGEST) }

    // Both digests, so the failure says which module is on the page as well as what was wanted. The
    // module's own is the one it reported through its entry point rather than the one this binding
    // asked for, which is what makes this a check of the module and not of the argument.
    assertTrue(
      digestMismatch.diagnostic.contains(
        "(module ${StructLayouts.HEADERS_DIGEST}, binding $FOREIGN_DIGEST)"
      ),
      "unexpected digest diagnostic: ${digestMismatch.diagnostic}",
    )

    // The loader itself, not only the comparison it makes. This module is already on the page,
    // which is the path that used to hand a caller whatever it found and return, so a failure here
    // says the check sits where a caller cannot get past it. The URL is never fetched, because the
    // module the loader would have accepted is refused before anything is resolved against it.
    val fromLoad =
      assertFailsWith<InvalidStateException> {
        BrowserModule.load(Maplibre.DEFAULT_MODULE_URL, expectedDigest = FOREIGN_DIGEST)
      }

    assertTrue(
      fromLoad.diagnostic.contains("already loaded on this page"),
      "unexpected load diagnostic: ${fromLoad.diagnostic}",
    )

    // The loaded module is unaffected, so the guard rejected rather than tore anything down.
    assertTrue(BrowserModule.checkLoadedModule(), "the module this suite loaded stopped matching")
    Maplibre.loadNativeLibrary()
  }

  @Test
  fun aModuleAlreadyOnThePagePackingCallsForAnotherProtocolIsRefused(): Promise<JsAny?> =
    browserTest {
      // Separate from the digest, because the two move independently: a module built from identical
      // headers can still pack a call differently, and a binding that packed for one and called the
      // other would mispack memory rather than fail. Reached through the loader's own expectation
      // for the same reason the digest is -- this page's module packs what this binding packs.
      val mismatch =
        assertFailsWith<InvalidStateException> {
          BrowserModule.checkLoadedModule(expectedProtocol = NativeCall.EXPECTED_PROTOCOL + 1)
        }

      assertTrue(
        mismatch.diagnostic.contains(
          "packs calls for protocol ${NativeCall.EXPECTED_PROTOCOL}, but this binding packs for " +
            "${NativeCall.EXPECTED_PROTOCOL + 1}"
        ),
        "unexpected protocol diagnostic: ${mismatch.diagnostic}",
      )

      Maplibre.loadNativeLibrary()
    }

  @Test
  fun aSecondBindingInstanceOnThePageIsRefused(): Promise<JsAny?> = browserTest {
    // The module and the calls parked on it live on the page, while the tokens and handles that
    // index into them live in one WebAssembly instance, so a second separately bundled instance
    // would resolve this one's calls with its own results. A suite runs in a single instance and
    // cannot make a second, but the whole of what a second one does at this seam is arrive with an
    // instance number of its own, which is what this mints.
    val second = BrowserModule.mintInstanceId()

    val error = assertFailsWith<InvalidStateException> { BrowserModule.checkSoleBinding(second) }

    assertTrue(
      error.diagnostic.contains("already owns this page"),
      "unexpected diagnostic: ${error.diagnostic}",
    )

    // Refusing claims nothing, so this instance still owns the page and can go on calling native.
    BrowserModule.checkSoleBinding()
    Maplibre.loadNativeLibrary()
  }

  @Test
  fun aCoordinateSurvivesAProjectionRoundTrip(): Promise<JsAny?> = browserTest {
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
  fun networkStatusRoundTripsAndRejectsAValueNativeCannotBeGiven(): Promise<JsAny?> = browserTest {
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

    /**
     * A headers digest no module reports.
     *
     * Shaped like one — the same hexadecimal alphabet — so the diagnostic it produces reads the way
     * a real mismatch's would, rather than making the check look like it caught a malformed value.
     */
    const val FOREIGN_DIGEST = "0000000000000000000000000000000000000000000000000000000000000000"
  }
}
