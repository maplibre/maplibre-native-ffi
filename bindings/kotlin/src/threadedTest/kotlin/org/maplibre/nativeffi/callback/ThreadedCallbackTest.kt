package org.maplibre.nativeffi.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertIs
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.HttpHeaderTransform
import org.maplibre.nativeffi.generated.HttpHeaderTransformResponse
import org.maplibre.nativeffi.generated.LogEvent
import org.maplibre.nativeffi.generated.LogHandler
import org.maplibre.nativeffi.generated.logSetCallback
import org.maplibre.nativeffi.generated.setHeader
import org.maplibre.nativeffi.generated.setHttpHeaderTransform
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.runOnBackgroundThread
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.assertCommitted
import org.maplibre.nativeffi.withMap

/**
 * The callbacks that compute native's answer on the MapLibre thread that calls them, which only a
 * threaded target serves.
 */
class ThreadedCallbackTest {
  @Test
  fun aLogCallbackAnswersWhetherItConsumesARecord() = runSuspendTest {
    val roots = CallbackOwner.global
    val base = roots.rootCountForTesting()
    val parsed = CompletableDeferred<LogEvent>()
    try {
      GeneratedApi.logSetCallback(
        LogHandler { _, event, _, _ ->
          if (event == LogEvent.PARSE_STYLE) parsed.complete(event)
          // Leave the record to MapLibre's own logger as well.
          0u
        },
      )
      assertEquals(base + 1, roots.rootCountForTesting())
      withMap {
        // A style whose center has the wrong JSON type logs a parser warning.
        map
          .setStyleJson(
            """{"version":8,"center":false,"sources":{},"layers":[]}""".encodeToByteArray(),
          )
          .awaitWithin("the style command")
        assertEquals(LogEvent.PARSE_STYLE, parsed.awaitWithin("the log callback's record"))
      }
    } finally {
      GeneratedApi.logClearCallback()
    }
    assertEquals(base, roots.rootCountForTesting())
  }

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
      assertCommitted(
        runtime.setResourceProvider(passingThrough(HTTP_STYLE_URL)).awaitWithin("the provider"),
      )
      assertCommitted(runtime.setHttpHeaderTransform(transform).awaitWithin("the header transform"))
      map.setStyleUrl(HTTP_STYLE_URL).awaitWithin("the style command")
      val (inside, fromAnotherThread) = outcome.awaitWithin("the header transform callback")
      assertEquals(null, inside, "setting a header inside the callback is allowed")
      assertIs<IllegalStateException>(fromAnotherThread, "the response from another thread")
      assertFailsWith<IllegalStateException> {
        escaped!!.setHeader("X-Test", "1")
      }
      assertCommitted(
        runtime.clearHttpHeaderTransform().awaitWithin("the header transform clear"),
      )
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
