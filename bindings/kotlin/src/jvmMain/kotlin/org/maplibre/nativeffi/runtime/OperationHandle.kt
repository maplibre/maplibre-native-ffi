package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status

/** Common operation observer backed by the JVM FFM bridge. */
public actual class OperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  id: Long,
  kind: OperationKind,
  resultKind: OperationResultKind,
) : AutoCloseable {
  private val core = OperationHandleCore(runtime, id, kind, resultKind)

  init {
    HandleLeakCleaner.registerOperation(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isClosed

  internal fun <R> withUse(block: (Long) -> R): R =
    try {
      core.withUse(runtime, block)
    } finally {
      retireConsumed()
    }

  internal fun <R> withResultUse(
    expectedKind: OperationKind,
    expectedResultKind: OperationResultKind,
    block: (Long) -> R,
  ): R =
    try {
      core.withUse(runtime, expectedKind, expectedResultKind, block)
    } finally {
      retireConsumed()
    }

  internal fun markResultConsumed() {
    core.markResultConsumed()
  }

  public actual fun poll(): Boolean = withUse(NativeAccess::pollOperation)

  public actual fun waitForCompletion(timeoutMillis: Long): Boolean = withUse {
    NativeAccess.waitOperation(it, timeoutMillis)
  }

  public actual fun cancel() {
    withUse { Status.check(NativeAccess.cancelOperation(it)) }
  }

  public actual fun terminalStatus(): MaplibreStatus = withUse {
    MaplibreStatus.fromNative(NativeAccess.operationTerminalStatus(it))
  }

  public actual fun diagnostic(): String = withUse(NativeAccess::operationDiagnostic)

  public actual fun finish() {
    withUse {
      Status.check(NativeAccess.finishOperation(it))
      core.markResultConsumed()
    }
  }

  public actual override fun close() {
    if (!core.beginClose()) return
    runtime.forgetOperation(core.id)
    NativeAccess.releaseOperation(core.id)
    core.finishClose()
  }

  private fun retireConsumed() {
    if (!core.hasConsumedResult() || !core.beginClose()) return
    runtime.forgetOperation(core.id)
    core.finishClose()
  }
}
