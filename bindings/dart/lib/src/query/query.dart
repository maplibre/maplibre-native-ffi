/// Rendered and source feature query descriptors, results, and extension APIs.
library;

import '../geo/geo.dart';
import '../json/json.dart';

/// Feature-state source, feature, and key selector.
final class FeatureStateSelector {
  /// Creates a feature-state selector.
  const FeatureStateSelector({
    required this.sourceId,
    this.sourceLayerId,
    this.featureId,
    this.stateKey,
  });

  /// Source ID.
  final String sourceId;

  /// Optional source layer ID for vector-source disambiguation.
  final String? sourceLayerId;

  /// Optional feature ID.
  final String? featureId;

  /// Optional state key.
  final String? stateKey;
}

/// Rendered feature query geometry.
sealed class RenderedQueryGeometry {
  const RenderedQueryGeometry();
}

/// Rendered point query geometry.
final class RenderedQueryPoint extends RenderedQueryGeometry {
  /// Creates a rendered point query.
  const RenderedQueryPoint(this.point);

  /// Screen point to query.
  final ScreenPoint point;
}

/// Rendered box query geometry.
final class RenderedQueryBox extends RenderedQueryGeometry {
  /// Creates a rendered box query.
  const RenderedQueryBox(this.box);

  /// Screen-space box to query.
  final ScreenBox box;
}

/// Rendered line-string query geometry.
final class RenderedQueryLineString extends RenderedQueryGeometry {
  /// Creates a rendered line-string query.
  RenderedQueryLineString(List<ScreenPoint> points)
    : points = List.unmodifiable(points);

  /// Screen points to query.
  final List<ScreenPoint> points;
}

/// Options for rendered feature queries.
final class RenderedFeatureQueryOptions {
  /// Creates rendered feature query options.
  RenderedFeatureQueryOptions({List<String>? layerIds, this.filter})
    : layerIds = layerIds == null ? null : List.unmodifiable(layerIds);

  /// Optional style layer IDs. When absent, all rendered layers are queried.
  final List<String>? layerIds;

  /// Optional MapLibre style-spec filter JSON.
  final JsonValue? filter;
}

/// Options for source feature queries.
final class SourceFeatureQueryOptions {
  /// Creates source feature query options.
  SourceFeatureQueryOptions({List<String>? sourceLayerIds, this.filter})
    : sourceLayerIds = sourceLayerIds == null
          ? null
          : List.unmodifiable(sourceLayerIds);

  /// Optional source-layer IDs. Required by vector sources; ignored by GeoJSON.
  final List<String>? sourceLayerIds;

  /// Optional MapLibre style-spec filter JSON.
  final JsonValue? filter;
}

/// Copied feature returned by rendered/source feature queries.
final class QueriedFeature {
  /// Creates a copied queried feature.
  const QueriedFeature({
    required this.feature,
    this.sourceId,
    this.sourceLayerId,
    this.state,
  });

  /// Copied GeoJSON feature.
  final FeatureGeoJson feature;

  /// Native render source ID when available.
  final String? sourceId;

  /// Native source layer ID when available.
  final String? sourceLayerId;

  /// Rendered feature state when available.
  final JsonValue? state;
}

/// Result returned by feature extension queries.
sealed class FeatureExtensionResult {
  const FeatureExtensionResult();
}

/// JSON value feature extension result.
final class FeatureExtensionValue extends FeatureExtensionResult {
  /// Creates a JSON value extension result.
  const FeatureExtensionValue(this.value);

  /// Result value.
  final JsonValue value;
}

/// Feature collection feature extension result.
final class FeatureExtensionFeatureCollection extends FeatureExtensionResult {
  /// Creates a feature collection extension result.
  FeatureExtensionFeatureCollection(List<FeatureGeoJson> features)
    : features = List.unmodifiable(features);

  /// Result features.
  final List<FeatureGeoJson> features;
}

/// Feature extension result type unknown to this Dart binding version.
final class FeatureExtensionUnknown extends FeatureExtensionResult {
  /// Creates an unknown result preserving the raw native type.
  const FeatureExtensionUnknown(this.rawType);

  /// Raw native result type value.
  final int rawType;
}
