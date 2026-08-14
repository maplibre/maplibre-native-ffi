package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status

/** Common operation observer backed by the JVM FFM bridge. */
public actual class OperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  id: Long,
  kind: OperationKind,
  resultKind: OperationResultKind,
  ownerRetention: HandleStateCore.ChildRetention? = null,
) : AutoCloseable {
  private val runtimeRetention = runtime.retainChild("OperationHandle")
  private val core =
    OperationHandleCore(runtime, id, kind, resultKind) {
      ownerRetention?.close()
      runtimeRetention.close()
    }

  init {
    HandleLeakCleaner.registerOperation(this, core.leakReport)
  }

  public actual val isClosed: Boolean
    get() = core.isClosed

  internal fun <R> withUse(block: (Long) -> R): R = core.withUse(runtime, block)

  internal fun <R> withResultUse(
    expectedKind: OperationKind,
    expectedResultKind: OperationResultKind,
    block: (Long) -> R,
  ): R = core.withUse(runtime, expectedKind, expectedResultKind, block)

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

  public actual fun discard() {
    withUse {
      Status.check(NativeAccess.discardOperation(it))
      core.markResultConsumed()
    }
  }

  public actual override fun close() {
    if (!core.beginClose()) return
    runtime.forgetOperation(core.id)
    NativeAccess.releaseOperation(core.id)
    core.finishClose()
  }
}
