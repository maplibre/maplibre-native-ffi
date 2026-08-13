package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertTrue
import kotlin.time.TimeSource
import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.StableRef
import kotlinx.cinterop.alloc
import kotlinx.cinterop.asStableRef
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.staticCFunction
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapOptions
import platform.posix.pthread_create
import platform.posix.pthread_join
import platform.posix.pthread_tVar
import platform.posix.usleep

@OptIn(ExperimentalAtomicApi::class, ExperimentalForeignApi::class)
class WakeSourceTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-088, BND-089.

  // Pumps until the runtime is idle, so a later park is released only by the test's signal.
  private fun quiesce(runtime: RuntimeHandle) {
    repeat(100) {
      runtime.pump(0)
      if (runtime.drainEvents().events.isEmpty()) {
        return
      }
    }
    error("the runtime kept producing events while idle")
  }

  @Test
  fun parkedOwnerThreadWakesForNativeWorkAndForAWakeSource() {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          width = 512
          height = 512
        },
      )
    quiesce(runtime)

    // Native reports the load failure from its own threads; it must reach the parked
    // owner thread.
    map.setStyleUrl("unsupported://style.json")
    var loadingFailed = false
    val loadStarted = TimeSource.Monotonic.markNow()
    repeat(20) {
      if (!loadingFailed) {
        runtime.pump(10_000)
        assertTrue(
          loadStarted.elapsedNow().inWholeMilliseconds < 5_000,
          "parks sat out their timeouts while the style load was pending",
        )
        if (runtime.drainEvents().events.any { it.type == RuntimeEventType.MAP_LOADING_FAILED }) {
          loadingFailed = true
        }
      }
    }
    assertTrue(loadingFailed)

    // Nothing else can end this park, so only the cross-thread signal releases it.
    val source = runtime.acquireWakeSource()
    quiesce(runtime)
    val signalError = AtomicReference<Throwable?>(null)
    memScoped {
      val signal = BackgroundWakeSignal(source, signalError)
      val selfRef = StableRef.create(signal)
      val thread = alloc<pthread_tVar>()
      val status =
        pthread_create(
          thread.ptr,
          null,
          staticCFunction(::signalWakeSourceOnNativeThread),
          selfRef.asCPointer(),
        )
      if (status != 0) {
        selfRef.dispose()
        error("pthread_create failed with status $status")
      }
      val parkStarted = TimeSource.Monotonic.markNow()
      runtime.pump(10_000)
      assertTrue(
        parkStarted.elapsedNow().inWholeMilliseconds < 5_000,
        "the parked owner thread timed out instead of taking the signal",
      )
      pthread_join(thread.ptr[0], null)
    }
    signalError.load()?.let { throw AssertionError("wake source signal failed", it) }

    // A wake source stays usable after its runtime closes, in either teardown order.
    map.close()
    runtime.close()
    source.signal()
    source.close()
    assertTrue(source.isClosed)
    assertFailsWith<InvalidStateException> { source.signal() }
  }

  @Test
  fun pumpClearsTheWakeFlagItReturnsOn() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      runtime.acquireWakeSource().use { source ->
        quiesce(runtime)

        source.signal()
        val signalledStarted = TimeSource.Monotonic.markNow()
        runtime.pump(10_000)
        assertTrue(
          signalledStarted.elapsedNow().inWholeMilliseconds < 5_000,
          "a pump waited even though the wake flag was set",
        )

        // The pump above cleared the wake flag, so this one waits its full timeout.
        val idleStarted = TimeSource.Monotonic.markNow()
        runtime.pump(200)
        assertTrue(
          idleStarted.elapsedNow().inWholeMilliseconds >= 100,
          "the first pump left the wake flag set",
        )
      }
    }
  }
}

@OptIn(ExperimentalAtomicApi::class)
private class BackgroundWakeSignal(
  private val source: WakeSource,
  private val error: AtomicReference<Throwable?>,
) {
  fun run() {
    try {
      usleep(20_000u)
      source.signal()
    } catch (throwable: Throwable) {
      error.store(throwable)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private fun signalWakeSourceOnNativeThread(raw: COpaquePointer?): COpaquePointer? {
  val selfRef = requireNotNull(raw).asStableRef<BackgroundWakeSignal>()
  try {
    selfRef.get().run()
  } finally {
    selfRef.dispose()
  }
  return null
}
