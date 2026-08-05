/// Geographic, geometry, tile, and feature value types.
library;

import '../json/json.dart';

/// Geographic coordinate in degrees.
final class LatLng {
  /// Creates a geographic coordinate.
  const LatLng(this.latitude, this.longitude);

  /// Latitude in degrees.
  final double latitude;

  /// Longitude in degrees.
  final double longitude;

  @override
  bool operator ==(Object other) =>
      other is LatLng &&
      other.latitude == latitude &&
      other.longitude == longitude;

  @override
  int get hashCode => Object.hash(latitude, longitude);
}

/// Geographic bounds in degrees.
final class LatLngBounds {
  /// Creates geographic bounds.
  const LatLngBounds({required this.southwest, required this.northeast});

  /// Southwest corner.
  final LatLng southwest;

  /// Northeast corner.
  final LatLng northeast;

  @override
  bool operator ==(Object other) =>
      other is LatLngBounds &&
      other.southwest == southwest &&
      other.northeast == northeast;

  @override
  int get hashCode => Object.hash(southwest, northeast);
}

/// Lower-level Spherical Mercator projected-meter coordinate.
final class ProjectedMeters {
  /// Creates projected meters.
  const ProjectedMeters({required this.northing, required this.easting});

  /// Distance measured northward from the equator, in meters.
  final double northing;

  /// Distance measured eastward from the prime meridian, in meters.
  final double easting;

  @override
  bool operator ==(Object other) =>
      other is ProjectedMeters &&
      other.northing == northing &&
      other.easting == easting;

  @override
  int get hashCode => Object.hash(northing, easting);
}

/// Screen-space point in logical map pixels.
final class ScreenPoint {
  /// Creates a screen-space point.
  const ScreenPoint(this.x, this.y);

  /// X coordinate.
  final double x;

  /// Y coordinate.
  final double y;

  @override
  bool operator ==(Object other) =>
      other is ScreenPoint && other.x == x && other.y == y;

  @override
  int get hashCode => Object.hash(x, y);
}

/// Screen-space box in logical map pixels.
final class ScreenBox {
  /// Creates a screen-space box.
  const ScreenBox({required this.min, required this.max});

  /// Minimum corner.
  final ScreenPoint min;

  /// Maximum corner.
  final ScreenPoint max;

  @override
  bool operator ==(Object other) =>
      other is ScreenBox && other.min == min && other.max == max;

  @override
  int get hashCode => Object.hash(min, max);
}

/// Screen-space inset in logical map pixels.
final class EdgeInsets {
  /// Creates edge insets.
  const EdgeInsets({
    this.top = 0,
    this.left = 0,
    this.bottom = 0,
    this.right = 0,
  });

  /// Top inset.
  final double top;

  /// Left inset.
  final double left;

  /// Bottom inset.
  final double bottom;

  /// Right inset.
  final double right;

  @override
  bool operator ==(Object other) =>
      other is EdgeInsets &&
      other.top == top &&
      other.left == left &&
      other.bottom == bottom &&
      other.right == right;

  @override
  int get hashCode => Object.hash(top, left, bottom, right);
}

/// Three-component vector used by free camera options.
final class Vec3 {
  /// Creates a three-component vector.
  const Vec3(this.x, this.y, this.z);

  /// X component.
  final double x;

  /// Y component.
  final double y;

  /// Z component.
  final double z;

  @override
  bool operator ==(Object other) =>
      other is Vec3 && other.x == x && other.y == y && other.z == z;

  @override
  int get hashCode => Object.hash(x, y, z);
}

/// Quaternion stored as x, y, z, w components.
final class Quaternion {
  /// Creates a quaternion.
  const Quaternion(this.x, this.y, this.z, this.w);

  /// X component.
  final double x;

  /// Y component.
  final double y;

  /// Z component.
  final double z;

  /// W component.
  final double w;

  @override
  bool operator ==(Object other) =>
      other is Quaternion &&
      other.x == x &&
      other.y == y &&
      other.z == z &&
      other.w == w;

  @override
  int get hashCode => Object.hash(x, y, z, w);
}

/// Overscaled tile identity reported in observer events.
final class TileId {
  /// Creates a tile identity.
  const TileId({
    required this.overscaledZ,
    required this.wrap,
    required this.canonicalZ,
    required this.canonicalX,
    required this.canonicalY,
  });

  /// Overscaled zoom.
  final int overscaledZ;

  /// Tile wrap.
  final int wrap;

  /// Canonical zoom.
  final int canonicalZ;

  /// Canonical X coordinate.
  final int canonicalX;

  /// Canonical Y coordinate.
  final int canonicalY;
}

/// Canonical tile identity.
final class CanonicalTileId {
  /// Creates a canonical tile identity.
  const CanonicalTileId({required this.z, required this.x, required this.y});

  /// Canonical zoom.
  final int z;

  /// Canonical X coordinate.
  final int x;

  /// Canonical Y coordinate.
  final int y;
}

/// Owned geometry descriptor.
sealed class Geometry {
  const Geometry();
}

/// Empty geometry.
final class EmptyGeometry extends Geometry {
  /// Creates an empty geometry.
  const EmptyGeometry();
}

