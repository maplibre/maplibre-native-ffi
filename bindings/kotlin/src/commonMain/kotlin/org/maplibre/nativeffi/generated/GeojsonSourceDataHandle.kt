// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.lifecycle.*

/**
 * Prepared GeoJSON data that sources retain independently of its handle.
 *
 * See `mln_geojson_source_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class GeojsonSourceDataHandle
internal constructor(
  handle: Long,
) : GeneratedGeojsonSourceDataOperations(), AutoCloseable {
  internal override val binding =
    HandleState("GeojsonSourceDataHandle", handle, GeneratedOwnerDisposal::geojsonSourceData)
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
}
