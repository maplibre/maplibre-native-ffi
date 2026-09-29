// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedGeojsonSourceDataOperations internal actual constructor() {
  internal abstract fun bindingGeojsonSourceDataHandle(): ULong

  internal abstract fun bindingCloseGeojsonSourceData(call: (ULong) -> Unit)

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
        memScoped {
          val arena = this
          mln_geojson_source_data_destroy(owner)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
