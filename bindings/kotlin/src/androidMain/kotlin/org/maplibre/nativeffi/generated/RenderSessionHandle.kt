// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class RenderSessionHandle
internal constructor(
  private val handle: Long,
  parent: MapHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::renderSession,
) : GeneratedRenderSessionOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "RenderSessionHandle",
      handle,
      parent,
      dispose = dispose,
    )

  init {
    org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingRenderSessionHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseRenderSession(call: (Long) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    destroy()
  }
}
