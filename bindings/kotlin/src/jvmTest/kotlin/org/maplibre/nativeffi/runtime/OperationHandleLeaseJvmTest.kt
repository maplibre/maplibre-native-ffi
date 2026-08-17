package org.maplibre.nativeffi.runtime

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.concurrent.thread
import kotlin.test.Test
import kotlin.test.assertFalse
import kotlin.test.assertTrue

class OperationHandleLeaseJvmTest {
  @Test
  fun closeWaitsForStartedNativeUse(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      val runtime = Any()
      val enteredUse = CountDownLatch(1)
      val releaseUse = CountDownLatch(1)
      val closeReturned = CountDownLatch(1)
      val core =
        OperationHandleCore(runtime, 7L, OperationKind.REGION_CREATE, OperationResultKind.REGION)

      val user = thread {
        core.withUse(runtime) {
          enteredUse.countDown()
          releaseUse.await()
        }
      }
      assertTrue(enteredUse.await(5, TimeUnit.SECONDS))

      val closer = thread {
        assertTrue(core.beginClose())
        core.finishClose()
        closeReturned.countDown()
      }
      assertFalse(closeReturned.await(50, TimeUnit.MILLISECONDS))

      releaseUse.countDown()
      assertTrue(closeReturned.await(5, TimeUnit.SECONDS))
      user.join()
      closer.join()
      assertTrue(core.isClosed)
    }
}
