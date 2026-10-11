// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * An owned batch of runtime events from one drain.
 *
 * See `mln_event_batch` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class EventBatchHandle
internal constructor(handle: Long, dispose: (Long) -> Unit = GeneratedOwnerDisposal::eventBatch) :
  GeneratedEventBatchOperations(), AutoCloseable {
  internal override val binding = HandleStateCore("EventBatchHandle", handle, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()

  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * close, so events drained later still match this handle.
   */
  public val id: ULong
    get() = binding.issued().toULong()

  public override fun close() {
    release()
  }
}
