// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

public class BufferHandle
internal constructor(handle: Long, dispose: (Long) -> Unit = GeneratedOwnerDisposal::buffer) :
  GeneratedBufferOperations(), AutoCloseable {
  internal override val binding = HandleStateCore("BufferHandle", handle, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()

  public override fun close() {
    destroy()
  }
}
