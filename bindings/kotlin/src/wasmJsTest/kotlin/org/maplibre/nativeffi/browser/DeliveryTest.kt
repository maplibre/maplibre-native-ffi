package org.maplibre.nativeffi.browser

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import kotlinx.coroutines.CompletableDeferred
import org.maplibre.nativeffi.TestSupport
import org.maplibre.nativeffi.TestWake
import org.maplibre.nativeffi.awaitDeliveries
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.runSuspendTest

/**
 * The browser module's wake shim, which carries each wake and its release to the page. The fixtures
 * in test_support.c call a wake registration's C functions as native would, from a thread of their
 * own or from the page.
 */
class DeliveryTest {
  @Test
  fun eachWakeRunsOnALaterPageTaskBeforeItsRelease() = runSuspendTest {
    val calls = mutableListOf<String>()
    val second = CompletableDeferred<Unit>()
    lateinit var wake: TestWake
    wake = TestWake {
      calls += "wake rooted=${wake.owner.rootCountForTesting()}"
      if (calls.size == 2) second.complete(Unit)
    }
    wake.use {
      TestSupport.wake(wake.address)
      TestSupport.wake(wake.address)
      TestSupport.release(wake.address)
      assertEquals(emptyList(), calls, "a wake ran inside the call that posted it")
      second.awaitWithin("the second wake")
      awaitDeliveries()
      assertEquals(listOf("wake rooted=1", "wake rooted=1"), calls)
      assertEquals(0, wake.owner.rootCountForTesting())
    }
  }

  @Test
  fun aDeliveryNeverRunsInsideANativeCallThatSpins() = runSuspendTest {
    val held = CompletableDeferred<Unit>()
    var during = false
    val deliveredDuring = CompletableDeferred<Unit>()
    TestWake { held.complete(Unit) }
      .use { holding ->
        TestWake {
          during = true
          deliveredDuring.complete(Unit)
        }
          .use { spinning ->
            TestSupport.holdContention(holding.address, spinning.address)
            held.awaitWithin("the holder to take the mutex")

            // The holder posts a wake while the page spins on the mutex, and a probe task on the
            // system queue, which the spin runs. A holder whose probe task never runs gives up
            // after a deadline, and the call then reports that it did not spin.
            assertTrue(TestSupport.contend(), "the contended call did not spin")
            assertFalse(during, "a delivery ran inside the spinning call")
            deliveredDuring.awaitWithin("the wake posted during the spin")
            awaitDeliveries()
            assertEquals(0, holding.owner.rootCountForTesting())
            assertEquals(0, spinning.owner.rootCountForTesting())
          }
      }
  }
}
