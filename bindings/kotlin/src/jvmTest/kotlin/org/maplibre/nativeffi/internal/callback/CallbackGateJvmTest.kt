package org.maplibre.nativeffi.internal.callback

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertTrue

class CallbackGateJvmTest {
  @Test
  fun closeWaitsForActiveCallbackOnAnotherThread(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      var closes = 0
      val gate = CallbackGate("test callbacks") { closes++ }
      val entered = CountDownLatch(1)
      val releaseCallback = CountDownLatch(1)
      val closeReturned = AtomicBoolean(false)

      val callbackThread = Thread {
        val lease = assertNotNull(gate.enter())
        entered.countDown()
        assertTrue(releaseCallback.await(5, TimeUnit.SECONDS))
        lease.close()
      }
      callbackThread.start()
      assertTrue(entered.await(5, TimeUnit.SECONDS))

      val closeThread = Thread {
        gate.close()
        closeReturned.set(true)
      }
      closeThread.start()

      assertTrue(waitForClosing(gate))
      assertFalse(closeReturned.get())
      assertFalse(gate.isClosedForTesting())
      assertEquals(0, closes)

      releaseCallback.countDown()
      closeThread.join(5_000)
      callbackThread.join(5_000)

      assertTrue(closeReturned.get())
      assertTrue(gate.isClosedForTesting())
      assertEquals(1, closes)
    }

  private fun waitForClosing(gate: CallbackGate): Boolean {
    val deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(5)
    while (System.nanoTime() < deadline) {
      if (gate.isClosingForTesting()) return true
      Thread.yield()
    }
    return false
  }
}
