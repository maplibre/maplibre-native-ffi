package org.maplibre.nativeffi.log

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.cstr
import kotlinx.cinterop.memScoped
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.internal.callback.LogCallbackState

@OptIn(ExperimentalForeignApi::class)
class LogCallbackStateTest {
  @Test
  fun processGlobalLogCallbackCopiesRecordsContainsExceptionsAndClearsState() {
    val records = mutableListOf<LogRecord>()
    try {
      Maplibre.setLogCallback(
        LogCallback { record ->
          records += record
          true
        }
      )
      val state = LogCallbackState.currentForTesting()
      memScoped { assertEquals(1U, state?.invoke(1U, 3U, 7L, "hello".cstr.getPointer(this))) }
      assertEquals(LogSeverity.INFO, records.single().severity)
      assertEquals(LogEvent.PARSE_STYLE, records.single().event)
      assertEquals(7L, records.single().code)
      assertEquals("hello", records.single().message)

      Maplibre.setLogCallback(LogCallback { throw IllegalStateException("contained") })
      assertEquals(0U, LogCallbackState.currentForTesting()?.invoke(2U, 6U, 0L, null))
    } finally {
      Maplibre.clearLogCallback()
      assertNull(LogCallbackState.currentForTesting())
    }
  }

  @Test
  fun logSeverityMasksRejectUnknownInputs() {
    assertEquals(1 shl 1, LogSeverity.INFO.nativeMask)
    kotlin.test.assertFailsWith<IllegalArgumentException> { LogSeverity.UNKNOWN.nativeMask }
  }
}
