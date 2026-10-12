package org.maplibre.nativeffi.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.LogEvent
import org.maplibre.nativeffi.generated.LogHandler
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/** The process-global log callback. This is the only test that sets it. */
class LogCallbackTest {
  @Test
  fun replacingTheLogCallbackReleasesThePreviousRegistration(): Unit = runSuspendTest {
    val roots = CallbackOwner.global
    val base = roots.rootCountForTesting()
    val parsed = CompletableDeferred<LogEvent>()
    try {
      GeneratedApi.logSetCallback(LogHandler { _, _, _, _ -> 0u })
      assertEquals(base + 1, roots.rootCountForTesting())

      // Native releases the replaced registration before the replacement returns.
      GeneratedApi.logSetCallback(
        LogHandler { _, event, _, _ ->
          if (event == LogEvent.PARSE_STYLE) parsed.complete(event)
          0u
        }
      )
      assertEquals(base + 1, roots.rootCountForTesting())

      withMap {
        // A style whose center has the wrong JSON type logs a parser warning.
        map
          .setStyleJson(
            """{"version":8,"center":false,"sources":{},"layers":[]}""".encodeToByteArray()
          )
          .awaitWithin("the style command")
        assertEquals(
          LogEvent.PARSE_STYLE,
          parsed.awaitWithin("the replacement to receive a record"),
        )
      }
    } finally {
      GeneratedApi.logClearCallback()
    }
    assertEquals(base, roots.rootCountForTesting())
  }
}
