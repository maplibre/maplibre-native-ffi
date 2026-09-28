// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class AcquiredFrameHandle
internal constructor(
  private val handle: Long,
  parent: RenderSessionHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::acquiredFrame,
) : GeneratedAcquiredFrameOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "AcquiredFrameHandle",
      handle,
      parent,
      dispose = dispose,
    )

  init {
    org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingAcquiredFrameHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun bindingCloseAcquiredFrame(call: (Long) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    release()
  }
}
