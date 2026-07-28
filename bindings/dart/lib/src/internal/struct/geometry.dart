import 'dart:ffi';

import 'package:ffi/ffi.dart';

import '../../geo/geo.dart';
import '../../json/json.dart';
import '../c/maplibre_native_c.g.dart' as raw;
import '../memory/memory.dart';
import '../status/status.dart';
import 'json.dart';
import 'struct.dart';

/// Maximum nested geometry descriptor depth accepted by the binding.
const int maxGeometryDescriptorDepth = 64;

final BigInt _uint64Modulus = BigInt.one << 64;
final BigInt _maxUnsignedInt64 = _uint64Modulus - BigInt.one;
final BigInt _maxSignedInt64 = (BigInt.one << 63) - BigInt.one;

/// Call-scoped native geometry descriptor.
final class NativeGeometry {
  const NativeGeometry(this.pointer);

  /// Pointer to the root native geometry descriptor.
  final Pointer<raw.mln_geometry> pointer;
}

/// Call-scoped native GeoJSON descriptor.
final class NativeGeoJson {
  const NativeGeoJson(this.pointer);

  /// Pointer to the root native GeoJSON descriptor.
  final Pointer<raw.mln_geojson> pointer;
}

/// Materializes [value] into arena-owned native geometry descriptor storage.
NativeGeometry nativeGeometry(Geometry value, Allocator allocator) {
  final pointer = allocator<raw.mln_geometry>();
  _writeGeometry(pointer, value, allocator, 0);
  return NativeGeometry(pointer);
}

/// Materializes [value] into arena-owned native GeoJSON descriptor storage.
NativeGeoJson nativeGeoJson(GeoJson value, Allocator allocator) {
  final pointer = allocator<raw.mln_geojson>();
  _writeGeoJson(pointer, value, allocator);
  return NativeGeoJson(pointer);
}

void _writeGeometry(
  Pointer<raw.mln_geometry> out,
  Geometry value,
  Allocator allocator,
  int depth,
) {
  _checkGeometryDepth(depth);
  out.ref.size = sizeOf<raw.mln_geometry>();
  switch (value) {
    case EmptyGeometry():
      out.ref.type = raw.mln_geometry_type.MLN_GEOMETRY_TYPE_EMPTY.value;
    case PointGeometry(:final coordinate):
      out.ref.type = raw.mln_geometry_type.MLN_GEOMETRY_TYPE_POINT.value;
      out.ref.data.point = latLngToNative(coordinate);
    case LineStringGeometry(:final coordinates):
      out.ref.type = raw.mln_geometry_type.MLN_GEOMETRY_TYPE_LINE_STRING.value;
      out.ref.data.line_string = _coordinateSpan(coordinates, allocator);
    case PolygonGeometry(:final rings):
      out.ref.type = raw.mln_geometry_type.MLN_GEOMETRY_TYPE_POLYGON.value;
      out.ref.data.polygon = _polygonGeometry(rings, allocator);
    case MultiPointGeometry(:final coordinates):
      out.ref.type = raw.mln_geometry_type.MLN_GEOMETRY_TYPE_MULTI_POINT.value;
      out.ref.data.multi_point = _coordinateSpan(coordinates, allocator);
    case MultiLineStringGeometry(:final lines):
      out.ref.type =
          raw.mln_geometry_type.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING.value;
      final nativeLines = lines.isEmpty
          ? nullptr.cast<raw.mln_coordinate_span>()
          : allocator<raw.mln_coordinate_span>(lines.length);
      for (var index = 0; index < lines.length; index += 1) {
        nativeLines[index] = _coordinateSpan(lines[index], allocator);
      }
      final multiLine = Struct.create<raw.mln_multi_line_geometry>();
      multiLine.lines = nativeLines;
      multiLine.line_count = lines.length;
      out.ref.data.multi_line_string = multiLine;
    case MultiPolygonGeometry(:final polygons):
      out.ref.type =
          raw.mln_geometry_type.MLN_GEOMETRY_TYPE_MULTI_POLYGON.value;
      final nativePolygons = polygons.isEmpty
          ? nullptr.cast<raw.mln_polygon_geometry>()
          : allocator<raw.mln_polygon_geometry>(polygons.length);
      for (var index = 0; index < polygons.length; index += 1) {
        nativePolygons[index] = _polygonGeometry(polygons[index], allocator);
      }
      final multiPolygon = Struct.create<raw.mln_multi_polygon_geometry>();
      multiPolygon.polygons = nativePolygons;
      multiPolygon.polygon_count = polygons.length;
      out.ref.data.multi_polygon = multiPolygon;
    case GeometryCollection(:final geometries):
      out.ref.type =
          raw.mln_geometry_type.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION.value;
      final nativeGeometries = geometries.isEmpty
          ? nullptr.cast<raw.mln_geometry>()
          : allocator<raw.mln_geometry>(geometries.length);
      for (var index = 0; index < geometries.length; index += 1) {
        _writeGeometry(
          nativeGeometries + index,
          geometries[index],
          allocator,
          depth + 1,
        );
      }
      final collection = Struct.create<raw.mln_geometry_collection>();
      collection.geometries = nativeGeometries;
      collection.geometry_count = geometries.length;
      out.ref.data.geometry_collection = collection;
  }
}

