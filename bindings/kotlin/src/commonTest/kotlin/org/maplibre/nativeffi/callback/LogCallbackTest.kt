package org.maplibre.nativeffi.callback

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.LogEvent
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.clearLogCallback
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.setTestLogCallback
import org.maplibre.nativeffi.withMap

/** The process-global log callback. */
class LogCallbackTest {
  @Test
  fun aReplacementLogCallbackReceivesNativeRecords() = runSuspendTest {
    val parsed = CompletableDeferred<LogEvent>()
    try {
      setTestLogCallback {}
      // Native releases the replaced registration before the replacement returns, and the binding
      // keeps the replacement.
      setTestLogCallback { event -> if (event == LogEvent.PARSE_STYLE) parsed.complete(event) }

      withMap {
        // A style whose center has the wrong JSON type logs a parser warning.
        map
          .setStyleJson("""{"version":8,"center":false,"sources":{},"layers":[]}""")
          .awaitWithin("the style command")
        assertEquals(
          LogEvent.PARSE_STYLE,
          parsed.awaitWithin("the replacement to receive a record"),
        )
      }
    } finally {
      clearLogCallback()
    }
  }
}
