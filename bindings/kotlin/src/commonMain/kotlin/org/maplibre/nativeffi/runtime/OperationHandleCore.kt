package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.yieldWhileClosing

/** Thread-safe state and active-use leasing for one native operation observer. */
@OptIn(ExperimentalAtomicApi::class)
internal class OperationHandleCore(
  private val runtimeIdentity: Any,
  internal val id: Long,
  internal val kind: OperationKind,
  internal val resultKind: OperationResultKind,
) {
  val leakReport = OperationLeakReport()
  private val state = AtomicInt(0)
  private val resultConsumed = AtomicInt(0)

  init {
    require(id != 0L) { "operation handle must not be zero" }
  }

  val isClosed: Boolean
    get() = state.load() < 0

  fun <T> withUse(expectedRuntime: Any, block: (Long) -> T): T {
    acquireUse(expectedRuntime)
    return try {
      block(id)
    } finally {
      state.fetchAndAdd(-1)
    }
  }

  fun <T> withUse(
    expectedRuntime: Any,
    expectedKind: OperationKind,
    expectedResultKind: OperationResultKind,
    block: (Long) -> T,
  ): T =
    withUse(expectedRuntime) { operation ->
      if (kind != expectedKind || resultKind != expectedResultKind) {
        throw InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode,
          "OperationHandle has incompatible result type",
        )
      }
      if (resultConsumed.load() != 0) {
        throw InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode,
          "OperationHandle result is already consumed",
        )
      }
      block(operation)
    }

  fun markResultConsumed() {
    resultConsumed.store(1)
  }

  fun hasConsumedResult(): Boolean = resultConsumed.load() != 0

  /** Prevents new uses and waits until every use that already started has returned. */
  fun beginClose(): Boolean {
    while (true) {
      val current = state.load()
      if (current < 0) return false
      if (state.compareAndSet(current, current or CLOSED_BIT)) {
        while (state.load() != CLOSED_BIT) {
          yieldWhileClosing()
        }
        return true
      }
    }
  }

  fun finishClose() {
    leakReport.markClosed()
  }

  private fun acquireUse(expectedRuntime: Any) {
    if (runtimeIdentity !== expectedRuntime) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "OperationHandle belongs to a different RuntimeHandle",
      )
    }
    while (true) {
      val current = state.load()
      if (current < 0) {
        throw InvalidStateException(
          MaplibreStatus.INVALID_STATE.nativeCode,
          "OperationHandle is already closed",
        )
      }
      if (state.compareAndSet(current, current + 1)) return
    }
  }

  private companion object {
    const val CLOSED_BIT: Int = Int.MIN_VALUE
  }
}
