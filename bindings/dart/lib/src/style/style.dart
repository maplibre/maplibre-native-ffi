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

/// Fixed source metadata copied from native style state.
final class SourceInfo {
  /// Creates source metadata.
  const SourceInfo({
    required this.type,
    required this.id,
    required this.isVolatile,
    this.attribution,
  });

  /// Source type.
  final SourceType type;

  /// Source ID.
  final String id;

  /// Whether the source is volatile.
  final bool isVolatile;

  /// Optional attribution.
  final String? attribution;
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

/// Style image options.
final class StyleImageOptions {
  /// Creates style image options.
  const StyleImageOptions({this.pixelRatio, this.sdf});

  /// Optional pixel ratio.
  final double? pixelRatio;

  /// Optional signed-distance-field flag.
  final bool? sdf;
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
  });

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
