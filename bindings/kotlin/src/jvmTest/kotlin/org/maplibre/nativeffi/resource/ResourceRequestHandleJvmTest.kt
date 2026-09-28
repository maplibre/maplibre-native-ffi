package org.maplibre.nativeffi.resource

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicLong
import kotlin.concurrent.thread
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelRegistry
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelSetResult
import org.maplibre.nativeffi.internal.lifecycle.SyntheticHandles
import org.maplibre.nativeffi.internal.loader.NativeAccess

class ResourceRequestHandleJvmTest {
  @Test
  fun rejectedCompletionPreservesOwnerAndAllowsRetry() {
    NativeAccess.ensureLoaded()
    var releases = 0
    val handle =
      ResourceRequestHandle(SyntheticHandles.resourceRequest(), releaser = { releases++ })
    handle.finishProviderDecision(ResourceProviderDecision.HANDLE)
    val failure =
      assertFailsWith<InvalidArgumentException> {
        handle.bindingCompleteResourceRequestHandle {
          MapLibreNativeC.mln_network_status_set(999_999)
        }
      }
    assertTrue(failure.diagnostic.contains("network status"))
    assertEquals(0, releases)
    handle.bindingCompleteResourceRequestHandle { 0 }
    assertFailsWith<InvalidStateException> { handle.bindingCompleteResourceRequestHandle { 0 } }
    assertEquals(0, releases)
    handle.close()
    assertEquals(1, releases)
  }

  @Test
  fun concurrentCloseDefersReleaseUntilCompletionReturns() {
    val entered = CountDownLatch(1)
    val leave = CountDownLatch(1)
    val releases = AtomicInteger(0)
    val handle =
      ResourceRequestHandle(
        SyntheticHandles.resourceRequest(),
        releaser = { releases.incrementAndGet() },
      )
    handle.finishProviderDecision(ResourceProviderDecision.HANDLE)
    val worker = thread {
      handle.bindingCompleteResourceRequestHandle {
        entered.countDown()
        check(leave.await(5, TimeUnit.SECONDS))
        0
      }
    }
    try {
      assertTrue(entered.await(5, TimeUnit.SECONDS))
      handle.close()
      handle.close()
      assertEquals(0, releases.get())
    } finally {
      leave.countDown()
      worker.join()
    }
    assertEquals(1, releases.get())
    assertFailsWith<InvalidStateException> { handle.bindingReadResourceRequestHandle {} }
  }

  @Test
  fun unreachableHandleWithSelfCapturingCancelCallbackReleasesNativeRequest() {
    val released = CountDownLatch(1)
    registerUnreachable(released)
    repeat(100) {
      if (released.await(20, TimeUnit.MILLISECONDS)) return
      System.gc()
    }
    assertEquals(0L, released.count, "self-capturing callback kept its owner reachable")
  }

  private fun registerUnreachable(released: CountDownLatch) {
    val handle =
      ResourceRequestHandle(SyntheticHandles.resourceRequest(), releaser = { released.countDown() })
    handle.bindingRegisterResourceRequestHandleCancel({ handle.close() }) { _, _ ->
      ResourceRequestCancelSetResult(0, false)
    }
    handle.finishProviderDecision(ResourceProviderDecision.HANDLE)
  }

  @Test
  fun providerPassThroughDisarmsCancellationWithoutReleasingTwice() {
    val token = AtomicLong(0)
    var releases = 0
    val handle =
      ResourceRequestHandle(SyntheticHandles.resourceRequest(), releaser = { releases++ })
    handle.bindingRegisterResourceRequestHandleCancel({}) { _, registered ->
      token.set(registered)
      ResourceRequestCancelSetResult(0, false)
    }
    assertTrue(ResourceRequestCancelRegistry.isRegisteredForTesting(token.get()))
    assertEquals(
      ResourceProviderDecision.PASS_THROUGH.rawValue.toInt(),
      handle.finishProviderDecision(ResourceProviderDecision.PASS_THROUGH),
    )
    assertFalse(ResourceRequestCancelRegistry.isRegisteredForTesting(token.get()))
    handle.close()
    assertEquals(0, releases)
  }
}
