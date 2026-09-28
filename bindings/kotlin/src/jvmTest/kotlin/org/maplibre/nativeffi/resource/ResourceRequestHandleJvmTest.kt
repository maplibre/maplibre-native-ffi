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
import org.maplibre.nativeffi.generated.ResourceRequestHandle
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.DecisionCancelRegistry
import org.maplibre.nativeffi.internal.callback.DecisionCancelSetResult
import org.maplibre.nativeffi.internal.lifecycle.SyntheticHandles
import org.maplibre.nativeffi.internal.loader.NativeAccess

class ResourceRequestHandleJvmTest {
  @Test
  fun rejectedCompletionPreservesOwnerAndAllowsRetry() {
    NativeAccess.ensureLoaded()
    var releases = 0
    val handle = ResourceRequestHandle(SyntheticHandles.resourceRequest(), dispose = { releases++ })
    handle.finishBindingDecision(ResourceProviderDecision.HANDLE.rawValue)
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
        dispose = { releases.incrementAndGet() },
      )
    handle.finishBindingDecision(ResourceProviderDecision.HANDLE.rawValue)
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
      ResourceRequestHandle(SyntheticHandles.resourceRequest(), dispose = { released.countDown() })
    handle.bindingRegisterResourceRequestHandleCancel({ handle.close() }) { _, _ ->
      DecisionCancelSetResult(0, false)
    }
    handle.finishBindingDecision(ResourceProviderDecision.HANDLE.rawValue)
  }

  @Test
  fun providerPassThroughDisarmsCancellationWithoutReleasingTwice() {
    val token = AtomicLong(0)
    var releases = 0
    val handle = ResourceRequestHandle(SyntheticHandles.resourceRequest(), dispose = { releases++ })
    handle.bindingRegisterResourceRequestHandleCancel({}) { _, registered ->
      token.set(registered)
      DecisionCancelSetResult(0, false)
    }
    assertTrue(DecisionCancelRegistry.isRegisteredForTesting(token.get()))
    assertEquals(
      ResourceProviderDecision.PASS_THROUGH.rawValue,
      handle.finishBindingDecision(ResourceProviderDecision.PASS_THROUGH.rawValue),
    )
    assertFalse(DecisionCancelRegistry.isRegisteredForTesting(token.get()))
    handle.close()
    assertEquals(0, releases)
  }
}
