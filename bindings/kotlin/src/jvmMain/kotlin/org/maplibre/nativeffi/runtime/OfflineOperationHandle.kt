package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner

/** Owner-thread offline database operation backed by the JVM FFM bridge. */
public actual class OfflineOperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  id: Long,
  kind: OfflineOperationKind,
  resultKind: OfflineOperationResultKind,
) : AutoCloseable {
  private val runtimeRetention = runtime.retainChild("OfflineOperationHandle")
  private val core =
    OfflineOperationHandleCore(runtime, id, kind, resultKind, runtimeRetention::close)

  init {
    HandleLeakCleaner.registerOfflineOperation(this, core.leakReport)
  }

  public actual val id: Long
    get() = core.id

  public actual val kind: OfflineOperationKind
    get() = core.kind

  public actual val resultKind: OfflineOperationResultKind
    get() = core.resultKind

  public actual val isClosed: Boolean
    get() = core.isClosed

  internal fun requireLive(expectedRuntime: RuntimeHandle): Long = core.requireLive(expectedRuntime)

  internal fun requireLive(
    expectedRuntime: RuntimeHandle,
    expectedKind: OfflineOperationKind,
    expectedResultKind: OfflineOperationResultKind,
  ): Long = core.requireLive(expectedRuntime, expectedKind, expectedResultKind)

  internal fun markConsumed() = core.markConsumed()

  public actual override fun close() {
    if (!isClosed) runtime.discardOfflineOperation(this)
  }
}
