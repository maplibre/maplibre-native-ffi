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
import kotlinx.coroutines.runBlocking
import org.bytedeco.javacpp.BoolPointer
import org.bytedeco.javacpp.PointerScope
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.ResourceRequestHandle
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.runtime.use

class ResourceRequestHandleAndroidTest {
  @Test
  fun rejectedCompletionPreservesOwnerAndAllowsRetry() {
    var releases = 0
    val handle = ResourceRequestHandle(1L, dispose = { releases++ })
    handle.finishBindingDecision(ResourceProviderDecision.HANDLE.rawValue)
    val failure =
      assertFailsWith<InvalidArgumentException> {
        handle.bindingCompleteResourceRequestHandle {
          MaplibreNativeC.mln_network_status_set(999_999)
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
    val handle = ResourceRequestHandle(1L, dispose = { releases.incrementAndGet() })
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
  fun unreachableHandleWithSelfCapturingCancelCallbackReleasesNativeRequest(): Unit = runBlocking {
    GeneratedApi.runtimeCreate(GeneratedApi.runtimeOptionsDefault()).use { runtime ->
      val raw = AtomicLong(0)
      runtime.setResourceProvider(selfCapturingCancelProvider(raw)).await()
      val map =
        runtime
          .mapCreate(
            GeneratedApi.mapOptionsDefault()
              .copy(
                initialExtent =
                  GeneratedApi.mapOptionsDefault().initialExtent.copy(width = 64u, height = 64u)
              )
          )
          .await()
      try {
        map.setStyleUrl(SELF_CAPTURING_URL).await()
        var rounds = 0
        while (raw.get() == 0L && rounds++ < 10_000) {
          runtime.barrier().await()
          Thread.sleep(1)
        }
        assertTrue(raw.get() != 0L, "resource provider did not receive the request")
        // Only the handle's own cancel callback captures it, and the map keeps the request live.
        val released =
          (0 until 100).any {
            // System.gc() defers collection until runFinalization() on older Android releases.
            Runtime.getRuntime().gc()
            Thread.sleep(20)
            isReleased(raw.get())
          }
        assertTrue(released, "self-capturing cancel callback kept its owner reachable")
      } finally {
        map.release().await()
      }
    }
  }

  private fun selfCapturingCancelProvider(raw: AtomicLong) =
    ResourceProvider(
      callback = provider@{ request, handle ->
          if (request.requestedUrl != SELF_CAPTURING_URL) {
            return@provider ResourceProviderDecision.PASS_THROUGH
          }
          assertFalse(handle.resourceRequestSetCancelCallback { handle.close() })
          raw.set(handle.bindingIssuedResourceRequestHandleHandle())
          ResourceProviderDecision.HANDLE
        }
    )

  private fun isReleased(raw: Long): Boolean =
    PointerScope().use {
      MaplibreNativeC.mln_resource_request_cancelled(raw, BoolPointer(1L)) !=
        MaplibreStatus.OK.nativeCode
    }
}

private const val SELF_CAPTURING_URL = "custom://self-capturing-cancel-style.json"
