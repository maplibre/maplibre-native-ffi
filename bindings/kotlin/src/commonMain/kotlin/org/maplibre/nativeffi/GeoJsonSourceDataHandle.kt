// Generated from handle ownership plans by tools/bindgen. Do not edit.
package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

/**
 * GeoJSON data, parsed and ready to add to a GeoJSON source.
 *
 * See `mln_geo_json_source_data` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
public class GeoJsonSourceDataHandle
internal constructor(
  handle: Long,
) : AutoCloseable {
  internal val binding =
    HandleState("GeoJsonSourceDataHandle", handle, GeneratedOwnerDisposal::geoJsonSourceData)
  @Suppress("unused") private val cleanup = disposeWhenUnreachable(this, binding)
  /** The native handle value. It stays readable after release. */
  public val id: ULong
    get() = binding.handle.toULong()

  /** Whether a release, dispose, or discard has ended the handle. */
  public val isClosed: Boolean
    get() = binding.isReleased

  /** Calls [release], for the standard library's `use`. */
  public override fun close() {
    release()
  }

  /**
   * Releases prepared GeoJSON data.
   *
   * Sources that use the data keep it.
   *
   * See `mln_geo_json_source_data_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun release(): Unit =
    nativeClose(this, binding) { C.mln_geo_json_source_data_release(handle) }

  override fun toString(): String =
    "GeoJsonSourceDataHandle(id=0x${binding.handle.toString(16)}, closed=$isClosed)"

  public companion object {
    /**
     * Prepares GeoJSON data for GeoJSON sources.
     *
     * `data` is a GeoJSON feature collection, feature, or geometry. Preparing it can be slow, but
     * adding the result to a source is quick, and several sources can share it.
     *
     * See `mln_geo_json_source_data_create` in the
     * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
     *
     * Errors:
     *
     * - `Status.INVALID_ARGUMENT` when `data` isn't valid GeoJSON, when an option in `options` is
     *   out of range, or when `options` turns on clustering and `data` isn't a feature collection
     *   of points.
     */
    public fun create(
      data: String,
      options: GeoJsonSourceOptions? = null,
    ): GeoJsonSourceDataHandle =
      nativeCall(null, null) {
        val out = allocate(8, w(8, 8, 4))
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