raw.mln_coordinate_span _coordinateSpan(
  List<LatLng> coordinates,
  Allocator allocator,
) {
  final nativeCoordinates = coordinates.isEmpty
      ? nullptr.cast<raw.mln_lat_lng>()
      : allocator<raw.mln_lat_lng>(coordinates.length);
  for (var index = 0; index < coordinates.length; index += 1) {
    nativeCoordinates[index] = latLngToNative(coordinates[index]);
  }
  final span = Struct.create<raw.mln_coordinate_span>();
  span.coordinates = nativeCoordinates;
  span.coordinate_count = coordinates.length;
  return span;
}

raw.mln_polygon_geometry _polygonGeometry(
  List<List<LatLng>> rings,
  Allocator allocator,
) {
  final nativeRings = rings.isEmpty
      ? nullptr.cast<raw.mln_coordinate_span>()
      : allocator<raw.mln_coordinate_span>(rings.length);
  for (var index = 0; index < rings.length; index += 1) {
    nativeRings[index] = _coordinateSpan(rings[index], allocator);
  }
  final polygon = Struct.create<raw.mln_polygon_geometry>();
  polygon.rings = nativeRings;
  polygon.ring_count = rings.length;
  return polygon;
}

void _writeGeoJson(
  Pointer<raw.mln_geojson> out,
  GeoJson value,
  Allocator allocator,
) {
  out.ref.size = sizeOf<raw.mln_geojson>();
  switch (value) {
    case GeometryGeoJson(:final geometry):
      out.ref.type = raw.mln_geojson_type.MLN_GEOJSON_TYPE_GEOMETRY.value;
      out.ref.data.geometry = nativeGeometry(geometry, allocator).pointer;
    case FeatureGeoJson():
      out.ref.type = raw.mln_geojson_type.MLN_GEOJSON_TYPE_FEATURE.value;
      out.ref.data.feature = _feature(value, allocator);
    case FeatureCollectionGeoJson(:final features):
      out.ref.type =
          raw.mln_geojson_type.MLN_GEOJSON_TYPE_FEATURE_COLLECTION.value;
      final nativeFeatures = features.isEmpty
          ? nullptr.cast<raw.mln_feature>()
          : allocator<raw.mln_feature>(features.length);
      for (var index = 0; index < features.length; index += 1) {
        _writeFeature(nativeFeatures + index, features[index], allocator);
      }
      final collection = Struct.create<raw.mln_feature_collection>();
      collection.features = nativeFeatures;
      collection.feature_count = features.length;
      out.ref.data.feature_collection = collection;
  }
}

