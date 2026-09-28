// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

@OptIn(ExperimentalNativeApi::class)
public actual class RenderFrameBatchHandle internal constructor(private val handle: ULong) :
  GeneratedRenderFrameBatchOperations(), AutoCloseable {
  private val core =
    HandleStateCore(
      "RenderFrameBatchHandle",
      handle.toLong(),
      dispose = GeneratedOwnerDisposal::renderFrameBatch,
    )
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingRenderFrameBatchHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseRenderFrameBatch(call: (ULong) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce({
      GeneratedOwnerDisposal.renderFrameBatch(handle.toLong())
      0
    })
  }
}
