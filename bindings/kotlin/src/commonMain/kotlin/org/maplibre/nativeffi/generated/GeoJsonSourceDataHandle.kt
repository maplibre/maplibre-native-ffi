// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * Prepared GeoJSON data that sources retain independently of its handle.
 *
 * See `mln_geo_json_source_data` in the
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

  public companion object {
    /**
     * Prepares GeoJSON source data for installation on a map.
     *
     * See `mln_geo_json_source_data_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
     */
    public fun create(
      data: String,
      options: GeoJsonSourceOptions? = null,
    ): GeoJsonSourceDataHandle =
      nativeCall(null, null) {
        val out = allocate(8, 8)
        check(
          C.mln_geo_json_source_data_create(
            view(data),
            options?.let { writeGeoJsonSourceOptions(it) } ?: 0L,
            out,
            diagnostic,
          ),
        )
        adopt(out, GeneratedOwnerDisposal::geoJsonSourceData) { GeoJsonSourceDataHandle(it) }
      }
  }
}
