// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

public class RuntimeHandle
internal constructor(handle: Long, dispose: (Long) -> Unit = GeneratedOwnerDisposal::runtime) :
  GeneratedRuntimeOperations(), org.maplibre.nativeffi.runtime.AsyncReleasable {
  internal override val binding = HandleStateCore("RuntimeHandle", handle, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()
}
