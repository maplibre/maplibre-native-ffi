package org.maplibre.nativeffi.internal.callback

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
import org.maplibre.nativeffi.log.LogRecord

class LogCallbackStateTest {
  @Test
  fun callbackClosureWaitsForEnteredCallbacksAndSupportsClosureFromCallback(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val entered = CountDownLatch(1)
      val release = CountDownLatch(1)
      val state =
        install(
          LogCallback {
            entered.countDown()
            release.await(5, TimeUnit.SECONDS)
            true
          }
        )
      val invocation = thread { state.invoke(0, 0, 0, null) }
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
        install(
          LogCallback {
            reentrant.close()
            true
          }
        )
      assertEquals(1, reentrant.invoke(0, 0, 0, null))
      assertTrue(reentrant.isClosedForTesting())
      LogCallbackState.clearForTesting()
    }

  @Test
  fun replacementPreservesRawEnumsAndFailedReplacementKeepsPreviousState(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      var copied: LogRecord? = null
      try {
        val previous =
          install(
            LogCallback {
              copied = it
              false
            }
          )
        assertFailsWith<IllegalStateException> {
          LogCallbackState.setForTesting(LogCallback { true }) { error("registration failed") }
        }
        assertSame(previous, LogCallbackState.currentForTesting())
        assertFalse(previous.isClosedForTesting())
        previous.invoke(991, 992, 7, null)
        assertEquals(991, copied?.severity?.nativeValue)
        assertEquals(992, copied?.event?.nativeValue)

        LogCallbackState.setForTesting(LogCallback { true })
        assertTrue(previous.isClosedForTesting())
      } finally {
        LogCallbackState.clearForTesting()
      }
    }

  private fun install(callback: LogCallback): LogCallbackState {
    LogCallbackState.setForTesting(callback)
    return assertNotNull(LogCallbackState.currentForTesting())
  }
}
