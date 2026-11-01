// Snippets for the docs site's Query page. They are analyzed with the tests,
// which never run them.

import 'package:maplibre_native_ffi/maplibre_native_ffi.dart';

void useFeature(Object? feature) {}

Future<void> query(RenderSessionHandle session, ScreenPoint point) async {
  // #region rendered
  final hits = await session.queryRenderedFeatures(
    RenderedQueryGeometry.box(
      ScreenBox(
        min: ScreenPoint(x: point.x - 5, y: point.y - 5),
        max: ScreenPoint(x: point.x + 5, y: point.y + 5),
      ),
    ),
    options: RenderedFeatureQueryOptions(layerIds: ['poi']),
  );
  for (final hit in hits) {
    // hit.feature is the decoded GeoJSON feature.
    useFeature(hit.feature);
  }
  // #endregion rendered

  // #region source
  final features = await session.querySourceFeatures(
    'openmaptiles',
    options: SourceFeatureQueryOptions(
      sourceLayerIds: ['transportation'],
      // A filter from the MapLibre style spec.
      filter: [
        '==',
        ['get', 'class'],
        'motorway',
      ],
    ),
  );
  for (final feature in features) {
    useFeature(feature.feature);
  }
  // #endregion source
}
