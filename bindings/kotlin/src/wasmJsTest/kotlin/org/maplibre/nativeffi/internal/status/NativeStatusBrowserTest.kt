package org.maplibre.nativeffi.internal.status

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.NativeErrorException
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.render.FrameAcquirePolicy
import org.maplibre.nativeffi.render.MetalContextDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.NativePointer
import org.maplibre.nativeffi.render.RenderTargetExtent
import org.maplibre.nativeffi.runtime.NetworkStatus
import org.maplibre.nativeffi.withMap

/**
 * What a failure looks like by the time it reaches a page.
 *
 * The diagnostic is the part this target cannot take for granted. `mln_thread_last_error_message`
 * reports the message belonging to the *calling* native thread, and almost every call here ran on
 * the module's owner thread rather than on the page — so a binding that read the message where the
 * exception is built would report an empty or unrelated one. The dispatch path copies it beside the
 * status instead, on the thread that produced it, which is what these tests are checking.
 */
class NativeStatusBrowserTest {
  // Spec coverage: BND-020, BND-021, BND-022, BND-023, BND-025, BND-026.

  @Test
  fun eachStatusCategoryANativeCallProducesArrivesAsItsOwnExceptionType(): Promise<JsAny?> =
    browserTest {
      maplibreScope {
        withMap { _, map ->
          // Invalid argument, produced by native coordinate validation.
          val invalidArgument =
            assertFailsWith<InvalidArgumentException> {
              map.pixelForLatLng(LatLng(Double.NaN, 0.0))
            }
          assertEquals(MaplibreStatus.INVALID_ARGUMENT, invalidArgument.status)
          assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, invalidArgument.nativeStatusCode)

          // Native error, produced by the style parser refusing a document.
          val nativeError =
            assertFailsWith<NativeErrorException> { map.setStyleJson("not a style") }
          assertEquals(MaplibreStatus.NATIVE_ERROR, nativeError.status)
          assertEquals(MaplibreStatus.NATIVE_ERROR.nativeCode, nativeError.nativeStatusCode)

          // Unsupported, for a backend this module was not built with.
          val unsupported =
            assertFailsWith<UnsupportedFeatureException> {
              map.attachMetalOwnedTexture(
                MetalOwnedTextureDescriptor(
                  RenderTargetExtent(16, 16, 1.0),
                  MetalContextDescriptor(NativePointer.ofAddress(0x10L)),
                )
              )
            }
          assertEquals(MaplibreStatus.UNSUPPORTED, unsupported.status)

          // Invalid state, from the binding's own closed-handle guard.
          val projection = map.createProjection()
          projection.close()
          val invalidState =
            assertFailsWith<InvalidStateException> { projection.pixelForLatLng(LatLng(0.0, 0.0)) }
          assertEquals(MaplibreStatus.INVALID_STATE, invalidState.status)
        }
      }
    }

  @Test
  fun anUnknownStatusKeepsItsRawValueRatherThanBecomingAKnownOne(): Promise<JsAny?> = browserTest {
    // No C call in this module returns a status from a future revision, so the conversion is driven
    // directly. What matters is that an unrecognized code is carried rather than collapsed into one
    // of the categories this binding happens to know.
    val exception = Status.exception(-127)

    assertEquals(MaplibreStatus(-127), exception.status)
    assertEquals(-127, exception.nativeStatusCode)
  }

  /**
   * The diagnostic a failure on the owner thread carries back to the page.
   *
   * `mln_thread_last_error_message` is thread-local, and the thread that produced the failure is
   * the module's owner thread rather than the page. So the message has to be copied where it was
   * produced and travel back beside the status. Nothing else in the binding can recover it: by the
   * time the page resumes, the owner thread has moved on and the page's own slot was never written.
   *
   * This is also what BND-026 rests on, because a cleanup call that fails cannot be shown to leave
   * the original message alone while every message is the same empty string.
   */
  @Test
  fun aDiagnosticIsCopiedAtTheFailureAndNotReReadLater(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        val first =
          assertFailsWith<InvalidArgumentException> { map.pixelForLatLng(LatLng(Double.NaN, 0.0)) }
        val copied = first.diagnostic
        assertTrue(copied.contains("latitude must be finite"), "diagnostic was [$copied]")

        // A second failing call on the same owner thread replaces the message the first one left
        // behind there. The exception already holds its own copy, so it still says what went wrong.
        assertFailsWith<NativeErrorException> { map.setStyleJson("not a style") }
        assertEquals(copied, first.diagnostic)

        // And a call that succeeds clears it, which is the case a lazily-read diagnostic would
        // report as an empty string.
        map.setStyleJson(EMPTY_STYLE_JSON)
        assertEquals(copied, first.diagnostic)

        // The cleanup path a failed frame acquire takes makes a native call of its own while the
        // original failure is in flight. What the caller is handed is still the original.
        val thrown =
          assertFailsWith<InvalidArgumentException> {
            FrameAcquirePolicy.cleanupAfterWrapperFailure(
              acquired = true,
              releaseNative = { map.pixelForLatLng(LatLng(0.0, Double.NaN)) },
              closeLocal = {},
              failure = first,
            )
          }
        assertSame(first, thrown)
        assertEquals(copied, thrown.diagnostic)
      }
    }
  }

  @Test
  fun aClosedHandleIsRefusedByTheBindingWithItsOwnDiagnostic(): Promise<JsAny?> = browserTest {
    maplibreScope {
      withMap { _, map ->
        // A native failure first, so a stale native message exists to be reported by mistake.
        val native =
          assertFailsWith<InvalidArgumentException> { map.pixelForLatLng(LatLng(Double.NaN, 0.0)) }
        assertTrue(native.diagnostic.contains("latitude"), native.diagnostic)

        val projection = map.createProjection()
        projection.close()

        val error =
          assertFailsWith<InvalidStateException> { projection.pixelForLatLng(LatLng(0.0, 0.0)) }

        // Binding-owned, so the message names the wrapper rather than repeating what native last
        // said, and nothing crossed into the module to produce it.
        assertEquals(MaplibreStatus.INVALID_STATE, error.status)
        assertEquals("MapProjectionHandle is already closed", error.diagnostic)
        assertFalse(error.diagnostic.contains("latitude"))
      }
    }
  }

  @Test
  fun aFailureRaisedBeforeTheModuleIsReachedNamesNoNativeDiagnostic(): Promise<JsAny?> =
    browserTest {
      // Binding-owned validation on a process-global entry point: it fails without dispatching, so
      // there is no owner-thread diagnostic for it to inherit.
      val error =
        assertFailsWith<InvalidArgumentException> { Maplibre.setNetworkStatus(NetworkStatus(900)) }

      assertEquals(MaplibreStatus.INVALID_ARGUMENT, error.status)
      assertTrue(error.diagnostic.contains("900"), error.diagnostic)
    }
}
