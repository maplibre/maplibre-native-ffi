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
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.ResourceRequestCancelRegistry
import org.maplibre.nativeffi.internal.lifecycle.SyntheticHandles
import org.maplibre.nativeffi.internal.loader.NativeAccess

class ResourceRequestHandleJvmTest {
  @Test
  fun unreachableProviderOwnedHandleReleasesNativeRequest() {
    val released = CountDownLatch(1)

    registerUnreachableProviderOwnedHandle(released)

    assertTrue(awaitRelease(released), "expected unreachable request cleanup to release native")
  }

  @Test
  fun completionFailureIsTerminalAndCopiesDiagnosticBeforeReleaseCleanup() {
    NativeAccess.ensureLoaded()
    val releases = AtomicInteger(0)
    val handle =
      ResourceRequestHandle(
        SyntheticHandles.resourceRequest(),
        completer = { _, _ -> MapLibreNativeC.mln_network_status_set(999_999) },
        releaser = {
          releases.incrementAndGet()
          MapLibreNativeC.mln_runtime_destroy(0L)
        },
      )
    assertEquals(
      ResourceProviderDecision.HANDLE.nativeValue,
      handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
    )

    val failure =
      assertFailsWith<InvalidArgumentException> {
        handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
      }
    assertTrue(failure.diagnostic.contains("network status"))
    assertFalse(failure.diagnostic.contains("runtime"))
    assertEquals(1, releases.get())
    assertFailsWith<InvalidStateException> {
      handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT))
    }
  }

  @Test
  fun concurrentCloseDefersReleaseUntilCompletionAndCancellationUsesInjectedCheck() {
    val entered = CountDownLatch(1)
    val continueCompletion = CountDownLatch(1)
    val releases = AtomicInteger(0)
    val handle =
      ResourceRequestHandle(
        SyntheticHandles.resourceRequest(),
        completer = { _, _ ->
          entered.countDown()
          continueCompletion.await(5, TimeUnit.SECONDS)
          MaplibreStatus.OK.nativeCode
        },
        cancellationChecker = { true },
        releaser = { releases.incrementAndGet() },
      )
    assertEquals(
      ResourceProviderDecision.HANDLE.nativeValue,
      handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
    )
    assertTrue(handle.isCancelled())
    val completion = thread { handle.complete(ResourceResponse(ResourceResponseStatus.NO_CONTENT)) }
    assertTrue(entered.await(5, TimeUnit.SECONDS))
    handle.close()
    assertEquals(0, releases.get())
    continueCompletion.countDown()
    completion.join()
    assertEquals(1, releases.get())
    assertFailsWith<InvalidStateException> { handle.isCancelled() }
  }

  // BND-198.
  @Test
  fun closedHandleRejectsCancelCallbackRegistrationBeforeReachingNative() {
    val registrations = AtomicInteger(0)
    val handle =
      ResourceRequestHandle(
        SyntheticHandles.resourceRequest(),
        cancelCallbackSetter = { _, _, _ ->
          registrations.incrementAndGet()
          MaplibreStatus.OK.nativeCode
        },
        releaser = {},
      )
    assertEquals(
      ResourceProviderDecision.HANDLE.nativeValue,
      handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
    )

    handle.setCancelCallback {}
    handle.setCancelCallback(null)
    assertEquals(2, registrations.get())

    handle.close()

    assertFailsWith<InvalidStateException> { handle.setCancelCallback {} }
    assertEquals(2, registrations.get())
  }

  // BND-198.
  @Test
  fun unreachableHandleWithSelfCapturingCancelCallbackReleasesNativeRequest() {
    val released = CountDownLatch(1)

    registerUnreachableHandleWithSelfCapturingCancelCallback(released)

    assertTrue(awaitRelease(released), "expected unreachable request cleanup to release native")
  }

  // BND-198.
  @Test
  fun providerFailureDropsCancelRouting() {
    val releases = AtomicInteger(0)
    val token = AtomicLong(0)
    val handle =
      ResourceRequestHandle(
        SyntheticHandles.resourceRequest(),
        cancelCallbackSetter = { _, _, userData ->
          token.set(userData.address())
          MaplibreStatus.OK.nativeCode
        },
        releaser = { releases.incrementAndGet() },
      )

    handle.setCancelCallback {}
    assertTrue(ResourceRequestCancelRegistry.isRegisteredForTesting(token.get()))

    assertEquals(-1, handle.finishProviderException())

    assertFalse(ResourceRequestCancelRegistry.isRegisteredForTesting(token.get()))
    assertEquals(0, releases.get())
  }

  private fun registerUnreachableHandleWithSelfCapturingCancelCallback(released: CountDownLatch) {
    val handle =
      ResourceRequestHandle(
        SyntheticHandles.resourceRequest(),
        cancelCallbackSetter = { _, _, _ -> MaplibreStatus.OK.nativeCode },
        releaser = { released.countDown() },
      )
    // A callback that closes its own request is the documented shape, and it must not keep the
    // handle reachable from the cancel registry.
    handle.setCancelCallback { handle.close() }
    assertEquals(
      ResourceProviderDecision.HANDLE.nativeValue,
      handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
    )
  }

  private fun registerUnreachableProviderOwnedHandle(released: CountDownLatch) {
    val handle =
      ResourceRequestHandle(SyntheticHandles.resourceRequest(), releaser = { released.countDown() })
    assertEquals(
      ResourceProviderDecision.HANDLE.nativeValue,
      handle.finishProviderDecision(ResourceProviderDecision.HANDLE),
    )
  }

  private fun awaitRelease(released: CountDownLatch): Boolean {
    repeat(ATTEMPTS) {
      if (released.await(POLL_MILLIS, TimeUnit.MILLISECONDS)) return true
      System.gc()
    }
    return released.count == 0L
  }

  private companion object {
    private const val ATTEMPTS = 100
    private const val POLL_MILLIS = 20L
  }
}
