// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner

@OptIn(ExperimentalNativeApi::class)
public actual class EventBatchHandle
internal constructor(
  private val handle: ULong,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::eventBatch,
) : GeneratedEventBatchOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "EventBatchHandle",
      handle.toLong(),
      dispose = dispose,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingEventBatchHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun <T> bindingReadEventBatch(block: (ULong) -> T): T = core.withLive {
    block(handle)
  }

  internal override fun bindingCloseEventBatch(call: (ULong) -> Unit) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    release()
  }
}
