/// Style source, layer, image, light, property, and custom geometry APIs.
library;

import 'dart:typed_data';

import '../geo/geo.dart';

/// Style source type.
final class SourceType {
  const SourceType._(this.rawValue, this.name);

  /// Creates a source type from a native raw value.
  factory SourceType.fromRaw(int rawValue) => switch (rawValue) {
    0 => unknown,
    1 => vector,
    2 => raster,
    3 => rasterDem,
    4 => geoJson,
    5 => image,
    6 => video,
    7 => annotations,
    8 => customVector,
    _ => SourceType._(rawValue, 'unknown($rawValue)'),
  };

  /// Unknown source type.
  static const unknown = SourceType._(0, 'unknown');

  /// Vector source.
  static const vector = SourceType._(1, 'vector');

  /// Raster source.
  static const raster = SourceType._(2, 'raster');

  /// Raster DEM source.
  static const rasterDem = SourceType._(3, 'rasterDem');

  /// GeoJSON source.
  static const geoJson = SourceType._(4, 'geoJson');

  /// Image source.
  static const image = SourceType._(5, 'image');

  /// Video source.
  static const video = SourceType._(6, 'video');

  /// Annotations source.
  static const annotations = SourceType._(7, 'annotations');

  /// Custom vector source.
  static const customVector = SourceType._(8, 'customVector');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Style tile scheme.
final class TileScheme {
  const TileScheme._(this.rawValue, this.name);

  /// Creates a tile scheme from a native raw value.
  factory TileScheme.fromRaw(int rawValue) => switch (rawValue) {
    0 => xyz,
    1 => tms,
    _ => TileScheme._(rawValue, 'unknown($rawValue)'),
  };

  /// XYZ tile scheme.
  static const xyz = TileScheme._(0, 'xyz');

  /// TMS tile scheme.
  static const tms = TileScheme._(1, 'tms');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Vector tile encoding.
final class VectorTileEncoding {
  const VectorTileEncoding._(this.rawValue, this.name);

  /// Creates a vector tile encoding from a native raw value.
  factory VectorTileEncoding.fromRaw(int rawValue) => switch (rawValue) {
    0 => mvt,
    1 => mlt,
    _ => VectorTileEncoding._(rawValue, 'unknown($rawValue)'),
  };

  /// Mapbox Vector Tile encoding.
  static const mvt = VectorTileEncoding._(0, 'mvt');

  /// MapLibre Tile encoding.
  static const mlt = VectorTileEncoding._(1, 'mlt');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// DEM raster encoding.
final class RasterDemEncoding {
  const RasterDemEncoding._(this.rawValue, this.name);

  /// Creates a DEM raster encoding from a native raw value.
  factory RasterDemEncoding.fromRaw(int rawValue) => switch (rawValue) {
    0 => mapbox,
    1 => terrarium,
    _ => RasterDemEncoding._(rawValue, 'unknown($rawValue)'),
  };

  /// Mapbox DEM encoding.
  static const mapbox = RasterDemEncoding._(0, 'mapbox');

  /// Terrarium DEM encoding.
  static const terrarium = RasterDemEncoding._(1, 'terrarium');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Location indicator image kind.
final class LocationIndicatorImageKind {
  const LocationIndicatorImageKind._(this.rawValue, this.name);

  /// Top image.
  static const top = LocationIndicatorImageKind._(0, 'top');

  /// Bearing image.
  static const bearing = LocationIndicatorImageKind._(1, 'bearing');

  /// Shadow image.
  static const shadow = LocationIndicatorImageKind._(2, 'shadow');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;
}

/// Source metadata copied from retained native style state.
final class SourceInfo {
  /// Creates source metadata.
  const SourceInfo({
    required this.type,
    required this.id,
    required this.isVolatile,
    this.attribution,
    this.url,
    this.tileJson,
    this.tileSize,
    this.vectorEncoding,
    this.rasterDemEncoding,
  });

  /// Source type.
  final SourceType type;

  /// Source ID.
  final String id;

  /// Whether the source is volatile.
  final bool isVolatile;

  /// Optional attribution.
  final String? attribution;

  /// Retained source URL, when the source has one.
  final String? url;

  /// Retained TileJSON for an inline tile source.
  final ParsedTileJson? tileJson;

