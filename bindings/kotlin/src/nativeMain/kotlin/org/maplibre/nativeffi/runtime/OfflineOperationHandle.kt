package org.maplibre.nativeffi.runtime

import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

@OptIn(ExperimentalAtomicApi::class, ExperimentalNativeApi::class)
public actual class OfflineOperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  private val nativeId: ULong,
  public actual val kind: OfflineOperationKind,
  public actual val resultKind: OfflineOperationResultKind,
) : AutoCloseable {
  /** Native `uint64_t` operation id preserved as a [Long] bit pattern. */
  public actual val id: Long = uint64BitsToLong(nativeId)
  private val runtimeRetention: HandleStateCore.ChildRetention =
    runtime.retainChild("OfflineOperationHandle")
  private val leakReport = OfflineOperationLeakReport(id, kind, resultKind)
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(leakReport) { it.report() }
  private var closed = false

  init {
    require(nativeId != 0UL) { "offline operation id must not be zero" }
  }

  public actual val isClosed: Boolean
    get() = closed

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
    if (closed) return
    closed = true
    leakReport.markClosed()
    runtimeRetention.close()
  }

  public actual override fun close() {
    if (closed) return
    runtime.discardOfflineOperation(this)
  }

  private fun uint64BitsToLong(value: ULong): Long = value.toLong()
}
