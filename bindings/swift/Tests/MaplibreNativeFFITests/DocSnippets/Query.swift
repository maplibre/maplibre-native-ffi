// Snippets for the docs site's Query page. They compile with the tests, which
// never run them.

import MaplibreNativeFFI

private func useFeature(_: String) {}

private func query(
  session: RenderSessionHandle,
  point: ScreenPoint
) async throws {
  // #region rendered
  let hits = try await session.queryRenderedFeatures(
    geometry: .box(ScreenBox(
      min: ScreenPoint(x: point.x - 5, y: point.y - 5),
      max: ScreenPoint(x: point.x + 5, y: point.y + 5)
    )),
    options: RenderedFeatureQueryOptions(layerIds: ["poi"])
  )
  for hit in hits {
    // hit.feature is the GeoJSON feature, as JSON.
    useFeature(hit.feature)
  }
  // #endregion rendered

  // #region source
  let features = try await session.querySourceFeatures(
    "openmaptiles",
    options: SourceFeatureQueryOptions(
      sourceLayerIds: ["transportation"],
      // A filter from the MapLibre style spec, as JSON.
      filter: #"["==", ["get", "class"], "motorway"]"#
    )
  )
  for feature in features {
    useFeature(feature.feature)
  }
  // #endregion source
}
