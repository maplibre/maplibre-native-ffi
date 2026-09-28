// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

public actual class EventBatchHandle internal constructor(private val handle: Long) :
  GeneratedEventBatchOperations(), AutoCloseable {
  private val core =
    HandleStateCore("EventBatchHandle", handle, dispose = GeneratedOwnerDisposal::eventBatch)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingEventBatchHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun <T> bindingReadEventBatch(block: (Long) -> T): T = core.withLive {
    block(handle)
  }

  internal override fun bindingCloseEventBatch(call: (Long) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce({
      GeneratedOwnerDisposal.eventBatch(handle)
      0
    })
  }
}
