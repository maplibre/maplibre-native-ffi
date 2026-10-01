// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

public class MapHandle
internal constructor(
  handle: Long,
  parent: RuntimeHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::map,
) : GeneratedMapOperations(), org.maplibre.nativeffi.runtime.AsyncReleasable {
  internal override val binding = HandleStateCore("MapHandle", handle, parent, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()
}
