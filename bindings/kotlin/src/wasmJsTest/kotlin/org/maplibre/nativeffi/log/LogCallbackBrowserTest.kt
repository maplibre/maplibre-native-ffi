package org.maplibre.nativeffi.log

import kotlin.js.JsAny
import kotlin.js.Promise
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.Maplibre
import org.maplibre.nativeffi.browserTest
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.internal.callback.LogQueueBridge
import org.maplibre.nativeffi.maplibreScope
import org.maplibre.nativeffi.nextPageTask
import org.maplibre.nativeffi.withMap

/**
 * The process-global log callback, which reaches a page on a browser task of its own.
 *
 * Logging has no runtime to pump, so this binding drains the module's log queue onto its own
 * macrotask rather than inside a pump: a host most wants logging during startup and during teardown
 * after the last pump. That is the one visible difference from the other platforms — a record is
 * delivered on a later turn of the page's event loop rather than on the thread that produced it —
 * so a test has to yield the page before it can assert on what arrived.
 *
 * A callback's return value cannot be honoured here, and that is stated rather than worked around:
 * MapLibre needs the consumed/not-consumed decision on the logging thread, before the record has
 * reached the page at all.
 */
class LogCallbackBrowserTest {
  // Spec coverage: BND-120, BND-121, BND-122, BND-123.

  @Test
  fun aRecordNativeProducesReachesTheInstalledCallback(): Promise<JsAny?> = browserTest {
    val records = mutableListOf<LogRecord>()
    try {
      Maplibre.setAsyncLogSeverities(
        setOf(LogSeverity.INFO, LogSeverity.WARNING, LogSeverity.ERROR)
      )
      Maplibre.setLogCallback { record ->
        records += record
        true
      }

      // A style URL whose scheme no file source serves. The failure is reported as a log record
      // as well as as an event, and it needs no network to produce.
      maplibreScope {
        withMap { runtime, map ->
          map.setStyleUrl(UNSERVED_STYLE_URL)
          repeat(PUMPS) {
            runtime.pump(PUMP_MILLIS)
            while (runtime.pollEvent() != null) {}
          }
        }
      }

      // The drain runs as its own browser task, so the records arrive on a later turn than the one
      // that produced them.
      yieldUntil { records.isNotEmpty() }
      val record = assertNotNull(records.firstOrNull(), "no log record reached the callback")
      assertTrue(record.message.isNotBlank())
      assertTrue(record.severity.nativeValue > 0)

      // Clearing stops delivery: nothing arrives on the turns after it.
      Maplibre.clearLogCallback()
      val delivered = records.size
      repeat(PAGE_TURNS) { nextPageTask() }
      assertEquals(delivered, records.size)
    } finally {
      runCatching { Maplibre.clearLogCallback() }
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  @Test
  fun onlyTheCallbackInstalledLastReceivesARecord(): Promise<JsAny?> = browserTest {
    // Delivery is driven directly here rather than through native, because which callback receives
    // a record is this binding's own decision: the module's registration is made once and never
    // withdrawn, so nothing native does distinguishes an installed callback from a replaced one.
    val first = mutableListOf<LogRecord>()
    val second = mutableListOf<LogRecord>()

    val firstBridge = LogQueueBridge {
      first += it
      true
    }
    val secondBridge = LogQueueBridge {
      second += it
      true
    }

    val record = LogRecord(LogSeverity.INFO, LogEvent.PARSE_STYLE, 7L, "hello")
    firstBridge.deliver(record)
    assertEquals(listOf(record), first)

    // Replacement retires the previous registration, which then delivers to nobody.
    firstBridge.close()
    secondBridge.deliver(record)
    firstBridge.deliver(record)
    assertEquals(listOf(record), first)
    assertEquals(listOf(record), second)

    // Clearing retires the replacement too.
    secondBridge.close()
    secondBridge.deliver(record)
    assertEquals(listOf(record), second)
  }

  @Test
  fun aFailingCallbackIsContainedAndLaterRecordsStillArrive(): Promise<JsAny?> = browserTest {
    val seen = mutableListOf<LogRecord>()
    val bridge = LogQueueBridge {
      seen += it
      // Nothing above this frame is a native frame to unwind into, and a callback that failed
      // must not stop the drain for every later record.
      throw IllegalStateException("contained")
    }

    val first = LogRecord(LogSeverity.WARNING, LogEvent.GENERAL, 1L, "first")
    val second = LogRecord(LogSeverity.ERROR, LogEvent.GENERAL, 2L, "second")
    bridge.deliver(first)
    bridge.deliver(second)

    assertEquals(listOf(first, second), seen)
    bridge.close()
  }

  @Test
  fun aCallbackCannotBeReplacedOrClearedFromInsideItself(): Promise<JsAny?> = browserTest {
    var replaceError: Throwable? = null
    var clearError: Throwable? = null
    try {
      // Replacing or clearing from inside the callback being replaced cannot wait for that upcall
      // to finish, so it is refused rather than returning while the old callback is still running.
      Maplibre.setLogCallback {
        replaceError = runCatching { Maplibre.setLogCallback { true } }.exceptionOrNull()
        clearError = runCatching { Maplibre.clearLogCallback() }.exceptionOrNull()
        true
      }

      Maplibre.setAsyncLogSeverities(
        setOf(LogSeverity.INFO, LogSeverity.WARNING, LogSeverity.ERROR)
      )
      maplibreScope {
        withMap { runtime, map ->
          map.setStyleUrl(UNSERVED_STYLE_URL)
          repeat(PUMPS) {
            runtime.pump(PUMP_MILLIS)
            while (runtime.pollEvent() != null) {}
          }
        }
      }
      yieldUntil { replaceError != null }

      assertTrue(replaceError is InvalidStateException, "replace reported $replaceError")
      assertTrue(clearError is InvalidStateException, "clear reported $clearError")
    } finally {
      runCatching { Maplibre.clearLogCallback() }
      Maplibre.restoreDefaultAsyncLogSeverities()
    }
  }

  @Test
  fun aSeverityMaskRefusesAValueTheModuleHasNoBitFor(): Promise<JsAny?> = browserTest {
    assertEquals(1 shl 1, LogSeverity.INFO.nativeMask)
    assertFailsWith<InvalidArgumentException> {
      Maplibre.setAsyncLogSeverities(setOf(LogSeverity(900)))
    }
  }

  /** Yields the page until [predicate] holds or the turns run out. */
  private suspend fun yieldUntil(predicate: () -> Boolean) {
    repeat(PAGE_TURNS) {
      if (predicate()) return
      nextPageTask()
    }
  }

  private companion object {
    const val PUMPS = 40
    const val PUMP_MILLIS = 2L
    const val PAGE_TURNS = 200

    /**
     * A scheme no file source serves, which MapLibre reports through the log as well as an event.
     */
    const val UNSERVED_STYLE_URL = "jar:file:/packaged/style.json"
  }
}
