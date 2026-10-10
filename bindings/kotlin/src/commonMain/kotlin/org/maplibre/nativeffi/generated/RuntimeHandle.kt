// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * Handles are opaque 64-bit generational ids.
 *
 * See `mln_runtime` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class RuntimeHandle
internal constructor(handle: Long, dispose: (Long) -> Unit = GeneratedOwnerDisposal::runtime) :
  GeneratedRuntimeOperations(), org.maplibre.nativeffi.runtime.AsyncReleasable {
  internal override val binding = HandleStateCore("RuntimeHandle", handle, dispose = dispose)
  @Suppress("unused") private val cleanup = trackLeak(this, binding.leakReport)
  public val isClosed: Boolean
    get() = binding.isReleased()
}
