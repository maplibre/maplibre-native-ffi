/// Rendered and source feature query descriptors and copied query hits.
library;

import 'dart:typed_data';

import '../geo/geo.dart';
import '../internal/value/byte_values.dart';

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

/// One copied feature query hit.
final class QueriedFeature {
  /// Creates a copied queried feature.
  QueriedFeature({
    required Uint8List feature,
    this.sourceId,
    this.sourceLayerId,
    Uint8List? state,
  }) : feature = copyBytes(feature),
       state = copyOptionalBytes(state);

  /// GeoJSON Feature bytes.
  final Uint8List feature;

  /// Optional source ID.
  final String? sourceId;

  /// Optional source-layer ID.
  final String? sourceLayerId;

  /// Optional feature-state JSON object bytes.
  final Uint8List? state;

  @override
  bool operator ==(Object other) =>
      other is QueriedFeature &&
      optionalBytesEqual(other.feature, feature) &&
      other.sourceId == sourceId &&
      other.sourceLayerId == sourceLayerId &&
      optionalBytesEqual(other.state, state);

  @override
  int get hashCode => Object.hash(
    optionalBytesHash(feature),
    sourceId,
    sourceLayerId,
    optionalBytesHash(state),
  );
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
  RenderedFeatureQueryOptions({List<String>? layerIds, Uint8List? filter})
    : layerIds = layerIds == null ? null : List.unmodifiable(layerIds),
      filter = copyOptionalBytes(filter);

  /// Optional style layer IDs. When absent, all rendered layers are queried.
  final List<String>? layerIds;

  /// Optional MapLibre style-spec filter JSON.
  final Uint8List? filter;

  @override
  bool operator ==(Object other) =>
      other is RenderedFeatureQueryOptions &&
      _optionalStringsEqual(other.layerIds, layerIds) &&
      optionalBytesEqual(other.filter, filter);

  @override
  int get hashCode => Object.hash(
    Object.hashAll(layerIds ?? const <String>[]),
    optionalBytesHash(filter),
  );
}

/// Options for source feature queries.
final class SourceFeatureQueryOptions {
  /// Creates source feature query options.
  SourceFeatureQueryOptions({List<String>? sourceLayerIds, Uint8List? filter})
    : sourceLayerIds = sourceLayerIds == null
          ? null
          : List.unmodifiable(sourceLayerIds),
      filter = copyOptionalBytes(filter);

  /// Optional source-layer IDs. Required by vector sources; ignored by GeoJSON.
  final List<String>? sourceLayerIds;

  /// Optional MapLibre style-spec filter JSON.
  final Uint8List? filter;

  @override
  bool operator ==(Object other) =>
      other is SourceFeatureQueryOptions &&
      _optionalStringsEqual(other.sourceLayerIds, sourceLayerIds) &&
      optionalBytesEqual(other.filter, filter);

  @override
  int get hashCode => Object.hash(
    Object.hashAll(sourceLayerIds ?? const <String>[]),
    optionalBytesHash(filter),
  );
}

bool _optionalStringsEqual(List<String>? left, List<String>? right) {
  if (identical(left, right)) return true;
  if (left == null || right == null || left.length != right.length) {
    return false;
  }
  for (var index = 0; index < left.length; index += 1) {
    if (left[index] != right[index]) {
      return false;
    }
  }
  return true;
}
