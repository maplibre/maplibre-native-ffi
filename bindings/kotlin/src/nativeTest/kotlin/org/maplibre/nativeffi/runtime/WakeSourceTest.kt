package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.test.Test
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertTrue
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
  // BND-088, BND-089: park-and-wake behavior and wake source lifetime.

  // Leaves the runtime idle with no latched signal, so a following park can only be released by
  // the signal the test raises.
  private fun drainLatchedWakes(runtime: RuntimeHandle) {
    repeat(100) {
      if (!runtime.waitForWork(0)) {
        return
      }
      runtime.runOnce()
      while (runtime.pollEvent() != null) {
        // Drain.
      }
    }
    error("the runtime kept latching wakes while idle")
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
    drainLatchedWakes(runtime)

    // The style is malformed, so native reports the failure from its own threads. What matters
    // here is that the failure reaches a parked owner thread at all.
    map.setStyleUrl("unsupported://style.json")
    var loadingFailed = false
    repeat(20) {
      if (!loadingFailed) {
        assertTrue(runtime.waitForWork(10_000), "a park timed out while the style load was pending")
        runtime.runOnce()
        while (true) {
          val event = runtime.pollEvent() ?: break
          if (event.type == RuntimeEventType.MAP_LOADING_FAILED) {
            loadingFailed = true
          }
        }
      }
    }
    assertTrue(loadingFailed)

    // A source signalled from another thread is what a host's submission path holds, and the park
    // it releases has no other work to end it.
    val source = runtime.acquireWakeSource()
    drainLatchedWakes(runtime)
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
      assertTrue(
        runtime.waitForWork(10_000),
        "the parked owner thread timed out instead of taking the signal",
      )
      pthread_join(thread.ptr[0], null)
    }
    signalError.load()?.let { throw AssertionError("wake source signal failed", it) }

    // A wake source stays usable once its runtime is gone, so host teardown ordering is free.
    map.close()
    runtime.close()
    source.signal()
    source.close()
    assertTrue(source.isClosed)
    assertFailsWith<InvalidStateException> { source.signal() }
  }

  @Test
  fun waitConsumesOneLatchedSignalAtATime() {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      runtime.acquireWakeSource().use { source ->
        drainLatchedWakes(runtime)

        source.signal()
        assertTrue(runtime.waitForWork(0))
        // The latch is consumed, so an idle runtime reports the timeout instead.
        assertFalse(runtime.waitForWork(0))
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
