// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class EventBatchHandle
internal constructor(
  private val handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::eventBatch,
) : GeneratedEventBatchOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "EventBatchHandle",
      handle,
      dispose = dispose,
    )

  init {
    org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingEventBatchHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun <T> bindingReadEventBatch(block: (Long) -> T): T = core.withLive {
    block(handle)
  }

  internal override fun bindingCloseEventBatch(call: (Long) -> Unit) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    release()
  }
}
