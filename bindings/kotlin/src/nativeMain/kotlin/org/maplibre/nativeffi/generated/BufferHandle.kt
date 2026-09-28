// Generated from handle ownership plans. Do not edit.
package org.maplibre.nativeffi.generated

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.createCleaner
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore

@OptIn(ExperimentalNativeApi::class)
public actual class BufferHandle internal constructor(private val handle: ULong) :
  GeneratedBufferOperations(), AutoCloseable {
  private val core =
    HandleStateCore("BufferHandle", handle.toLong(), dispose = GeneratedOwnerDisposal::buffer)
  @Suppress("unused") private val cleaner = createCleaner(core.leakReport) { it.report() }

  internal override fun bindingBufferHandle(): ULong {
    core.requireLive()
    return handle
  }

  internal override fun <T> bindingReadBuffer(block: (ULong) -> T): T = core.withLive {
    block(handle)
  }

  internal override fun bindingCloseBuffer(call: (ULong) -> Int) {
    core.closeOnce({ call(handle) })
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual override fun close() {
    core.closeOnce({
      GeneratedOwnerDisposal.buffer(handle.toLong())
      0
    })
  }
}