Pointer<raw.mln_feature> _feature(FeatureGeoJson value, Allocator allocator) {
  final pointer = allocator<raw.mln_feature>();
  _writeFeature(pointer, value, allocator);
  return pointer;
}

void _writeFeature(
  Pointer<raw.mln_feature> out,
  FeatureGeoJson value,
  Allocator allocator,
) {
  out.ref.size = sizeOf<raw.mln_feature>();
  out.ref.geometry = nativeGeometry(value.geometry, allocator).pointer;
  out.ref.properties = _jsonMembers(value.properties, allocator);
  out.ref.property_count = value.properties.length;
  _writeFeatureIdentifier(out.ref, value.identifier, allocator);
}

Pointer<raw.mln_json_member> _jsonMembers(
  List<JsonMember> members,
  Allocator allocator,
) {
  if (members.isEmpty) {
    return nullptr.cast<raw.mln_json_member>();
  }
  final nativeMembers = allocator<raw.mln_json_member>(members.length);
  for (var index = 0; index < members.length; index += 1) {
    final member = members[index];
    nativeMembers[index].key = nativeStringView(member.key, allocator).value;
    nativeMembers[index].value = nativeJsonValue(
      member.value,
      allocator,
    ).pointer;
  }
  return nativeMembers;
}

void _writeFeatureIdentifier(
  raw.mln_feature out,
  FeatureIdentifier identifier,
  Allocator allocator,
) {
  switch (identifier) {
    case NullFeatureIdentifier():
      out.identifier_type = raw
          .mln_feature_identifier_type
          .MLN_FEATURE_IDENTIFIER_TYPE_NULL
          .value;
    case UIntFeatureIdentifier(:final value):
      final nativeValue = _uint64ToNative(
        value,
        'feature identifiers with unsigned integer values',
      );
      out.identifier_type = raw
          .mln_feature_identifier_type
          .MLN_FEATURE_IDENTIFIER_TYPE_UINT
          .value;
      out.identifier.uint_value = nativeValue;
    case IntFeatureIdentifier(:final value):
      out.identifier_type =
          raw.mln_feature_identifier_type.MLN_FEATURE_IDENTIFIER_TYPE_INT.value;
      out.identifier.int_value = value;
    case DoubleFeatureIdentifier(:final value):
      if (!value.isFinite) {
        throwInvalidArgument(
          'feature identifiers with double values must be finite',
        );
      }
      out.identifier_type = raw
          .mln_feature_identifier_type
          .MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE
          .value;
      out.identifier.double_value = value;
    case StringFeatureIdentifier(:final value):
      out.identifier_type = raw
          .mln_feature_identifier_type
          .MLN_FEATURE_IDENTIFIER_TYPE_STRING
          .value;
      out.identifier.string_value = nativeStringView(value, allocator).value;
  }
}

/// Copies a borrowed native geometry descriptor into an owned Dart value.
Geometry geometryFromNative(raw.mln_geometry value) =>
    _geometryFromNative(value, 0);

Geometry _geometryFromNative(raw.mln_geometry value, int depth) {
  _checkGeometryDepth(depth);
  switch (value.type) {
    case 0:
      return const EmptyGeometry();
    case 1:
      return PointGeometry(latLngFromNative(value.data.point));
    case 2:
      return LineStringGeometry(_copyCoordinateSpan(value.data.line_string));
    case 3:
      return PolygonGeometry(_copyPolygonGeometry(value.data.polygon));
    case 4:
      return MultiPointGeometry(_copyCoordinateSpan(value.data.multi_point));
    case 5:
      final multiLine = value.data.multi_line_string;
      return MultiLineStringGeometry([
        for (var index = 0; index < multiLine.line_count; index += 1)
          _copyCoordinateSpan(multiLine.lines[index]),
      ]);
    case 6:
      final multiPolygon = value.data.multi_polygon;
      return MultiPolygonGeometry([
        for (var index = 0; index < multiPolygon.polygon_count; index += 1)
          _copyPolygonGeometry(multiPolygon.polygons[index]),
      ]);
    case 7:
      final collection = value.data.geometry_collection;
      return GeometryCollection([
        for (var index = 0; index < collection.geometry_count; index += 1)
          _geometryFromNative(collection.geometries[index], depth + 1),
      ]);
    default:
      throwInvalidArgument('unknown native geometry type: ${value.type}');
  }
}

