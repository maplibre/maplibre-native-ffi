package org.maplibre.nativeffi.render

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

/** Tracks the one active session-owned texture frame borrow for a render session. */
@OptIn(ExperimentalAtomicApi::class)
internal class ActiveFrameState {
  private val active = AtomicInt(0)

  fun beginAcquire() {
    if (!active.compareAndSet(0, 1)) {
      throw activeFrameError("acquire another texture frame")
    }
  }

  fun ensureInactive(operation: String) {
    if (active.load() != 0) {
      throw activeFrameError(operation)
    }
  }

  fun endBorrow() {
    active.store(0)
  }

  private fun activeFrameError(operation: String): InvalidStateException =
    InvalidStateException(
      MaplibreStatus.INVALID_STATE.nativeCode,
      "RenderSessionHandle cannot $operation while a texture frame is acquired",
    )
}
