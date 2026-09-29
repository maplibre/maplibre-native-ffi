package org.maplibre.nativeffi.resource

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
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
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.ResourceRequestHandle
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.lifecycle.SyntheticHandles
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use

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
          NativeDiagnostics.check { diagnostic ->
            MapLibreNativeC.mln_network_status_set(999_999, diagnostic)
          }
        }
      }
    assertTrue(failure.diagnostic.contains("network status"))
    assertEquals(0, releases)
    handle.bindingCompleteResourceRequestHandle {}
    assertFailsWith<InvalidStateException> { handle.bindingCompleteResourceRequestHandle {} }
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
  fun unreachableHandleWithSelfCapturingCancelCallbackReleasesNativeRequest(): Unit =
    runSuspendTest {
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
              System.gc()
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
    Arena.ofConfined().use { arena ->
      MapLibreNativeC.mln_resource_request_cancelled(
        raw,
        arena.allocate(ValueLayout.JAVA_BOOLEAN),
        MemorySegment.NULL,
      ) != MaplibreStatus.OK.nativeCode
    }
}

private const val SELF_CAPTURING_URL = "custom://self-capturing-cancel-style.json"
