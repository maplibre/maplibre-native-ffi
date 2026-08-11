package org.maplibre.nativeffi.runtime

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.Cleaner
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
public actual class OfflineOperationHandle<T>
internal constructor(
  private val runtime: RuntimeHandle,
  nativeId: ULong,
  kind: OfflineOperationKind,
  resultKind: OfflineOperationResultKind,
) : AutoCloseable {
  private val runtimeRetention = runtime.retainChild("OfflineOperationHandle")
  private val core =
    OfflineOperationHandleCore(
      runtime,
      nativeId.toLong(),
      kind,
      resultKind,
      runtimeRetention::close,
    )
  @Suppress("unused") private val cleaner: Cleaner = createCleaner(core.leakReport) { it.report() }

  public actual val id: Long
    get() = core.id

  public actual val kind: OfflineOperationKind
    get() = core.kind

  public actual val resultKind: OfflineOperationResultKind
    get() = core.resultKind

  public actual val isClosed: Boolean
    get() = core.isClosed

  internal fun requireLive(expectedRuntime: RuntimeHandle): ULong =
    core.requireLive(expectedRuntime).toULong()

  internal fun requireLive(
    expectedRuntime: RuntimeHandle,
    expectedKind: OfflineOperationKind,
    expectedResultKind: OfflineOperationResultKind,
  ): ULong = core.requireLive(expectedRuntime, expectedKind, expectedResultKind).toULong()

  internal fun markConsumed() {
    core.markConsumed()
  }

  public actual override fun close() {
    if (!isClosed) runtime.discardOfflineOperation(this)
  }
}
