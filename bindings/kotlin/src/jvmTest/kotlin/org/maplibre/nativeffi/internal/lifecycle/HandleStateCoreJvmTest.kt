package org.maplibre.nativeffi.internal.lifecycle

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicBoolean
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.MaplibreStatus

class HandleStateCoreJvmTest {
  // BND-197.
  @Test
  fun closeWaitsForAUseInFlightOnAnotherThread() {
    val state = HandleStateCore("TestHandle", 0x1234)
    var destroys = 0
    val entered = CountDownLatch(1)
    val releaseUse = CountDownLatch(1)
    val closeReturned = AtomicBoolean(false)
    val destroyedDuringUse = AtomicBoolean(false)

    val useThread = Thread {
      state.withLive {
        entered.countDown()
        assertTrue(releaseUse.await(5, TimeUnit.SECONDS))
        if (destroys != 0) destroyedDuringUse.set(true)
      }
    }
    useThread.start()
    assertTrue(entered.await(5, TimeUnit.SECONDS))

    val closeThread = Thread {
      state.closeOnce(
        destroy = {
          destroys += 1
          MaplibreStatus.OK.nativeCode
        }
      )
      closeReturned.set(true)
    }
    closeThread.start()

    Thread.sleep(50)
    assertFalse(closeReturned.get(), "close should wait for the use in flight")
    assertEquals(0, destroys, "the handle should still be live during the use")

    releaseUse.countDown()
    closeThread.join(5_000)
    useThread.join(5_000)

    assertTrue(closeReturned.get())
    assertEquals(1, destroys)
    assertFalse(destroyedDuringUse.get(), "the use should never observe a destroyed handle")
    assertTrue(state.isReleased())
  }
}
