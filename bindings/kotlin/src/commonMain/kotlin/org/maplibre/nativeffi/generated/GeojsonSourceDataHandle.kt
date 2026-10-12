// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

public class GeojsonSourceDataHandle
internal constructor(
  handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::geojsonSourceData,
) : GeneratedGeojsonSourceDataOperations(), AutoCloseable {
  internal override val binding =
    HandleStateCore("GeojsonSourceDataHandle", handle, dispose = dispose)
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
    destroy()
  }
}
