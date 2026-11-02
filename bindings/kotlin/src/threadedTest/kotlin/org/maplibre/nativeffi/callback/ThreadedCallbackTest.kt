package org.maplibre.nativeffi.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.HttpHeaderTransform
import org.maplibre.nativeffi.HttpHeaderTransformResponse
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.runOnBackgroundThread
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.setHeader
import org.maplibre.nativeffi.setHttpHeaderTransform
import org.maplibre.nativeffi.withMap

/**
 * The callbacks that compute native's answer on the MapLibre thread that calls them, which only a
 * threaded target serves.
 */
class ThreadedCallbackTest {
  @Test
  fun aHeaderResponseRefusesUseAfterItsCallbackAndFromAnotherThread() = runSuspendTest {
    val outcome = CompletableDeferred<Pair<Throwable?, Throwable?>>()
    var escaped: HttpHeaderTransformResponse? = null
    val transform =
      HttpHeaderTransform(
        callback = { _, url, response ->
          if (url != HTTP_STYLE_URL || outcome.isCompleted) return@HttpHeaderTransform
          escaped = response
          var fromAnotherThread: Throwable? = null
          runOnBackgroundThread {
            fromAnotherThread =
              runCatching {
                response.setHeader("X-Test", "1")
              }
                .exceptionOrNull()
          }
          val inside = runCatching {
            response.setHeader("X-Test", "1")
          }
            .exceptionOrNull()
          outcome.complete(inside to fromAnotherThread)
        },
      )
    withMap {
      // The provider passes the style through to the HTTP client, which runs the header transform
      // before it tries a port that nothing serves.
      runtime.setResourceProvider(passingThrough(HTTP_STYLE_URL)).awaitWithin("the provider")
      runtime.setHttpHeaderTransform(transform).awaitWithin("the header transform")
      map.setStyleUrl(HTTP_STYLE_URL).awaitWithin("the style command")
      val (inside, fromAnotherThread) = outcome.awaitWithin("the header transform callback")
      assertEquals(null, inside, "setting a header inside the callback is allowed")
      assertIs<IllegalStateException>(fromAnotherThread, "the response from another thread")
      assertFailsWith<IllegalStateException> {
        escaped!!.setHeader("X-Test", "1")
      }
      runtime.clearHttpHeaderTransform().awaitWithin("the header transform clear")
    }
  }

  /** Passes [url] through to native loading, and denies every other request. */
  private fun passingThrough(url: String) = denyingProvider { request, handle ->
    if (request.requestedUrl != url) return@denyingProvider false
    handle.passThrough()
    true
  }

  private companion object {
    // A loopback port that nothing serves, so the request fails without leaving the machine.
    const val HTTP_STYLE_URL = "http://127.0.0.1:1/style.json"
  }
}
