package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

/** Owner-thread offline database operation backed by the JVM FFM bridge. */
public actual class OfflineOperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  public actual val id: Long,
  public actual val kind: OfflineOperationKind,
  public actual val resultKind: OfflineOperationResultKind,
) : AutoCloseable {
  private val runtimeRetention: HandleStateCore.ChildRetention = runtime.retainChild()
  private var closed = false

  init {
    require(id != 0L) { "offline operation id must not be zero" }
  }

  public actual val isClosed: Boolean
    get() = closed

  internal fun requireLive(expectedRuntime: RuntimeHandle): Long {
    if (closed) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "OfflineOperationHandle is already closed",
      )
    }
    if (runtime !== expectedRuntime) {
      throw InvalidStateException(
        MaplibreStatus.INVALID_STATE.nativeCode,
        "OfflineOperationHandle belongs to a different RuntimeHandle",
      )
    }
    return id
  }

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
