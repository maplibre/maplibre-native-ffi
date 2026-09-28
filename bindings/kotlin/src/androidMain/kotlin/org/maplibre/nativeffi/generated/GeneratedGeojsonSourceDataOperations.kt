// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.bytedeco.javacpp.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC

public actual abstract class GeneratedGeojsonSourceDataOperations internal actual constructor() {
  internal abstract fun bindingGeojsonSourceDataHandle(): Long

  internal abstract fun bindingCloseGeojsonSourceData(call: (Long) -> Int)

  public actual fun destroy(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_geojson_source_data_destroy"
      )
      return bindingCloseGeojsonSourceData { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_geojson_source_data_destroy",
        )
        PointerScope().use { arena ->
          MaplibreNativeC.mln_geojson_source_data_destroy(owner)
          0
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
