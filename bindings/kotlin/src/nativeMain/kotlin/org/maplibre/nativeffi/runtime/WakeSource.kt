package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlinx.cinterop.ExperimentalForeignApi
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_wake_source_destroy
import org.maplibre.nativeffi.internal.c.mln_wake_source_signal
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.NativeWakeSource
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.status.Status
import platform.posix.sched_yield

/** Owned wake source. Signal and close it from any thread. */
@OptIn(ExperimentalForeignApi::class, ExperimentalAtomicApi::class)
public actual class WakeSource internal constructor(handle: NativeWakeSource) : AutoCloseable {
  // Held across the native signal and across close, so a close on another thread cannot destroy the
  // source between the live check and the call. Both critical sections are one short native call,
  // so a spin costs less than a mutex.
  private val nativeCallGate = AtomicInt(0)
  private val state = HandleState("WakeSource", handle)

  public actual val isClosed: Boolean
    get() = state.isReleased()

  public actual fun signal() {
    lockNativeCallGate()
    try {
      Status.check(mln_wake_source_signal(state.requireLive().rawHandleValue))
    } finally {
      nativeCallGate.store(0)
    }
  }

  public actual override fun close() {
    lockNativeCallGate()
    try {
      state.closeOnce { source ->
        mln_wake_source_destroy(source.rawHandleValue)
        MaplibreStatus.OK.nativeCode
      }
    } finally {
      nativeCallGate.store(0)
    }
  }

  private fun lockNativeCallGate() {
    while (!nativeCallGate.compareAndSet(0, 1)) {
      sched_yield()
    }
  }
}
