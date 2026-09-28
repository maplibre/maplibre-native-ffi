// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

@OptIn(ExperimentalNativeApi::class)
public actual class EventBatchHandle internal constructor(private val handle: ULong) :
  GeneratedEventBatchOperations(), AutoCloseable {
  private val core =
    HandleStateCore(
      "EventBatchHandle",
      handle.toLong(),
      dispose = GeneratedOwnerDisposal::eventBatch,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingEventBatchHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun <T> bindingReadEventBatch(block: (ULong) -> T): T = core.withLive {
    block(handle)
  }

  internal override fun bindingCloseEventBatch(call: (ULong) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce({
      GeneratedOwnerDisposal.eventBatch(handle.toLong())
      0
    })
  }
}
