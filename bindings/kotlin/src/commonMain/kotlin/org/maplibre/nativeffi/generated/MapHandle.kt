// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * A map, which holds map state independent of any render target.
 *
 * See `mln_map` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class MapHandle
internal constructor(
  handle: Long,
  @Suppress("unused") private val parent: RuntimeHandle,
) : GeneratedMapOperations(), org.maplibre.nativeffi.runtime.AsyncReleasable {
  internal override val binding = HandleState("MapHandle", handle, GeneratedOwnerDisposal::map)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * close, so events drained later still match this handle.
   */
  public val id: ULong
    get() = binding.handle.toULong()

  public override val isClosed: Boolean
    get() = binding.isReleased
}
