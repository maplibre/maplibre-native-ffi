// Snippets for the docs site's Create a map page. They compile with the tests, which never run
// them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.MapOptions
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.RuntimeOptions

// Attaches a render target, as the Rendering guides show.
private suspend fun attachTarget(map: MapHandle): RenderSessionHandle = TODO()

private suspend fun createAMap() {
  // #region runtime
  val runtime = RuntimeHandle.create(RuntimeOptions())
  // #endregion runtime

  // #region map
  val (map, ready) =
    runtime.createMap(
      MapOptions(
        initialExtent = LogicalExtent(width = 800u, height = 600u, scaleFactor = 2.0),
        mapMode = MapMode.CONTINUOUS,
      ),
    )
  ready.await()
  // #endregion map

  val session = attachTarget(map)

  // #region close
  session.close()
  map.release().await()
  runtime.release().await()
  // #endregion close
}
