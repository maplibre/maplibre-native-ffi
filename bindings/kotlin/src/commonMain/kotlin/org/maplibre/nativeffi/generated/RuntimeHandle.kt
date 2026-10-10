// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * A runtime: the native scheduler thread and event store for its maps.
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
