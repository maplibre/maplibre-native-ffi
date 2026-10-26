// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

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

  public companion object {
    /**
     * Creates a standalone projection of a view, such as the view of a rendered frame or of a map
     * snapshot.
     *
     * See `mln_map_projection_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
     */
    public fun create(view: MapView): MapProjectionHandle =
      nativeCall(null, null) {
        val out = allocate(8, 8)
        check(C.mln_map_projection_create(writeMapView(view), out, diagnostic))
        adopt(out, GeneratedOwnerDisposal::mapProjection) { MapProjectionHandle(it) }
      }
  }
}
