package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

class LogCallbackStateTest {
  @Test
  fun callbackCopiesRecordsContainsFailuresAndStopsAfterClose() {
    var copied: LogRecord? = null
    val state =
      LogCallbackState.createForTesting(
        LogCallback { record ->
          copied = record
          true
        }
      )
    Arena.ofConfined().use { arena ->
      assertEquals(
        1,
        state.invoke(
          MemorySegment.NULL,
          LogSeverity.WARNING.nativeValue,
          LogEvent.RENDER.nativeValue,
          42,
          arena.allocateFrom("warning"),
        ),
      )
    }
    assertEquals(LogRecord(LogSeverity.WARNING, LogEvent.RENDER, 42, "warning"), copied)
    state.close()
    assertEquals(0, state.invoke(MemorySegment.NULL, 1, 0, 0, MemorySegment.NULL))
    assertTrue(state.isClosedForTesting())

    val throwing = LogCallbackState.createForTesting(LogCallback { error("contained") })
    assertEquals(0, throwing.invoke(MemorySegment.NULL, 1, 0, 0, MemorySegment.NULL))
    throwing.close()
  }

  @Test
  fun nativeRegistrationReplacesAndClearsOwnedState() {
    Maplibre.setLogCallback(LogCallback { true })
    Maplibre.setLogCallback(LogCallback { false })
    Maplibre.clearLogCallback()
    Maplibre.clearLogCallback()
  }
}
