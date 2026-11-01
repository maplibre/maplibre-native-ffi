// Snippets for the docs site's Query page. They compile with the tests, which never run them.
package org.maplibre.nativeffi.docsnippets

import org.maplibre.nativeffi.RenderSessionHandle
import org.maplibre.nativeffi.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.RenderedQueryGeometry
import org.maplibre.nativeffi.ScreenBox
import org.maplibre.nativeffi.ScreenPoint
import org.maplibre.nativeffi.SourceFeatureQueryOptions

private fun useFeature(feature: String) {}

private suspend fun query(session: RenderSessionHandle, point: ScreenPoint) {
  // #region rendered
  val hits =
    session
      .queryRenderedFeatures(
        RenderedQueryGeometry.Box(
          ScreenBox(
            min = ScreenPoint(x = point.x - 5, y = point.y - 5),
            max = ScreenPoint(x = point.x + 5, y = point.y + 5),
          ),
        ),
        RenderedFeatureQueryOptions(layerIds = listOf("poi")),
      )
      .await()
  for (hit in hits) {
    // hit.feature is the GeoJSON feature, as JSON.
    useFeature(hit.feature)
  }
  // #endregion rendered

  // #region source
  val features =
    session
      .querySourceFeatures(
        "openmaptiles",
        SourceFeatureQueryOptions(
          sourceLayerIds = listOf("transportation"),
          // A filter from the MapLibre style spec.
          filter = """["==", ["get", "class"], "motorway"]""",
        ),
      )
      .await()
  for (feature in features) {
    useFeature(feature.feature)
  }
  // #endregion source
}
