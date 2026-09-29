// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

public actual class BufferHandle
internal constructor(
  private val handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::buffer,
) : GeneratedBufferOperations(), AutoCloseable {
  private val core =
    org.maplibre.nativeffi.internal.lifecycle.HandleStateCore(
      "BufferHandle",
      handle,
      dispose = dispose,
    )

  init {
    org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingBufferHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun <T> bindingReadBuffer(block: (Long) -> T): T = core.withLive {
    block(handle)
  }

  internal override fun bindingCloseBuffer(call: (Long) -> Unit) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    destroy()
  }
}
