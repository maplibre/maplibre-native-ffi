package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.concurrent.thread
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertSame
import kotlin.test.assertTrue
import org.maplibre.nativeffi.log.LogCallback
import org.maplibre.nativeffi.log.LogEvent
import org.maplibre.nativeffi.log.LogRecord
import org.maplibre.nativeffi.log.LogSeverity

class LogCallbackStateTest {
  @Test
  fun logCallbackCopiesRecordAndReturnsConsumedFlag(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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

  @Test
  fun callbackClosureWaitsForEnteredCallbacksAndSupportsClosureFromCallback(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val entered = CountDownLatch(1)
      val release = CountDownLatch(1)
      lateinit var state: LogCallbackState
      state =
        LogCallbackState.setForTestingAndGet(
          LogCallback {
            entered.countDown()
            release.await(5, TimeUnit.SECONDS)
            true
          }
        )
      val invocation = thread { state.invoke(MemorySegment.NULL, 0, 0, 0, MemorySegment.NULL) }
      assertTrue(entered.await(5, TimeUnit.SECONDS))
      val closed = CountDownLatch(1)
      val closer = thread {
        state.close()
        closed.countDown()
      }
      assertFalse(closed.await(50, TimeUnit.MILLISECONDS))
      release.countDown()
      invocation.join()
      closer.join()
      assertTrue(state.isClosedForTesting())

      lateinit var reentrant: LogCallbackState
      reentrant =
        LogCallbackState.setForTestingAndGet(
          LogCallback {
            reentrant.close()
            true
          }
        )
      assertEquals(1, reentrant.invoke(MemorySegment.NULL, 0, 0, 0, MemorySegment.NULL))
      assertTrue(reentrant.isClosedForTesting())
      LogCallbackState.clearForTesting()
    }

  @Test
  fun replacementPreservesRawEnumsAndFailedReplacementKeepsPreviousState(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      var copied: LogRecord? = null
      try {
        LogCallbackState.setForTesting(
          LogCallback {
            copied = it
            false
          }
        )
        val previous = assertNotNull(LogCallbackState.currentForTesting())
        assertFailsWith<IllegalStateException> {
          LogCallbackState.setForTesting(LogCallback { true }) { error("registration failed") }
        }
        assertSame(previous, LogCallbackState.currentForTesting())
        assertFalse(previous.isClosedForTesting())
        previous.invoke(MemorySegment.NULL, 991, 992, 7, MemorySegment.NULL)
        assertEquals(991, copied?.severity?.nativeValue)
        assertEquals(992, copied?.event?.nativeValue)

        LogCallbackState.setForTesting(LogCallback { true })
        assertTrue(previous.isClosedForTesting())
      } finally {
        LogCallbackState.clearForTesting()
      }
    }

  private fun LogCallbackState.Companion.setForTestingAndGet(
    callback: LogCallback
  ): LogCallbackState {
    setForTesting(callback)
    return assertNotNull(currentForTesting())
  }
}
