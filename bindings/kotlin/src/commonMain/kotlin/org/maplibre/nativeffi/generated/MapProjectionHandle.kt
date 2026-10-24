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
  /** The native handle value. It stays readable after close. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, close, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  public override fun close() {
    release()
  }

  override fun toString(): String =
    "MapProjectionHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
