package org.maplibre.nativeffi.log

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.cstr
import kotlinx.cinterop.memScoped
import org.maplibre.nativeffi.internal.callback.LogCallbackState

@OptIn(ExperimentalForeignApi::class)
class LogCallbackStateTest : org.maplibre.nativeffi.NativeTestBase() {
  @Test
  fun callbackCopiesRecordsContainsFailuresAndStopsAfterClose() = memScoped {
    var copied: LogRecord? = null
    val state =
      LogCallbackState(
        LogCallback { record ->
          copied = record
          true
        }
      )
    assertEquals(
      1U,
      state.invoke(
        LogSeverity.WARNING.nativeValue.toUInt(),
        LogEvent.RENDER.nativeValue.toUInt(),
        42,
        "warning".cstr.getPointer(this),
      ),
    )
    assertEquals(LogRecord(LogSeverity.WARNING, LogEvent.RENDER, 42, "warning"), copied)
    state.close()
    assertEquals(0U, state.invoke(1U, 0U, 0, null))
    assertTrue(state.isClosedForTesting())

    val throwing = LogCallbackState(LogCallback { error("contained") })
    assertEquals(0U, throwing.invoke(1U, 0U, 0, null))
    throwing.close()
  }
}
