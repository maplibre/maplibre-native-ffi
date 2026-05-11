package org.maplibre.nativeffi.internal.struct;

import static org.maplibre.nativeffi.internal.struct.CoreStructs.latLng;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.latLngArray;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.sizedArray;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.sizedPointer;
import static org.maplibre.nativeffi.internal.struct.CoreStructs.stringView;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import org.maplibre.nativeffi.geo.Feature;
import org.maplibre.nativeffi.geo.FeatureIdentifier;
import org.maplibre.nativeffi.geo.GeoJson;
import org.maplibre.nativeffi.geo.Geometry;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_coordinate_span;
import org.maplibre.nativeffi.internal.c.mln_feature;
import org.maplibre.nativeffi.internal.c.mln_feature_collection;
import org.maplibre.nativeffi.internal.c.mln_geojson;
import org.maplibre.nativeffi.internal.c.mln_geometry;
import org.maplibre.nativeffi.internal.c.mln_geometry_collection;
import org.maplibre.nativeffi.internal.c.mln_json_array;
import org.maplibre.nativeffi.internal.c.mln_json_member;
import org.maplibre.nativeffi.internal.c.mln_json_object;
import org.maplibre.nativeffi.internal.c.mln_json_value;
import org.maplibre.nativeffi.internal.c.mln_lat_lng;
import org.maplibre.nativeffi.internal.c.mln_multi_line_geometry;
import org.maplibre.nativeffi.internal.c.mln_multi_polygon_geometry;
import org.maplibre.nativeffi.internal.c.mln_polygon_geometry;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.internal.status.Status;
import org.maplibre.nativeffi.json.JsonValue;

/** Internal materializers and readers for JSON, geometry, feature, and GeoJSON values. */
public final class ValueStructs {
  private ValueStructs() {}

  public static MemorySegment jsonValue(JsonValue value, Arena arena) {
    var segment = mln_json_value.allocate(arena);
    writeJsonValue(segment, value, arena, 0);
    return segment;
  }

  public static JsonValue jsonValue(MemorySegment segment) {
    return readJsonValue(segment.reinterpret(mln_json_value.sizeof()), 0);
  }

  public static MemorySegment geometry(Geometry geometry, Arena arena) {
    return geometry(geometry, arena, 0);
  }

  public static Geometry geometry(MemorySegment segment) {
    return readGeometry(segment.reinterpret(mln_geometry.sizeof()), 0);
  }

  public static MemorySegment feature(Feature feature, Arena arena) {
    return feature(feature, arena, 0);
  }

  public static Feature feature(MemorySegment segment) {
    return readFeature(segment.reinterpret(mln_feature.sizeof()), 0);
  }

  public static MemorySegment geoJson(GeoJson geoJson, Arena arena) {
    var segment = mln_geojson.allocate(arena);
    writeGeoJson(segment, geoJson, arena);
    return segment;
  }

  public static GeoJson geoJson(MemorySegment segment) {
    return readGeoJson(segment.reinterpret(mln_geojson.sizeof()));
  }

  public static Optional<JsonValue> jsonSnapshot(MemorySegment snapshot) {
    if (MemoryUtil.isNull(snapshot)) {
      return Optional.empty();
    }
    try (var arena = Arena.ofConfined()) {
      var outValue = MemoryUtil.allocatePointer(arena);
      Status.check(MapLibreNativeC.mln_json_snapshot_get(snapshot, outValue));
      var value = outValue.get(java.lang.foreign.ValueLayout.ADDRESS, 0);
      return MemoryUtil.isNull(value) ? Optional.empty() : Optional.of(jsonValue(value));
    } finally {
      MapLibreNativeC.mln_json_snapshot_destroy(snapshot);
    }
  }

