package org.maplibre.nativeffi.log

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use
import org.maplibre.nativeffi.sleepMillis

@OptIn(ExperimentalAtomicApi::class)
class LogCallbackRegistrationTest {
  @Test
  fun replacingTheLogCallbackRoutesLaterRecordsToTheNewestOne(): Unit = runSuspendTest {
    val first = AtomicInt(0)
    val second = AtomicInt(0)

    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      runtime
        .mapCreate(
          GeneratedApi.mapOptionsDefault()
            .copy(
              initialExtent =
                GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
            )
        )
        .await()
        .use { map ->
          try {
            GeneratedApi.logSetCallback({ _, _, _, _ ->
              first.fetchAndAdd(1)
              1u
            })
            map.emitParserWarning()
            awaitRecord(first, "the first callback never received a record")

            GeneratedApi.logSetCallback({ _, _, _, _ ->
              second.fetchAndAdd(1)
              1u
            })
            val seenByFirst = first.load()
            map.emitParserWarning()
            awaitRecord(second, "the replacement callback never received a record")
            assertEquals(
              seenByFirst,
              first.load(),
              "the replaced callback kept receiving records after replacement",
            )

            GeneratedApi.logClearCallback()
            val seenBySecond = second.load()
            map.emitParserWarning()
            assertEquals(seenBySecond, second.load(), "records arrived after the callback cleared")
          } finally {
            GeneratedApi.logClearCallback()
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
  private fun awaitRecord(records: AtomicInt, message: String) {
    repeat(RECORD_TIMEOUT_MILLIS) {
      if (records.load() > 0) return
      sleepMillis(1)
    }
    assertTrue(records.load() > 0, message)
  }

  private companion object {
    private const val RECORD_TIMEOUT_MILLIS = 5_000
  }
}
