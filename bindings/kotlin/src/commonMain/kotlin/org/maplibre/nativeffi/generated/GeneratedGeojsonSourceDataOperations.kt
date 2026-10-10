// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedGeojsonSourceDataOperations internal constructor() {
  internal abstract val binding: HandleStateCore

  /**
   * Releases prepared GeoJSON source data.
   *
   * See `mln_geojson_source_data_destroy` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun destroy(): Unit =
    nativeClose(this, binding, "mln_geojson_source_data_destroy") {
      C.mln_geojson_source_data_destroy(handle)
    }
}
