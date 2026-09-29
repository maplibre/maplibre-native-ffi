// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class RenderFrameBatchHandle
internal constructor(
  private val handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::renderFrameBatch,
) : GeneratedRenderFrameBatchOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "RenderFrameBatchHandle",
      handle,
      dispose = dispose,
    )

  init {
    org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingRenderFrameBatchHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseRenderFrameBatch(call: (Long) -> Unit) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    release()
  }
}
