// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.HandleLeakCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

public actual class BufferHandle internal constructor(private val handle: Long) :
  GeneratedBufferOperations(), AutoCloseable {
  private val core =
    HandleStateCore("BufferHandle", handle, dispose = GeneratedOwnerDisposal::buffer)

  init {
    HandleLeakCleaner.register(this, core.leakReport)
  }

  internal override fun bindingBufferHandle(): Long {
    core.requireLive()
    return handle
  }

  internal override fun <T> bindingReadBuffer(block: (Long) -> T): T = core.withLive {
    block(handle)
  }

  internal override fun bindingCloseBuffer(call: (Long) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce({
      GeneratedOwnerDisposal.buffer(handle)
      0
    })
  }
}