  /// Tile size exposed by the source.
  final int? tileSize;

  /// Vector tile encoding exposed by the source.
  final VectorTileEncoding? vectorEncoding;

  /// DEM raster encoding exposed by the source.
  final RasterDemEncoding? rasterDemEncoding;
}

/// Retained TileJSON fields for an inline tile source.
final class ParsedTileJson {
  /// Creates copied inline TileJSON metadata.
  ParsedTileJson({
    required List<String> tileUrls,
    required this.minZoom,
    required this.maxZoom,
    required this.scheme,
    this.bounds,
  }) : tileUrls = List.unmodifiable(tileUrls);

  /// Complete tile URL template list.
  final List<String> tileUrls;

  /// Minimum zoom.
  final double minZoom;

  /// Maximum zoom.
  final double maxZoom;

  /// Tile coordinate scheme.
  final TileScheme scheme;

  /// Optional geographic bounds.
  final LatLngBounds? bounds;
}

/// Options for vector, raster, and raster DEM tile sources.
final class TileSourceOptions {
  /// Creates tile source options.
  const TileSourceOptions({
    this.minZoom,
    this.maxZoom,
    this.attribution,
    this.scheme,
    this.bounds,
    this.tileSize,
    this.vectorEncoding,
    this.rasterDemEncoding,
  });

  /// Optional minimum zoom.
  final double? minZoom;

  /// Optional maximum zoom.
  final double? maxZoom;

  /// Optional attribution.
  final String? attribution;

  /// Optional tile scheme.
  final TileScheme? scheme;

  /// Optional bounds.
  final LatLngBounds? bounds;

  /// Optional tile size.
  final int? tileSize;

  /// Optional vector tile encoding.
  final VectorTileEncoding? vectorEncoding;

  /// Optional raster DEM encoding.
  final RasterDemEncoding? rasterDemEncoding;
}

/// Options fixed when a GeoJSON source is created.
final class GeoJsonSourceOptions {
  /// Creates GeoJSON source options.
  const GeoJsonSourceOptions({
    this.minZoom,
    this.maxZoom,
    this.tolerance,
    this.clusterMaxZoom,
    this.clusterProperties,
    this.tileSize,
    this.buffer,
    this.clusterRadius,
    this.clusterMinPoints,
    this.lineMetrics,
    this.cluster,
    this.synchronousUpdate,
  });

  /// Optional minimum tiling zoom.
  final double? minZoom;

  /// Optional maximum tiling zoom.
  final double? maxZoom;

  /// Optional Douglas-Peucker simplification tolerance.
  final double? tolerance;

  /// Optional highest zoom at which points cluster.
  final double? clusterMaxZoom;

  /// Optional cluster aggregation expressions keyed by property name.
  final Uint8List? clusterProperties;

  /// Optional tile extent in pixels.
  final int? tileSize;

  /// Optional tile buffer in pixels.
  final int? buffer;

  /// Optional cluster radius in pixels.
  final int? clusterRadius;

  /// Optional number of points required to form a cluster.
  final int? clusterMinPoints;

  /// Optional line-distance-metrics switch.
  final bool? lineMetrics;

  /// Optional point-clustering switch.
  final bool? cluster;

  /// Optional synchronous-update switch. When set, data updates are tiled
  /// inline so they reach the next rendered frame instead of a later one.
  final bool? synchronousUpdate;

  @override
  bool operator ==(Object other) =>
      other is GeoJsonSourceOptions &&
      other.minZoom == minZoom &&
      other.maxZoom == maxZoom &&
      other.tolerance == tolerance &&
      other.clusterMaxZoom == clusterMaxZoom &&
      other.clusterProperties == clusterProperties &&
      other.tileSize == tileSize &&
      other.buffer == buffer &&
      other.clusterRadius == clusterRadius &&
      other.clusterMinPoints == clusterMinPoints &&
      other.lineMetrics == lineMetrics &&
      other.cluster == cluster &&
      other.synchronousUpdate == synchronousUpdate;

  @override
  int get hashCode => Object.hash(
    minZoom,
    maxZoom,
    tolerance,
    clusterMaxZoom,
    clusterProperties,
    tileSize,
    buffer,
    clusterRadius,
    clusterMinPoints,
    lineMetrics,
    cluster,
    synchronousUpdate,
  );
}

/// Whether a style layer draws.
final class StyleLayerVisibility {
  const StyleLayerVisibility._(this.rawValue, this.name);

