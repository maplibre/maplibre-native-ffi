// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

public class RenderSessionHandle
internal constructor(
  handle: Long,
  parent: MapHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::renderSession,
) : GeneratedRenderSessionOperations(), AutoCloseable {
  internal override val binding =
    HandleStateCore("RenderSessionHandle", handle, parent, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()

  public override fun close() {
    destroy()
  }
}
