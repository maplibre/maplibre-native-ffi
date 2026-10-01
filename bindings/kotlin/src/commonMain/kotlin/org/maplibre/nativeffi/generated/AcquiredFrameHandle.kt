// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

public class AcquiredFrameHandle
internal constructor(
  handle: Long,
  parent: RenderSessionHandle,
  dispose: (Long) -> Unit = GeneratedOwnerDisposal::acquiredFrame,
) : GeneratedAcquiredFrameOperations(), AutoCloseable {
  internal override val binding =
    HandleStateCore("AcquiredFrameHandle", handle, parent, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()

  public override fun close() {
    release()
  }
}
