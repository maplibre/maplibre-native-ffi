// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.*

public actual abstract class GeneratedGeojsonSourceDataOperations internal actual constructor() {
  internal abstract fun bindingGeojsonSourceDataHandle(): Long

  internal abstract fun bindingCloseGeojsonSourceData(call: (Long) -> Unit)

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
        Arena.ofConfined().use { arena -> MapLibreNativeC.mln_geojson_source_data_destroy(owner) }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
