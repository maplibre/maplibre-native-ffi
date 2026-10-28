package org.maplibre.nativeffi.browser

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.LogSeverity
import org.maplibre.nativeffi.QueuedLogHandler
import org.maplibre.nativeffi.TestSupport
import org.maplibre.nativeffi.awaitDeliveries
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.clearLogCallback
import org.maplibre.nativeffi.internal.callback.CallbackOwner
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
    val roots = CallbackOwner.global
    val base = roots.rootCountForTesting()
    val calls = mutableListOf<String>()
    val second = CompletableDeferred<Unit>()
    try {
      setLogCallback(
        QueuedLogHandler { severity, _, _, message ->
          if (severity == LogSeverity.ERROR && FAILURE in message) {
            calls += "record rooted=${roots.rootCountForTesting() - base}"
            if (calls.size == 2) second.complete(Unit)
          }
        },
      )
      TestSupport.logError()
      TestSupport.logError()
    } finally {
      // Native queues the registration's release behind its records.
      clearLogCallback()
    }
    assertEquals(emptyList(), calls, "a record ran inside the call that queued it")
    second.awaitWithin("the second record")
    assertEquals(listOf("record rooted=1", "record rooted=1"), calls)
    assertEquals(base, roots.rootCountForTesting())
  }

  @Test
  fun aQueuedCallNeverRunsInsideANativeCallThatSpins() = runSuspendTest {
    var during = false
    val logged = CompletableDeferred<Unit>()
    try {
      setLogCallback(
        QueuedLogHandler { _, _, _, message ->
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
    awaitDeliveries()
  }

  private companion object {
    // What the record that the fixtures make native log says.
    const val FAILURE = "owner finalization failed"
  }
}
