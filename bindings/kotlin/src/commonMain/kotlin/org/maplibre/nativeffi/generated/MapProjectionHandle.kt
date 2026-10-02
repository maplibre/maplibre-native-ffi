// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

public class MapProjectionHandle
internal constructor(
  handle: Long,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::mapProjection,
) : GeneratedMapProjectionOperations(), AutoCloseable {
  internal override val binding = HandleStateCore("MapProjectionHandle", handle, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()
}
