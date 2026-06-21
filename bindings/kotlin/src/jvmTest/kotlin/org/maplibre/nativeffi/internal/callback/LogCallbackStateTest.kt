package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNotNull
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

class LogCallbackStateTest {
  @Test
  fun logCallbackCopiesRecordAndReturnsConsumedFlag() {
    var copiedRecord: LogRecord? = null

    try {
      LogCallbackState.set(
        LogCallback { record ->
          copiedRecord = record
          true
        }
      )
      val state = assertNotNull(LogCallbackState.currentForTesting())

      Arena.ofConfined().use { arena ->
        val result =
          state.invoke(
            MemorySegment.NULL,
            LogSeverity.WARNING.nativeValue,
            LogEvent.RENDER.nativeValue,
            42L,
            arena.allocateFrom("render warning"),
          )

        assertEquals(1, result)
      }

      assertEquals(
        LogRecord(LogSeverity.WARNING, LogEvent.RENDER, 42L, "render warning"),
        copiedRecord,
      )
    } finally {
      LogCallbackState.clear()
    }
  }
}