/// Point geometry.
final class PointGeometry extends Geometry {
  /// Creates a point geometry.
  const PointGeometry(this.coordinate);

  /// Point coordinate.
  final LatLng coordinate;
}

/// Line-string geometry.
final class LineStringGeometry extends Geometry {
  /// Creates a line-string geometry.
  LineStringGeometry(List<LatLng> coordinates)
    : coordinates = List.unmodifiable(coordinates);

  /// Coordinates in order.
  final List<LatLng> coordinates;
}

/// Polygon geometry as linear rings.
final class PolygonGeometry extends Geometry {
  /// Creates polygon geometry.
  PolygonGeometry(List<List<LatLng>> rings)
    : rings = List.unmodifiable(
        rings.map((ring) => List<LatLng>.unmodifiable(ring)),
      );

  /// Polygon rings.
  final List<List<LatLng>> rings;
}

/// Multi-point geometry.
final class MultiPointGeometry extends Geometry {
  /// Creates multi-point geometry.
  MultiPointGeometry(List<LatLng> coordinates)
    : coordinates = List.unmodifiable(coordinates);

  /// Point coordinates.
  final List<LatLng> coordinates;
}

/// Multi-line-string geometry.
final class MultiLineStringGeometry extends Geometry {
  /// Creates multi-line-string geometry.
  MultiLineStringGeometry(List<List<LatLng>> lines)
    : lines = List.unmodifiable(
        lines.map((line) => List<LatLng>.unmodifiable(line)),
      );

  /// Line strings.
  final List<List<LatLng>> lines;
}

/// Multi-polygon geometry.
final class MultiPolygonGeometry extends Geometry {
  /// Creates multi-polygon geometry.
  MultiPolygonGeometry(List<List<List<LatLng>>> polygons)
    : polygons = List.unmodifiable(
        polygons.map(
          (polygon) => List<List<LatLng>>.unmodifiable(
            polygon.map((ring) => List<LatLng>.unmodifiable(ring)),
          ),
        ),
      );

  /// Polygons as rings.
  final List<List<List<LatLng>>> polygons;
}

/// Geometry collection.
final class GeometryCollection extends Geometry {
  /// Creates a geometry collection.
  GeometryCollection(List<Geometry> geometries)
    : geometries = List.unmodifiable(geometries);

  /// Child geometries.
  final List<Geometry> geometries;
}

/// Feature identifier used by GeoJSON feature descriptors.
sealed class FeatureIdentifier {
  const FeatureIdentifier();
}

/// Feature without an identifier.
final class NullFeatureIdentifier extends FeatureIdentifier {
  /// Creates a null feature identifier.
  const NullFeatureIdentifier();
}

/// Unsigned integer feature identifier.
final class UIntFeatureIdentifier extends FeatureIdentifier {
  /// Creates an unsigned identifier from a non-negative Dart [int].
  UIntFeatureIdentifier(int value) : value = BigInt.from(value);

  /// Creates an identifier across the full `uint64_t` domain.
  UIntFeatureIdentifier.fromBigInt(this.value);

  /// Unsigned value, represented as [BigInt] so all 64 bits are preserved.
  final BigInt value;
}

/// Signed integer feature identifier.
final class IntFeatureIdentifier extends FeatureIdentifier {
  /// Creates a signed integer feature identifier.
  const IntFeatureIdentifier(this.value);

  /// Identifier value.
  final int value;
}

/// Floating-point feature identifier.
final class DoubleFeatureIdentifier extends FeatureIdentifier {
  /// Creates a floating-point feature identifier.
  const DoubleFeatureIdentifier(this.value);

  /// Identifier value.
  final double value;
}

/// String feature identifier.
final class StringFeatureIdentifier extends FeatureIdentifier {
  /// Creates a string feature identifier.
  const StringFeatureIdentifier(this.value);

  /// Identifier value.
  final String value;
}

/// Owned GeoJSON descriptor.
sealed class GeoJson {
  const GeoJson();
}

/// GeoJSON geometry descriptor.
final class GeometryGeoJson extends GeoJson {
  /// Creates a geometry GeoJSON descriptor.
  const GeometryGeoJson(this.geometry);

  /// Geometry value.
  final Geometry geometry;
}

/// GeoJSON feature descriptor.
final class FeatureGeoJson extends GeoJson {
  /// Creates a feature GeoJSON descriptor.
  FeatureGeoJson({
    required this.geometry,
    List<JsonMember> properties = const [],
    this.identifier = const NullFeatureIdentifier(),
  }) : properties = List.unmodifiable(properties);

  /// Feature geometry.
  final Geometry geometry;

  /// Feature properties.
  final List<JsonMember> properties;

  /// Optional feature identifier.
  final FeatureIdentifier identifier;
}

/// GeoJSON feature collection descriptor.
final class FeatureCollectionGeoJson extends GeoJson {
  /// Creates a feature collection GeoJSON descriptor.
  FeatureCollectionGeoJson(List<FeatureGeoJson> features)
    : features = List.unmodifiable(features);

  /// Feature entries.
  final List<FeatureGeoJson> features;
}
