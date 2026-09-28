// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

public actual class RenderFrameBatchHandle internal constructor(private val handle: Long) :
  GeneratedRenderFrameBatchOperations(), AutoCloseable {
  private val core =
    HandleStateCore(
      "RenderFrameBatchHandle",
      handle,
      dispose = GeneratedOwnerDisposal::renderFrameBatch,
    )

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingRenderFrameBatchHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseRenderFrameBatch(call: (Long) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce({
      GeneratedOwnerDisposal.renderFrameBatch(handle)
      0
    })
  }
}
