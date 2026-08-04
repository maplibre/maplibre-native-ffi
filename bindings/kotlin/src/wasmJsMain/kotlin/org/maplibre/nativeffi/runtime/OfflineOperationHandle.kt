package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

/**
 * One offline database operation the runtime's owner thread started.
 *
 * The wrapper owns nothing native beyond an id, so nothing here is dispatched: every call that
 * reaches the C API goes through the runtime, which is what places it on the owner thread. What
 * this does own is the requirement that the operation is eventually taken or discarded, because
 * until then the runtime holds its result.
 *
 * It retains its runtime for its whole life, so closing a runtime with operations still outstanding
 * reports them rather than stranding results the host can no longer reach.
 *
 * The other platforms also register a leak report here, which non-deterministic cleanup runs if a
 * host drops the wrapper. A browser has no such cleanup to hang one on, so a dropped operation is
 * simply held by the runtime until the runtime itself is closed.
 */
public actual class OfflineOperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  public actual val id: Long,
  public actual val kind: OfflineOperationKind,
  public actual val resultKind: OfflineOperationResultKind,
) : AutoCloseable {
  private val runtimeRetention: HandleStateCore.ChildRetention =
    runtime.retainChild("OfflineOperationHandle")
  private var closed = false

  init {
    require(id != 0L) { "offline operation id must not be zero" }
  }

  public actual val isClosed: Boolean
    get() = closed

  /** Reports this operation's id, refusing a wrapper that belongs to another runtime. */
  internal fun requireLive(expectedRuntime: RuntimeHandle): Long {
    if (closed) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "OfflineOperationHandle is already closed",
      )
    }
    // An id names one operation within one runtime, so passing this to another runtime would take
    // whatever operation happens to carry the same id there.
    if (runtime !== expectedRuntime) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "OfflineOperationHandle belongs to a different RuntimeHandle",
      )
    }
    return id
  }

  /**
   * Reports this operation's id, refusing a wrapper whose result is a different shape.
   *
   * The kinds are what make the take methods type-safe: the wrapper's type parameter is erased, so
   * this is what stops a region-list result being taken as a status.
   */
  internal fun requireLive(
    expectedRuntime: RuntimeHandle,
    expectedKind: OfflineOperationKind,
    expectedResultKind: OfflineOperationResultKind,
  ): Long {
    val operationId = requireLive(expectedRuntime)
    if (kind != expectedKind || resultKind != expectedResultKind) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "OfflineOperationHandle has kind $kind/$resultKind, expected $expectedKind/$expectedResultKind",
      )
    }
    return operationId
  }

  /** Retires this wrapper once native no longer holds a result for it. */
  internal fun markConsumed() {
    if (closed) return
    closed = true
    runtimeRetention.close()
  }

  public actual override fun close() {
    if (closed) return
    runtime.discardOfflineOperation(this)
  }
}
