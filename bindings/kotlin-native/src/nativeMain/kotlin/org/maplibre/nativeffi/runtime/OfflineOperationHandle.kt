package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus

/** Owner-thread offline database operation that must be taken or discarded. */
public class OfflineOperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  private val nativeId: ULong,
  public val kind: OfflineOperationKind,
  public val resultKind: OfflineOperationResultKind,
) : AutoCloseable {
  /** Native uint64 operation id preserved as a Java-compatible [Long] bit pattern. */
  public val id: Long = uint64BitsToLong(nativeId)
  private var closed = false

  init {
    require(nativeId != 0UL) { "offline operation id must not be zero" }
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
    return nativeId
  }

  internal fun requireLive(
    expectedRuntime: RuntimeHandle,
    expectedKind: OfflineOperationKind,
    expectedResultKind: OfflineOperationResultKind,
  ): ULong {
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
    closed = true
  }

  override fun close() {
    runtime.discardOfflineOperation(this)
  }

  private fun uint64BitsToLong(value: ULong): Long = value.toLong()
}