  private static void writeJsonValue(
      MemorySegment segment, JsonValue value, Arena arena, int depth) {
    if (depth > JsonValue.MAX_DESCRIPTOR_DEPTH) {
      throw new IllegalArgumentException("JSON descriptor depth exceeds 64");
    }
    mln_json_value.size(segment, (int) mln_json_value.sizeof());
    var data = mln_json_value.data(segment);
    if (value instanceof JsonValue.Null) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_NULL());
    } else if (value instanceof JsonValue.Bool boolValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_BOOL());
      mln_json_value.data.bool_value(data, boolValue.value());
    } else if (value instanceof JsonValue.UInt uintValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_UINT());
      mln_json_value.data.uint_value(data, uintValue.value());
    } else if (value instanceof JsonValue.Int intValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_INT());
      mln_json_value.data.int_value(data, intValue.value());
    } else if (value instanceof JsonValue.DoubleValue doubleValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_DOUBLE());
      mln_json_value.data.double_value(data, doubleValue.value());
    } else if (value instanceof JsonValue.StringValue stringValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_STRING());
      mln_json_value.data.string_value(data, stringView(stringValue.value(), arena));
    } else if (value instanceof JsonValue.Array arrayValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_ARRAY());
      var values = arrayValue.values();
      var array = mln_json_value.data.array_value(data);
      if (!values.isEmpty()) {
        var nativeValues = mln_json_value.allocateArray(values.size(), arena);
        for (var index = 0; index < values.size(); index++) {
          writeJsonValue(
              mln_json_value.asSlice(nativeValues, index), values.get(index), arena, depth + 1);
        }
        mln_json_array.values(array, nativeValues);
      }
      mln_json_array.value_count(array, values.size());
    } else if (value instanceof JsonValue.ObjectValue objectValue) {
      mln_json_value.type(segment, MapLibreNativeC.MLN_JSON_VALUE_TYPE_OBJECT());
      var members = objectValue.members();
      var object = mln_json_value.data.object_value(data);
      if (!members.isEmpty()) {
        mln_json_object.members(object, jsonMembers(members, arena, depth + 1));
      }
      mln_json_object.member_count(object, members.size());
    } else {
      throw new IllegalArgumentException(
          "Unsupported JSON value type: " + value.getClass().getName());
    }
  }

  private static JsonValue readJsonValue(MemorySegment segment, int depth) {
    if (depth > JsonValue.MAX_DESCRIPTOR_DEPTH) {
      throw new IllegalArgumentException("JSON descriptor depth exceeds 64");
    }
    var data = mln_json_value.data(segment);
    var type = mln_json_value.type(segment);
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_NULL()) {
      return JsonValue.nullValue();
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_BOOL()) {
      return JsonValue.of(mln_json_value.data.bool_value(data));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_UINT()) {
      return JsonValue.unsigned(mln_json_value.data.uint_value(data));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_INT()) {
      return JsonValue.of(mln_json_value.data.int_value(data));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_DOUBLE()) {
      return JsonValue.of(mln_json_value.data.double_value(data));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_STRING()) {
      return JsonValue.of(stringView(mln_json_value.data.string_value(data)));
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_ARRAY()) {
      var array = mln_json_value.data.array_value(data);
      var count = Math.toIntExact(mln_json_array.value_count(array));
      var values =
          sizedArray(
              mln_json_array.values(array), count, mln_json_value.sizeof(), "JSON array values");
      var copied = new ArrayList<JsonValue>(count);
      for (var index = 0; index < count; index++) {
        copied.add(readJsonValue(mln_json_value.asSlice(values, index), depth + 1));
      }
      return JsonValue.array(copied);
    }
    if (type == MapLibreNativeC.MLN_JSON_VALUE_TYPE_OBJECT()) {
      var object = mln_json_value.data.object_value(data);
      var count = Math.toIntExact(mln_json_object.member_count(object));
      return JsonValue.object(readJsonMembers(mln_json_object.members(object), count, depth + 1));
    }
    throw new IllegalArgumentException("Unknown JSON value type: " + Integer.toUnsignedLong(type));
  }

  private static MemorySegment jsonMembers(List<JsonValue.Member> members, Arena arena, int depth) {
    var nativeMembers = mln_json_member.allocateArray(members.size(), arena);
    for (var index = 0; index < members.size(); index++) {
      var member = members.get(index);
      var nativeMember = mln_json_member.asSlice(nativeMembers, index);
      mln_json_member.key(nativeMember, stringView(member.key(), arena));
      var nativeValue = mln_json_value.allocate(arena);
      writeJsonValue(nativeValue, member.value(), arena, depth);
      mln_json_member.value(nativeMember, nativeValue);
    }
    return nativeMembers;
  }

  private static List<JsonValue.Member> readJsonMembers(
      MemorySegment members, int count, int depth) {
    var sizedMembers = sizedArray(members, count, mln_json_member.sizeof(), "JSON object members");
    var copied = new ArrayList<JsonValue.Member>(count);
    for (var index = 0; index < count; index++) {
      var member = mln_json_member.asSlice(sizedMembers, index);
      var value =
          sizedPointer(mln_json_member.value(member), mln_json_value.sizeof(), "JSON member value");
      copied.add(
          new JsonValue.Member(
              stringView(mln_json_member.key(member)), readJsonValue(value, depth)));
    }
    return List.copyOf(copied);
  }

  private static void writeGeometry(
      MemorySegment segment, Geometry geometry, Arena arena, int depth) {
    if (depth > Geometry.MAX_COLLECTION_DEPTH) {
      throw new IllegalArgumentException("geometry collection depth exceeds 64");
    }
    mln_geometry.size(segment, (int) mln_geometry.sizeof());
    var data = mln_geometry.data(segment);
    if (geometry instanceof Geometry.Empty) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_EMPTY());
    } else if (geometry instanceof Geometry.Point point) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_POINT());
      mln_geometry.data.point(data, latLng(point.coordinate(), arena));
    } else if (geometry instanceof Geometry.LineString lineString) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING());
      mln_geometry.data.line_string(data, coordinateSpan(lineString.coordinates(), arena));
    } else if (geometry instanceof Geometry.Polygon polygon) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_POLYGON());
      mln_geometry.data.polygon(data, polygonGeometry(polygon.rings(), arena));
    } else if (geometry instanceof Geometry.MultiPoint multiPoint) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT());
      mln_geometry.data.multi_point(data, coordinateSpan(multiPoint.coordinates(), arena));
    } else if (geometry instanceof Geometry.MultiLineString multiLineString) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING());
      mln_geometry.data.multi_line_string(data, multiLineGeometry(multiLineString.lines(), arena));
    } else if (geometry instanceof Geometry.MultiPolygon multiPolygon) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON());
      mln_geometry.data.multi_polygon(data, multiPolygonGeometry(multiPolygon.polygons(), arena));
    } else if (geometry instanceof Geometry.Collection collection) {
      mln_geometry.type(segment, MapLibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION());
      var nativeCollection = mln_geometry.data.geometry_collection(data);
      var geometries = collection.geometries();
      if (!geometries.isEmpty()) {
        var nativeGeometries = mln_geometry.allocateArray(geometries.size(), arena);
        for (var index = 0; index < geometries.size(); index++) {
          writeGeometry(
              mln_geometry.asSlice(nativeGeometries, index),
              geometries.get(index),
              arena,
              depth + 1);
        }
        mln_geometry_collection.geometries(nativeCollection, nativeGeometries);
      }
      mln_geometry_collection.geometry_count(nativeCollection, geometries.size());
    } else {
      throw new IllegalArgumentException(
          "Unsupported geometry type: " + geometry.getClass().getName());
    }
  }

  private static Geometry readGeometry(MemorySegment segment, int depth) {
    if (depth > Geometry.MAX_COLLECTION_DEPTH) {
      throw new IllegalArgumentException("geometry collection depth exceeds 64");
    }
    var data = mln_geometry.data(segment);
    var type = mln_geometry.type(segment);
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_EMPTY()) {
      return Geometry.empty();
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_POINT()) {
      return Geometry.point(latLng(mln_geometry.data.point(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_LINE_STRING()) {
      return Geometry.lineString(coordinateSpan(mln_geometry.data.line_string(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_POLYGON()) {
      return Geometry.polygon(polygonGeometry(mln_geometry.data.polygon(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POINT()) {
      return Geometry.multiPoint(coordinateSpan(mln_geometry.data.multi_point(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_LINE_STRING()) {
      return Geometry.multiLineString(multiLineGeometry(mln_geometry.data.multi_line_string(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_MULTI_POLYGON()) {
      return Geometry.multiPolygon(multiPolygonGeometry(mln_geometry.data.multi_polygon(data)));
    }
    if (type == MapLibreNativeC.MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION()) {
      var collection = mln_geometry.data.geometry_collection(data);
      var count = Math.toIntExact(mln_geometry_collection.geometry_count(collection));
      var geometries =
          sizedArray(
              mln_geometry_collection.geometries(collection),
              count,
              mln_geometry.sizeof(),
              "geometry collection");
      var copied = new ArrayList<Geometry>(count);
      for (var index = 0; index < count; index++) {
        copied.add(readGeometry(mln_geometry.asSlice(geometries, index), depth + 1));
      }
      return Geometry.collection(copied);
    }
    throw new IllegalArgumentException("Unknown geometry type: " + Integer.toUnsignedLong(type));
  }

  private static MemorySegment coordinateSpan(List<LatLng> coordinates, Arena arena) {
    var span = mln_coordinate_span.allocate(arena);
    if (!coordinates.isEmpty()) {
      mln_coordinate_span.coordinates(span, latLngArray(coordinates, arena));
    }
    mln_coordinate_span.coordinate_count(span, coordinates.size());
    return span;
  }

  private static List<LatLng> coordinateSpan(MemorySegment span) {
    var count = Math.toIntExact(mln_coordinate_span.coordinate_count(span));
    var coordinates =
        sizedArray(
            mln_coordinate_span.coordinates(span), count, mln_lat_lng.sizeof(), "coordinates");
    return latLngArray(coordinates, count);
  }

  private static MemorySegment polygonGeometry(List<List<LatLng>> rings, Arena arena) {
    var polygon = mln_polygon_geometry.allocate(arena);
    if (!rings.isEmpty()) {
      var nativeRings = coordinateSpans(rings, arena);
      mln_polygon_geometry.rings(polygon, nativeRings);
    }
    mln_polygon_geometry.ring_count(polygon, rings.size());
    return polygon;
  }

  private static List<List<LatLng>> polygonGeometry(MemorySegment polygon) {
    var count = Math.toIntExact(mln_polygon_geometry.ring_count(polygon));
    return coordinateSpans(mln_polygon_geometry.rings(polygon), count);
  }

  private static MemorySegment multiLineGeometry(List<List<LatLng>> lines, Arena arena) {
    var multiLine = mln_multi_line_geometry.allocate(arena);
    if (!lines.isEmpty()) {
      mln_multi_line_geometry.lines(multiLine, coordinateSpans(lines, arena));
    }
    mln_multi_line_geometry.line_count(multiLine, lines.size());
    return multiLine;
  }

  private static List<List<LatLng>> multiLineGeometry(MemorySegment multiLine) {
    var count = Math.toIntExact(mln_multi_line_geometry.line_count(multiLine));
    return coordinateSpans(mln_multi_line_geometry.lines(multiLine), count);
  }

  private static MemorySegment multiPolygonGeometry(
      List<List<List<LatLng>>> polygons, Arena arena) {
    var multiPolygon = mln_multi_polygon_geometry.allocate(arena);
    if (!polygons.isEmpty()) {
      var nativePolygons = mln_polygon_geometry.allocateArray(polygons.size(), arena);
      for (var index = 0; index < polygons.size(); index++) {
        mln_polygon_geometry
            .asSlice(nativePolygons, index)
            .copyFrom(polygonGeometry(polygons.get(index), arena));
      }
      mln_multi_polygon_geometry.polygons(multiPolygon, nativePolygons);
    }
    mln_multi_polygon_geometry.polygon_count(multiPolygon, polygons.size());
    return multiPolygon;
  }

  private static List<List<List<LatLng>>> multiPolygonGeometry(MemorySegment multiPolygon) {
    var count = Math.toIntExact(mln_multi_polygon_geometry.polygon_count(multiPolygon));
    var polygons =
        sizedArray(
            mln_multi_polygon_geometry.polygons(multiPolygon),
            count,
            mln_polygon_geometry.sizeof(),
            "multi-polygon polygons");
    var copied = new ArrayList<List<List<LatLng>>>(count);
    for (var index = 0; index < count; index++) {
      copied.add(polygonGeometry(mln_polygon_geometry.asSlice(polygons, index)));
    }
    return List.copyOf(copied);
  }

  private static MemorySegment coordinateSpans(List<List<LatLng>> spans, Arena arena) {
    var nativeSpans = mln_coordinate_span.allocateArray(spans.size(), arena);
    for (var index = 0; index < spans.size(); index++) {
      mln_coordinate_span
          .asSlice(nativeSpans, index)
          .copyFrom(coordinateSpan(spans.get(index), arena));
    }
    return nativeSpans;
  }

  private static List<List<LatLng>> coordinateSpans(MemorySegment spans, int count) {
    var nativeSpans = sizedArray(spans, count, mln_coordinate_span.sizeof(), "coordinate spans");
    var copied = new ArrayList<List<LatLng>>(count);
    for (var index = 0; index < count; index++) {
      copied.add(coordinateSpan(mln_coordinate_span.asSlice(nativeSpans, index)));
    }
    return List.copyOf(copied);
  }

  private static MemorySegment geometry(Geometry geometry, Arena arena, int depth) {
    var segment = mln_geometry.allocate(arena);
    writeGeometry(segment, geometry, arena, depth);
    return segment;
  }

  private static MemorySegment feature(Feature feature, Arena arena, int depth) {
    var segment = mln_feature.allocate(arena);
    writeFeature(segment, feature, arena, depth);
    return segment;
  }

  private static void writeFeature(MemorySegment segment, Feature feature, Arena arena, int depth) {
    mln_feature.size(segment, (int) mln_feature.sizeof());
    mln_feature.geometry(segment, geometry(feature.geometry(), arena, depth + 1));
    var properties = feature.properties();
    if (!properties.isEmpty()) {
      mln_feature.properties(segment, jsonMembers(properties, arena, depth + 1));
    }
    mln_feature.property_count(segment, properties.size());
    writeFeatureIdentifier(segment, feature.identifier(), arena);
  }

  private static Feature readFeature(MemorySegment segment, int depth) {
    var geometry =
        readGeometry(
            sizedPointer(mln_feature.geometry(segment), mln_geometry.sizeof(), "feature geometry"),
            depth + 1);
    var propertyCount = Math.toIntExact(mln_feature.property_count(segment));
    var properties = readJsonMembers(mln_feature.properties(segment), propertyCount, depth + 1);
    return new Feature(geometry, properties, readFeatureIdentifier(segment));
  }

  private static void writeFeatureIdentifier(
      MemorySegment segment, FeatureIdentifier identifier, Arena arena) {
    var data = mln_feature.identifier(segment);
    if (identifier instanceof FeatureIdentifier.Null) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_NULL());
    } else if (identifier instanceof FeatureIdentifier.UInt value) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_UINT());
      mln_feature.identifier.uint_value(data, value.value());
    } else if (identifier instanceof FeatureIdentifier.Int value) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_INT());
      mln_feature.identifier.int_value(data, value.value());
    } else if (identifier instanceof FeatureIdentifier.DoubleValue value) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE());
      mln_feature.identifier.double_value(data, value.value());
    } else if (identifier instanceof FeatureIdentifier.StringValue value) {
      mln_feature.identifier_type(segment, MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_STRING());
      mln_feature.identifier.string_value(data, stringView(value.value(), arena));
    } else {
      throw new IllegalArgumentException(
          "Unsupported feature identifier type: " + identifier.getClass().getName());
    }
  }

  private static FeatureIdentifier readFeatureIdentifier(MemorySegment segment) {
    var data = mln_feature.identifier(segment);
    var type = mln_feature.identifier_type(segment);
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_NULL()) {
      return FeatureIdentifier.nullValue();
    }
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_UINT()) {
      return FeatureIdentifier.unsigned(mln_feature.identifier.uint_value(data));
    }
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_INT()) {
      return FeatureIdentifier.of(mln_feature.identifier.int_value(data));
    }
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_DOUBLE()) {
      return FeatureIdentifier.of(mln_feature.identifier.double_value(data));
    }
    if (type == MapLibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_STRING()) {
      return FeatureIdentifier.of(stringView(mln_feature.identifier.string_value(data)));
    }
    throw new IllegalArgumentException(
        "Unknown feature identifier type: " + Integer.toUnsignedLong(type));
  }

  private static void writeGeoJson(MemorySegment segment, GeoJson geoJson, Arena arena) {
    mln_geojson.size(segment, (int) mln_geojson.sizeof());
    var data = mln_geojson.data(segment);
    if (geoJson instanceof GeoJson.GeometryValue geometryValue) {
      mln_geojson.type(segment, MapLibreNativeC.MLN_GEOJSON_TYPE_GEOMETRY());
      mln_geojson.data.geometry(data, geometry(geometryValue.geometry(), arena));
    } else if (geoJson instanceof GeoJson.FeatureValue featureValue) {
      mln_geojson.type(segment, MapLibreNativeC.MLN_GEOJSON_TYPE_FEATURE());
      mln_geojson.data.feature(data, feature(featureValue.feature(), arena, 0));
    } else if (geoJson instanceof GeoJson.FeatureCollection featureCollection) {
      mln_geojson.type(segment, MapLibreNativeC.MLN_GEOJSON_TYPE_FEATURE_COLLECTION());
      var features = featureCollection.features();
      var nativeCollection = mln_geojson.data.feature_collection(data);
      if (!features.isEmpty()) {
        var nativeFeatures = mln_feature.allocateArray(features.size(), arena);
        for (var index = 0; index < features.size(); index++) {
          writeFeature(mln_feature.asSlice(nativeFeatures, index), features.get(index), arena, 1);
        }
        mln_feature_collection.features(nativeCollection, nativeFeatures);
      }
      mln_feature_collection.feature_count(nativeCollection, features.size());
    } else {
      throw new IllegalArgumentException(
          "Unsupported GeoJSON type: " + geoJson.getClass().getName());
    }
  }

  private static GeoJson readGeoJson(MemorySegment segment) {
    var data = mln_geojson.data(segment);
    var type = mln_geojson.type(segment);
    if (type == MapLibreNativeC.MLN_GEOJSON_TYPE_GEOMETRY()) {
      return GeoJson.geometry(
          readGeometry(
              sizedPointer(
                  mln_geojson.data.geometry(data), mln_geometry.sizeof(), "GeoJSON geometry"),
              0));
    }
    if (type == MapLibreNativeC.MLN_GEOJSON_TYPE_FEATURE()) {
      return GeoJson.feature(
          readFeature(
              sizedPointer(mln_geojson.data.feature(data), mln_feature.sizeof(), "GeoJSON feature"),
              0));
    }
    if (type == MapLibreNativeC.MLN_GEOJSON_TYPE_FEATURE_COLLECTION()) {
      var collection = mln_geojson.data.feature_collection(data);
      var count = Math.toIntExact(mln_feature_collection.feature_count(collection));
      var features =
          sizedArray(
              mln_feature_collection.features(collection),
              count,
              mln_feature.sizeof(),
              "GeoJSON feature collection");
      var copied = new ArrayList<Feature>(count);
      for (var index = 0; index < count; index++) {
        copied.add(readFeature(mln_feature.asSlice(features, index), 1));
      }
      return GeoJson.featureCollection(copied);
    }
    throw new IllegalArgumentException("Unknown GeoJSON type: " + Integer.toUnsignedLong(type));
  }
}
