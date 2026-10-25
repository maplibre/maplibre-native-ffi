// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * Prepared GeoJSON data that sources retain independently of its handle.
 *
 * See `mln_geojson_source_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class GeoJsonSourceDataHandle
internal constructor(
  handle: Long,
) : GeneratedGeoJsonSourceDataOperations(), AutoCloseable {
  internal override val binding =
    HandleState("GeoJsonSourceDataHandle", handle, GeneratedOwnerDisposal::geoJsonSourceData)
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
    "GeoJsonSourceDataHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"
}
