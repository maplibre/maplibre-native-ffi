// Snippets for the docs site's Create a map page. They compile with
// the tests, which never run them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.CameraOptions
import org.maplibre.nativeffi.CameraUpdate
import org.maplibre.nativeffi.CameraUpdateMode
import org.maplibre.nativeffi.LogicalExtent
import org.maplibre.nativeffi.MapHandle
import org.maplibre.nativeffi.MapMode
import org.maplibre.nativeffi.MapOptions
import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.RuntimeHandle
import org.maplibre.nativeffi.useReleasing

private fun reportFailure(error: MaplibreException) {}

// Attaches a render target, as the render target guides show.
private suspend fun attachTarget(
  map: MapHandle,
): RenderSessionHandle = TODO()

private suspend fun createAMap() {
  // #region runtime
  val runtime = RuntimeHandle.create().await()
  // #endregion runtime

  // #region map
  val map =
    runtime.createMap(
      MapOptions(
        initialExtent =
          LogicalExtent(
            width = 800u,
            height = 600u,
            scaleFactor = 2.0,
          ),
        mapMode = MapMode.CONTINUOUS,
      ),
    )
  map.ready.await()
  // #endregion map

  // #region style
  map.discarding.setStyleUrl(
    "https://demotiles.maplibre.org/style.json",
  )
  // #endregion style

  val update =
    CameraUpdate(
      mode = CameraUpdateMode.JUMP,
      camera = CameraOptions(zoom = 3.0),
    )

  // #region wait
  map.updateCamera(update).await()

  // Use the discarding variant to ignore the result.
  map.discarding.updateCamera(update)
  // #endregion wait

  // #region errors
  try {
    map.updateCamera(update).await()
  } catch (error: MaplibreException) {
    // Its message describes the failure.
    reportFailure(error)
  }
  // #endregion errors

  val session = attachTarget(map)

  // #region close
  // Each block releases its handle when it ends, and waits for it.
  runtime.useReleasing {
    map.useReleasing {
      session.useReleasing {
        // Render frames and change the map.
      }
    }
  }
  // #endregion close
}
