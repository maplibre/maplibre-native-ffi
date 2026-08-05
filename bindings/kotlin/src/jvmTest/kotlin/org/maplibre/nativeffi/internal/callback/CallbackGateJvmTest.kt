package org.maplibre.nativeffi.internal.callback

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue

class CallbackGateJvmTest {
  @Test
  fun closeWaitsForActiveCallbackOnAnotherThread() {
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

    Thread.sleep(50)
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

  /**
   * The other close, which a gate whose bodies can suspend needs.
   *
   * The two are only distinguishable where the closer is not the callback's own thread, which on
   * the browser is every close that matters: a suspended delivery is on neither the closing stack
   * nor a thread that waiting could make progress against. Asserted here rather than there because
   * a second thread is what makes the difference observable at all.
   *
   * So the claim is threefold. It returns with a body still inside; it refuses everything from that
   * moment, which is the guarantee a registration is retired by; and the native state is still
   * released exactly once, when the body it left running finally leaves.
   */
  @Test
  fun closeWithoutDrainingReturnsWhileACallbackOnAnotherThreadIsStillInside() {
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

    // On its own thread with a bounded join, so a close that waited fails the assertion below
    // rather than hanging the run on a callback nothing has released yet.
    val closeThread = Thread {
      gate.closeWithoutDraining()
      closeReturned.set(true)
    }
    closeThread.start()
    closeThread.join(5_000)

    assertTrue(closeReturned.get(), "the close waited for the callback that was still inside")
    assertNull(gate.enter())
    assertFalse(gate.isClosedForTesting())
    assertEquals(0, closes)

    releaseCallback.countDown()
    callbackThread.join(5_000)

    assertTrue(gate.isClosedForTesting())
    assertEquals(1, closes)
  }
}
