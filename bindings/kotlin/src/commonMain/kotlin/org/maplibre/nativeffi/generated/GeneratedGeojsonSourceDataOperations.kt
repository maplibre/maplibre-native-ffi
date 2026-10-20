// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedGeojsonSourceDataOperations internal constructor() {
  internal abstract val binding: HandleState

  /**
   * Releases prepared GeoJSON source data.
   *
   * See `mln_geojson_source_data_release` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun release(): Unit =
    nativeClose(this, binding) { C.mln_geojson_source_data_release(handle) }
}
