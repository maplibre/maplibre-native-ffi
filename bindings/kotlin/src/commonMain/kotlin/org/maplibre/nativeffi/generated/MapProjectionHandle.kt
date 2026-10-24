// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * A standalone projection of one map view.
 *
 * See `mln_map_projection` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class MapProjectionHandle
internal constructor(
  handle: Long,
) : GeneratedMapProjectionOperations(), AutoCloseable {
  internal override val binding =
    HandleState("MapProjectionHandle", handle, GeneratedOwnerDisposal::mapProjection)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /**
   * The native handle value, which runtime events report as their source. It stays readable after
   * close, so events drained later still match this handle.
   */
  public val id: ULong
    get() = binding.handle.toULong()

  public override fun close() {
    release()
  }

  override fun toString(): String =
    "MapProjectionHandle(id=0x${binding.handle.toString(16)}, closed=${binding.isReleased})"
}