  /// Creates a visibility from a native raw value.
  factory StyleLayerVisibility.fromRawValue(int rawValue) => switch (rawValue) {
    0 => visible,
    1 => none,
    _ => StyleLayerVisibility._(rawValue, 'unknown($rawValue)'),
  };

  /// The layer draws.
  static const visible = StyleLayerVisibility._(0, 'visible');

  /// The layer does not draw.
  static const none = StyleLayerVisibility._(1, 'none');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is StyleLayerVisibility && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Style image options.
final class ImageStretch {
  /// Creates a stretchable interval.
  const ImageStretch(this.from, this.to);

  /// Interval start, in image pixels.
  final double from;

  /// Interval end, in image pixels.
  final double to;

  @override
  bool operator ==(Object other) =>
      other is ImageStretch && other.from == from && other.to == to;

  @override
  int get hashCode => Object.hash(from, to);
}

/// Content-box insets in image pixels, measured from the image's top-left.
final class ImageContent {
  /// Creates content-box insets.
  const ImageContent({
    required this.left,
    required this.top,
    required this.right,
    required this.bottom,
  });

  /// Left inset.
  final double left;

  /// Top inset.
  final double top;

  /// Right inset.
  final double right;

  /// Bottom inset.
  final double bottom;

  @override
  bool operator ==(Object other) =>
      other is ImageContent &&
      other.left == left &&
      other.top == top &&
      other.right == right &&
      other.bottom == bottom;

  @override
  int get hashCode => Object.hash(left, top, right, bottom);
}

/// How a stretchable image fits text along one axis.
final class StyleImageTextFit {
  const StyleImageTextFit._(this.rawValue, this.name);

  /// Creates a text fit from a native raw value.
  factory StyleImageTextFit.fromRawValue(int rawValue) => switch (rawValue) {
    0 => stretchOrShrink,
    1 => stretchOnly,
    2 => proportional,
    _ => StyleImageTextFit._(rawValue, 'unknown($rawValue)'),
  };

  /// The image stretches or shrinks to fit the text.
  static const stretchOrShrink = StyleImageTextFit._(0, 'stretchOrShrink');

  /// The image only stretches to fit the text.
  static const stretchOnly = StyleImageTextFit._(1, 'stretchOnly');

  /// The image scales proportionally to fit the text.
  static const proportional = StyleImageTextFit._(2, 'proportional');

  /// Raw native value.
  final int rawValue;

  /// Human-readable name.
  final String name;

  @override
  bool operator ==(Object other) =>
      other is StyleImageTextFit && other.rawValue == rawValue;

  @override
  int get hashCode => rawValue.hashCode;
}

/// Style image options.
final class StyleImageOptions {
  /// Creates style image options.
  ///
  /// The stretch lists are copied into unmodifiable storage, so later caller
  /// mutation leaves these options unchanged.
  StyleImageOptions({
    this.pixelRatio,
    this.sdf,
    List<ImageStretch>? stretchX,
    List<ImageStretch>? stretchY,
    this.content,
    this.textFitWidth,
    this.textFitHeight,
  }) : stretchX = stretchX == null ? null : List.unmodifiable(stretchX),
       stretchY = stretchY == null ? null : List.unmodifiable(stretchY);

  /// Optional pixel ratio.
  final double? pixelRatio;

  /// Optional signed-distance-field flag.
  final bool? sdf;

  /// Optional horizontally stretchable intervals. A present empty list stays
  /// distinguishable from an absent one.
  final List<ImageStretch>? stretchX;

  /// Optional vertically stretchable intervals.
  final List<ImageStretch>? stretchY;

  /// Optional content box used when `icon-text-fit` applies.
  final ImageContent? content;

  /// Optional text fit along the width axis.
  final StyleImageTextFit? textFitWidth;

  /// Optional text fit along the height axis.
  final StyleImageTextFit? textFitHeight;
}

/// The style's global transition options, distinct from camera animation
/// options and from the per-property transitions a style declares.
final class StyleTransitionOptions {
  /// Creates style transition options.
  const StyleTransitionOptions({
    this.durationMs,
    this.delayMs,
    this.enablePlacementTransitions,
  });

