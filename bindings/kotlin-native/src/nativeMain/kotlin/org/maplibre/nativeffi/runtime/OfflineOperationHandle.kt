package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

/** Owner-thread offline database operation that must be taken or discarded. */
public class OfflineOperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  public val id: ULong,
  public val kind: OfflineOperationKind,
  public val resultKind: OfflineOperationResultKind,
) : AutoCloseable {
  private var closed = false

  init {
    require(id != 0UL) { "offline operation id must not be zero" }
  }

  public fun isClosed(): Boolean = closed

  internal fun requireLive(expectedRuntime: RuntimeHandle): ULong {
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

  internal fun markConsumed() {
    closed = true
  }

  override fun close() {
    runtime.discardOfflineOperation(this)
  }
}