List<LatLng> _copyCoordinateSpan(raw.mln_coordinate_span span) => [
  for (var index = 0; index < span.coordinate_count; index += 1)
    latLngFromNative(span.coordinates[index]),
];

List<List<LatLng>> _copyPolygonGeometry(raw.mln_polygon_geometry polygon) => [
  for (var index = 0; index < polygon.ring_count; index += 1)
    _copyCoordinateSpan(polygon.rings[index]),
];

/// Copies a borrowed native GeoJSON descriptor into an owned Dart value.
GeoJson geoJsonFromNative(raw.mln_geojson value) {
  switch (value.type) {
    case 1:
      return GeometryGeoJson(geometryFromNative(value.data.geometry.ref));
    case 2:
      return _featureFromNative(value.data.feature.ref);
    case 3:
      final collection = value.data.feature_collection;
      return FeatureCollectionGeoJson([
        for (var index = 0; index < collection.feature_count; index += 1)
          _featureFromNative(collection.features[index]),
      ]);
    default:
      throwInvalidArgument('unknown native GeoJSON type: ${value.type}');
  }
}

/// Copies a borrowed native feature descriptor into an owned Dart value.
FeatureGeoJson featureGeoJsonFromNative(raw.mln_feature value) =>
    _featureFromNative(value);

FeatureGeoJson _featureFromNative(raw.mln_feature value) => FeatureGeoJson(
  geometry: geometryFromNative(value.geometry.ref),
  properties: [
    for (var index = 0; index < value.property_count; index += 1)
      JsonMember(
        _copyStringView(value.properties[index].key),
        jsonValueFromNative(value.properties[index].value.ref),
      ),
  ],
  identifier: _featureIdentifierFromNative(value),
);

/// Copies a borrowed native feature collection into owned Dart values.
List<FeatureGeoJson> featureCollectionFromNative(
  raw.mln_feature_collection collection,
) => [
  for (var index = 0; index < collection.feature_count; index += 1)
    _featureFromNative(collection.features[index]),
];

FeatureIdentifier _featureIdentifierFromNative(raw.mln_feature value) {
  switch (value.identifier_type) {
    case 0:
      return const NullFeatureIdentifier();
    case 1:
      return UIntFeatureIdentifier.fromBigInt(
        _uint64FromNative(value.identifier.uint_value),
      );
    case 2:
      return IntFeatureIdentifier(value.identifier.int_value);
    case 3:
      return DoubleFeatureIdentifier(value.identifier.double_value);
    case 4:
      return StringFeatureIdentifier(
        _copyStringView(value.identifier.string_value),
      );
    default:
      throwInvalidArgument(
        'unknown native feature identifier type: ${value.identifier_type}',
      );
  }
}

int _uint64ToNative(BigInt value, String name) {
  if (value < BigInt.zero || value > _maxUnsignedInt64) {
    throwInvalidArgument('$name must be between 0 and $_maxUnsignedInt64');
  }
  final signedBits = value > _maxSignedInt64 ? value - _uint64Modulus : value;
  return signedBits.toInt();
}

BigInt _uint64FromNative(int value) {
  final bits = BigInt.from(value);
  return value < 0 ? bits + _uint64Modulus : bits;
}

String _copyStringView(raw.mln_string_view view) {
  if (view.data == nullptr || view.size == 0) {
    return '';
  }
  return view.data.cast<Utf8>().toDartString(length: view.size);
}

void _checkGeometryDepth(int depth) {
  if (depth > maxGeometryDescriptorDepth) {
    throwInvalidArgument(
      'geometry descriptors may not exceed depth $maxGeometryDescriptorDepth',
    );
  }
}