  /// Transition duration in milliseconds. Absent falls back to the duration the
  /// style declares for each transitioning property.
  final double? durationMs;

  /// Transition delay in milliseconds. Absent falls back to the delay the style
  /// declares for each transitioning property.
  final double? delayMs;

  /// Whether symbol placement changes cross-fade. Absent leaves the cross-fade
  /// on; clearing it makes placement changes apply to the next rendered frame.
  /// Reading the options always reports a value.
  final bool? enablePlacementTransitions;

  @override
  bool operator ==(Object other) =>
      other is StyleTransitionOptions &&
      other.durationMs == durationMs &&
      other.delayMs == delayMs &&
      other.enablePlacementTransitions == enablePlacementTransitions;

  @override
  int get hashCode =>
      Object.hash(durationMs, delayMs, enablePlacementTransitions);
}

/// Caller-owned premultiplied RGBA8 image pixels.
final class PremultipliedRgba8Image {
  /// Creates a premultiplied RGBA8 image.
  PremultipliedRgba8Image({
    required this.width,
    required this.height,
    required this.stride,
    required Uint8List bytes,
  }) : bytes = Uint8List.fromList(bytes).asUnmodifiableView();

  /// Image width in pixels.
  final int width;

  /// Image height in pixels.
  final int height;

  /// Bytes per image row.
  final int stride;

  /// Premultiplied RGBA8 pixels.
  final Uint8List bytes;
}

/// Style image metadata.
final class StyleImageInfo {
  /// Creates style image metadata.
  const StyleImageInfo({
    required this.width,
    required this.height,
    required this.stride,
    required this.byteLength,
    required this.pixelRatio,
    required this.sdf,
    this.stretchXCount = 0,
    this.stretchYCount = 0,
    this.content,
    this.textFitWidth,
    this.textFitHeight,
  });

  /// Interval counts for the stretchable axes. Read the intervals themselves
  /// with `MapHandle.getStyleImageStretches`.
  final int stretchXCount;

  /// Vertical interval count.
  final int stretchYCount;

  /// Content box, absent when the image carries none.
  final ImageContent? content;

  /// Text fit along the width axis, absent when the image carries none.
  final StyleImageTextFit? textFitWidth;

  /// Text fit along the height axis, absent when the image carries none.
  final StyleImageTextFit? textFitHeight;

  /// Image width in pixels.
  final int width;

  /// Image height in pixels.
  final int height;

  /// Bytes per image row.
  final int stride;

  /// Required byte length for a copied premultiplied RGBA8 image.
  final int byteLength;

  /// Pixel ratio.
  final double pixelRatio;

  /// Whether the image is an SDF image.
  final bool sdf;
}

/// Copied style image pixels and metadata.
final class StyleImage {
  /// Creates a copied style image.
  StyleImage({required this.info, required Uint8List bytes})
    : bytes = Uint8List.fromList(bytes).asUnmodifiableView();

  /// Copied image metadata.
  final StyleImageInfo info;

  /// Copied tightly packed premultiplied RGBA8 pixels.
  final Uint8List bytes;
}

/// Callback invoked when a custom geometry source needs or cancels one tile.
typedef CustomGeometryTileCallback = void Function(CanonicalTileId tileId);

/// Custom geometry source options.
final class CustomGeometrySourceOptions {
  /// Creates custom geometry source options.
  const CustomGeometrySourceOptions({
    required this.fetchTile,
    this.cancelTile,
    this.minZoom,
    this.maxZoom,
    this.tolerance,
    this.tileSize,
    this.buffer,
    this.clip,
    this.wrap,
  });

  /// Required tile fetch notification.
  final CustomGeometryTileCallback fetchTile;

  /// Optional best-effort tile cancel notification.
  final CustomGeometryTileCallback? cancelTile;

  /// Optional minimum zoom.
  final double? minZoom;

  /// Optional maximum zoom.
  final double? maxZoom;

  /// Optional tolerance.
  final double? tolerance;

  /// Optional tile size.
  final int? tileSize;

  /// Optional tile buffer.
  final int? buffer;

  /// Optional clipping flag.
  final bool? clip;

  /// Optional wrapping flag.
  final bool? wrap;
}
