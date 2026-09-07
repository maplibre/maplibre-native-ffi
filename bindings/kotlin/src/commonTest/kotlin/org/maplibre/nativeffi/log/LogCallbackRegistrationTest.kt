package org.maplibre.nativeffi.log

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use
import org.maplibre.nativeffi.sleepMillis

class LogCallbackRegistrationTest {
  @Test
  fun replacingTheLogCallbackRoutesLaterRecordsToTheNewestOne(): Unit = runSuspendTest {
    val first = mutableListOf<LogRecord>()
    val second = mutableListOf<LogRecord>()

    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(
          runtime,
          MapOptions().apply {
            width = 64
            height = 64
          },
        )
        .await()
        .use { map ->
          try {
            Maplibre.setLogCallback(
              LogCallback {
                first += it
                true
              }
            )
            map.emitParserWarning()
            awaitRecord(first, "the first callback never received a record")

            Maplibre.setLogCallback(
              LogCallback {
                second += it
                true
              }
            )
            val seenByFirst = first.size
            map.emitParserWarning()
            awaitRecord(second, "the replacement callback never received a record")
            assertEquals(
              seenByFirst,
              first.size,
              "the replaced callback kept receiving records after replacement",
            )

            Maplibre.clearLogCallback()
            val seenBySecond = second.size
            map.emitParserWarning()
            sleepMillis(RECORD_TIMEOUT_MILLIS)
            assertEquals(seenBySecond, second.size, "records arrived after the callback cleared")
          } finally {
            Maplibre.clearLogCallback()
          }
        }
    }
  }

  /** Loads a style whose center is the wrong JSON type, which logs a native parser warning. */
  private suspend fun MapHandle.emitParserWarning() {
    setStyleJson("""{"version":8,"center":false,"sources":{},"layers":[]}""".encodeToByteArray())
      .await()
    runtime().barrier().await()
  }

  /** Waits for one record, since MapLibre logs the parse on a worker of its own. */
  private fun awaitRecord(records: List<LogRecord>, message: String) {
    repeat(RECORD_TIMEOUT_MILLIS) {
      if (records.isNotEmpty()) return
      sleepMillis(1)
    }
    assertTrue(records.isNotEmpty(), message)
  }

  private companion object {
    private const val RECORD_TIMEOUT_MILLIS = 5_000
  }
}
