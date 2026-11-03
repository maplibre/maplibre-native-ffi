package org.maplibre.nativeffi.browser

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.LogHandler
import org.maplibre.nativeffi.LogSeverity
import org.maplibre.nativeffi.TestSupport
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.clearLogCallback
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.setLogCallback

/**
 * The browser module's page shim, which wakes the page to dispatch the binding's delivery queue.
 * The fixtures in test_support.c make native queue a log record, from the page or from a thread of
 * their own.
 */
class DeliveryTest {
  @Test
  fun aQueuedCallRunsOnALaterPageTaskBeforeItsRelease() = runSuspendTest {
    var calls = 0
    val second = CompletableDeferred<Unit>()
    try {
      setLogCallback(
        LogHandler { severity, _, _, message ->
          if (severity == LogSeverity.ERROR && FAILURE in message) {
            calls += 1
            if (calls == 2) second.complete(Unit)
          }
        },
      )
      TestSupport.logError()
      TestSupport.logError()
    } finally {
      // Native queues the registration's release behind its records, so both still arrive.
      clearLogCallback()
    }
    assertEquals(0, calls, "a record ran inside the call that queued it")
    second.awaitWithin("the second record")
  }

  @Test
  fun aQueuedCallNeverRunsInsideANativeCallThatSpins() = runSuspendTest {
    var during = false
    val logged = CompletableDeferred<Unit>()
    try {
      setLogCallback(
        LogHandler { _, _, _, message ->
          if (FAILURE in message) {
            during = true
            logged.complete(Unit)
          }
        },
      )
      TestSupport.holdContention()

      // The holder logs while the page spins on the mutex, and posts a probe task to the system
      // queue, which the spin runs. A holder whose probe task never runs gives up after a deadline,
      // and the call then reports that it did not spin.
      assertTrue(TestSupport.contend(), "the contended call did not spin")
      assertFalse(during, "a queued call ran inside the spinning call")
      logged.awaitWithin("the record logged during the spin")
    } finally {
      clearLogCallback()
    }
  }

  private companion object {
    // What the record that the fixtures make native log says.
    const val FAILURE = "owner finalization failed"
  }
}
