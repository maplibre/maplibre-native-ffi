package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.AtomicInt
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

/** Platform-neutral state for one owner-thread offline operation. */
@OptIn(ExperimentalAtomicApi::class)
internal class OfflineOperationHandleCore(
  private val runtimeIdentity: Any,
  val id: Long,
  val kind: OfflineOperationKind,
  val resultKind: OfflineOperationResultKind,
  private val releaseRuntimeRetention: () -> Unit,
) {
  val leakReport = OfflineOperationLeakReport(id, kind, resultKind)
  private val closed = AtomicInt(0)

  init {
    require(id != 0L) { "offline operation id must not be zero" }
  }

  val isClosed: Boolean
    get() = closed.load() != 0

  fun requireLive(expectedRuntime: Any): Long {
    if (isClosed) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "OfflineOperationHandle is already closed",
      )
    }
    if (runtimeIdentity !== expectedRuntime) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "OfflineOperationHandle belongs to a different RuntimeHandle",
      )
    }
    return id
  }

  fun requireLive(
    expectedRuntime: Any,
    expectedKind: OfflineOperationKind,
    expectedResultKind: OfflineOperationResultKind,
  ): Long {
    val operationId = requireLive(expectedRuntime)
    if (kind != expectedKind || resultKind != expectedResultKind) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "OfflineOperationHandle has kind $kind/$resultKind, " +
          "expected $expectedKind/$expectedResultKind",
      )
    }
    return operationId
  }

  /** Closes this operation, returning true for the call that closed it. */
  fun markConsumed(): Boolean {
    if (!closed.compareAndSet(0, 1)) return false
    leakReport.markClosed()
    releaseRuntimeRetention()
    return true
  }
}
