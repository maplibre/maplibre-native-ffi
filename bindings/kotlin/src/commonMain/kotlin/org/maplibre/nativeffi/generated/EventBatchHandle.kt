// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

public class EventBatchHandle
internal constructor(handle: Long, dispose: (Long) -> Unit = GeneratedOwnerDisposal::eventBatch) :
  GeneratedEventBatchOperations(), AutoCloseable {
  internal override val binding = HandleStateCore("EventBatchHandle", handle, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()

  public override fun close() {
    release()
  }
}
