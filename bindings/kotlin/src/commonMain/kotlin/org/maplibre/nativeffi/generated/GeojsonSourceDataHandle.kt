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

  public override fun close() {
    destroy()
  }
}
