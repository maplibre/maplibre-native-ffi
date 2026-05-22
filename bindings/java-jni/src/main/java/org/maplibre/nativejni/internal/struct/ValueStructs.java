package org.maplibre.nativejni.internal.struct;

import java.util.List;
import java.util.Objects;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.geo.FeatureIdentifier;
import org.maplibre.nativejni.geo.GeoJson;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.json.JsonValue;

/** Materializes JSON, geometry, feature, and GeoJSON value trees for JNI calls. */
public final class ValueStructs {
  private ValueStructs() {}

  public sealed interface JsonNode
      permits JsonNull,
          JsonBool,
          JsonUInt,
          JsonInt,
          JsonDouble,
          JsonString,
          JsonArray,
          JsonObject {}

  public enum JsonNull implements JsonNode {
    INSTANCE
  }

  public record JsonBool(boolean value) implements JsonNode {}

  public record JsonUInt(long value) implements JsonNode {}

  public record JsonInt(long value) implements JsonNode {}

  public record JsonDouble(double value) implements JsonNode {}

  public record JsonString(String value) implements JsonNode {}

  public record JsonArray(List<JsonNode> values) implements JsonNode {
    public JsonArray {
      values = List.copyOf(values);
    }
  }

  public record JsonObject(List<JsonMemberNode> members) implements JsonNode {
    public JsonObject {
      members = List.copyOf(members);
    }
  }

  public record JsonMemberNode(String key, JsonNode value) {}

  public sealed interface GeometryNode
      permits GeometryEmpty,
          GeometryPoint,
          GeometryLineString,
          GeometryPolygon,
          GeometryMultiPoint,
          GeometryMultiLineString,
          GeometryMultiPolygon,
          GeometryCollection {}

  public enum GeometryEmpty implements GeometryNode {
    INSTANCE
  }

  public record GeometryPoint(CoreStructs.LatLngValue coordinate) implements GeometryNode {}

  public record GeometryLineString(List<CoreStructs.LatLngValue> coordinates)
      implements GeometryNode {
    public GeometryLineString {
      coordinates = List.copyOf(coordinates);
    }
  }

  public record GeometryPolygon(List<List<CoreStructs.LatLngValue>> rings) implements GeometryNode {
    public GeometryPolygon {
      rings = copyNested(rings);
    }
  }

  public record GeometryMultiPoint(List<CoreStructs.LatLngValue> coordinates)
      implements GeometryNode {
    public GeometryMultiPoint {
      coordinates = List.copyOf(coordinates);
    }
  }

  public record GeometryMultiLineString(List<List<CoreStructs.LatLngValue>> lines)
      implements GeometryNode {
    public GeometryMultiLineString {
      lines = copyNested(lines);
    }
  }

  public record GeometryMultiPolygon(List<List<List<CoreStructs.LatLngValue>>> polygons)
      implements GeometryNode {
    public GeometryMultiPolygon {
      polygons = copyDeep(polygons);
    }
  }

  public record GeometryCollection(List<GeometryNode> geometries) implements GeometryNode {
    public GeometryCollection {
      geometries = List.copyOf(geometries);
    }
  }

  public sealed interface FeatureIdentifierNode
      permits FeatureIdentifierNull,
          FeatureIdentifierUInt,
          FeatureIdentifierInt,
          FeatureIdentifierDouble,
          FeatureIdentifierString {}

  public enum FeatureIdentifierNull implements FeatureIdentifierNode {
    INSTANCE
  }

  public record FeatureIdentifierUInt(long value) implements FeatureIdentifierNode {}

  public record FeatureIdentifierInt(long value) implements FeatureIdentifierNode {}

  public record FeatureIdentifierDouble(double value) implements FeatureIdentifierNode {}

  public record FeatureIdentifierString(String value) implements FeatureIdentifierNode {}

  public record FeatureNode(
      GeometryNode geometry, List<JsonMemberNode> properties, FeatureIdentifierNode identifier) {
    public FeatureNode {
      properties = List.copyOf(properties);
    }
  }

  public sealed interface GeoJsonNode
      permits GeoJsonGeometry, GeoJsonFeature, GeoJsonFeatureCollection {}

  public record GeoJsonGeometry(GeometryNode geometry) implements GeoJsonNode {}

  public record GeoJsonFeature(FeatureNode feature) implements GeoJsonNode {}

  public record GeoJsonFeatureCollection(List<FeatureNode> features) implements GeoJsonNode {
    public GeoJsonFeatureCollection {
      features = List.copyOf(features);
    }
  }

  public static JsonNode jsonValue(JsonValue value) {
    return jsonValue(value, 0);
  }

  public static JsonValue jsonValue(JsonNode value) {
    Objects.requireNonNull(value, "value");
    return switch (value) {
      case JsonNull ignored -> JsonValue.nullValue();
      case JsonBool node -> JsonValue.of(node.value());
      case JsonUInt node -> JsonValue.unsigned(node.value());
      case JsonInt node -> JsonValue.of(node.value());
      case JsonDouble node -> JsonValue.of(node.value());
      case JsonString node -> JsonValue.of(node.value());
      case JsonArray node ->
          JsonValue.array(node.values().stream().map(ValueStructs::jsonValue).toList());
      case JsonObject node ->
          JsonValue.object(
              node.members().stream()
                  .map(member -> new JsonValue.Member(member.key(), jsonValue(member.value())))
                  .toList());
    };
  }

  public static GeometryNode geometry(Geometry value) {
    return geometry(value, 0);
  }

  public static Geometry geometry(GeometryNode value) {
    Objects.requireNonNull(value, "value");
    return switch (value) {
      case GeometryEmpty ignored -> Geometry.empty();
      case GeometryPoint node -> Geometry.point(CoreStructs.latLng(node.coordinate()));
      case GeometryLineString node ->
          Geometry.lineString(node.coordinates().stream().map(CoreStructs::latLng).toList());
      case GeometryPolygon node -> Geometry.polygon(copyNestedLatLngs(node.rings()));
      case GeometryMultiPoint node ->
          Geometry.multiPoint(node.coordinates().stream().map(CoreStructs::latLng).toList());
      case GeometryMultiLineString node ->
          Geometry.multiLineString(copyNestedLatLngs(node.lines()));
      case GeometryMultiPolygon node -> Geometry.multiPolygon(copyDeepLatLngs(node.polygons()));
      case GeometryCollection node ->
          Geometry.collection(node.geometries().stream().map(ValueStructs::geometry).toList());
    };
  }

  public static FeatureNode feature(Feature value) {
    return feature(value, 0);
  }

  public static Feature feature(FeatureNode value) {
    Objects.requireNonNull(value, "value");
    return new Feature(
        geometry(value.geometry()),
        value.properties().stream()
            .map(member -> new JsonValue.Member(member.key(), jsonValue(member.value())))
            .toList(),
        featureIdentifier(value.identifier()));
  }

  public static GeoJsonNode geoJson(GeoJson value) {
    return geoJson(value, 0, 0);
  }

  public static GeoJson geoJson(GeoJsonNode value) {
    Objects.requireNonNull(value, "value");
    return switch (value) {
      case GeoJsonGeometry node -> GeoJson.geometry(geometry(node.geometry()));
      case GeoJsonFeature node -> GeoJson.feature(feature(node.feature()));
      case GeoJsonFeatureCollection node ->
          GeoJson.featureCollection(node.features().stream().map(ValueStructs::feature).toList());
    };
  }

  private static JsonNode jsonValue(JsonValue value, int depth) {
    Objects.requireNonNull(value, "value");
    requireJsonDepth(depth);
    return switch (value) {
      case JsonValue.Null ignored -> JsonNull.INSTANCE;
      case JsonValue.Bool node -> new JsonBool(node.value());
      case JsonValue.UInt node -> new JsonUInt(node.value());
      case JsonValue.Int node -> new JsonInt(node.value());
      case JsonValue.DoubleValue node -> new JsonDouble(node.value());
      case JsonValue.StringValue node -> new JsonString(node.value());
      case JsonValue.Array node ->
          new JsonArray(node.values().stream().map(child -> jsonValue(child, depth + 1)).toList());
      case JsonValue.ObjectValue node ->
          new JsonObject(
              node.members().stream()
                  .map(
                      member ->
                          new JsonMemberNode(member.key(), jsonValue(member.value(), depth + 1)))
                  .toList());
    };
  }

  private static GeometryNode geometry(Geometry value, int depth) {
    Objects.requireNonNull(value, "value");
    requireGeometryDepth(depth);
    return switch (value) {
      case Geometry.Empty ignored -> GeometryEmpty.INSTANCE;
      case Geometry.Point node -> new GeometryPoint(CoreStructs.latLng(node.coordinate()));
      case Geometry.LineString node -> new GeometryLineString(copyLatLngs(node.coordinates()));
      case Geometry.Polygon node -> new GeometryPolygon(copyNestedCoreLatLngs(node.rings()));
      case Geometry.MultiPoint node -> new GeometryMultiPoint(copyLatLngs(node.coordinates()));
      case Geometry.MultiLineString node ->
          new GeometryMultiLineString(copyNestedCoreLatLngs(node.lines()));
      case Geometry.MultiPolygon node ->
          new GeometryMultiPolygon(copyDeepCoreLatLngs(node.polygons()));
      case Geometry.Collection node ->
          new GeometryCollection(
              node.geometries().stream().map(child -> geometry(child, depth + 1)).toList());
    };
  }

  private static FeatureNode feature(Feature value, int geometryDepth) {
    return feature(value, geometryDepth, 1);
  }

  private static FeatureNode feature(Feature value, int geometryDepth, int jsonDepth) {
    Objects.requireNonNull(value, "value");
    var geometry = geometry(value.geometry(), geometryDepth + 1);
    var properties =
        value.properties().stream()
            .map(member -> new JsonMemberNode(member.key(), jsonValue(member.value(), jsonDepth)))
            .toList();
    return new FeatureNode(geometry, properties, featureIdentifier(value.identifier()));
  }

  private static GeoJsonNode geoJson(GeoJson value, int geometryDepth, int jsonDepth) {
    Objects.requireNonNull(value, "value");
    return switch (value) {
      case GeoJson.GeometryValue node ->
          new GeoJsonGeometry(geometry(node.geometry(), geometryDepth));
      case GeoJson.FeatureValue node ->
          new GeoJsonFeature(feature(node.feature(), geometryDepth, jsonDepth + 1));
      case GeoJson.FeatureCollection node ->
          new GeoJsonFeatureCollection(
              node.features().stream()
                  .map(feature -> feature(feature, geometryDepth + 1, jsonDepth + 2))
                  .toList());
    };
  }

  private static FeatureIdentifierNode featureIdentifier(FeatureIdentifier value) {
    Objects.requireNonNull(value, "value");
    return switch (value) {
      case FeatureIdentifier.Null ignored -> FeatureIdentifierNull.INSTANCE;
      case FeatureIdentifier.UInt node -> new FeatureIdentifierUInt(node.value());
      case FeatureIdentifier.Int node -> new FeatureIdentifierInt(node.value());
      case FeatureIdentifier.DoubleValue node -> new FeatureIdentifierDouble(node.value());
      case FeatureIdentifier.StringValue node -> new FeatureIdentifierString(node.value());
    };
  }

  private static FeatureIdentifier featureIdentifier(FeatureIdentifierNode value) {
    Objects.requireNonNull(value, "value");
    return switch (value) {
      case FeatureIdentifierNull ignored -> FeatureIdentifier.nullValue();
      case FeatureIdentifierUInt node -> FeatureIdentifier.unsigned(node.value());
      case FeatureIdentifierInt node -> FeatureIdentifier.of(node.value());
      case FeatureIdentifierDouble node -> FeatureIdentifier.of(node.value());
      case FeatureIdentifierString node -> FeatureIdentifier.of(node.value());
    };
  }

  private static void requireJsonDepth(int depth) {
    if (depth > JsonValue.MAX_DESCRIPTOR_DEPTH) {
      throw new IllegalArgumentException("JSON descriptor exceeds maximum depth");
    }
  }

  private static void requireGeometryDepth(int depth) {
    if (depth > Geometry.MAX_COLLECTION_DEPTH) {
      throw new IllegalArgumentException("Geometry collection exceeds maximum depth");
    }
  }

  private static List<CoreStructs.LatLngValue> copyLatLngs(List<LatLng> coordinates) {
    return coordinates.stream().map(CoreStructs::latLng).toList();
  }

  private static List<List<CoreStructs.LatLngValue>> copyNestedCoreLatLngs(
      List<List<LatLng>> coordinates) {
    return coordinates.stream().map(ValueStructs::copyLatLngs).toList();
  }

  private static List<List<List<CoreStructs.LatLngValue>>> copyDeepCoreLatLngs(
      List<List<List<LatLng>>> coordinates) {
    return coordinates.stream().map(ValueStructs::copyNestedCoreLatLngs).toList();
  }

  private static List<LatLng> copyLatLngValues(List<CoreStructs.LatLngValue> coordinates) {
    return coordinates.stream().map(CoreStructs::latLng).toList();
  }

  private static List<List<LatLng>> copyNestedLatLngs(List<List<CoreStructs.LatLngValue>> values) {
    return values.stream().map(ValueStructs::copyLatLngValues).toList();
  }

  private static List<List<List<LatLng>>> copyDeepLatLngs(
      List<List<List<CoreStructs.LatLngValue>>> values) {
    return values.stream().map(ValueStructs::copyNestedLatLngs).toList();
  }

  private static <T> List<List<T>> copyNested(List<List<T>> values) {
    return List.copyOf(values.stream().map(List::copyOf).toList());
  }

  private static <T> List<List<List<T>>> copyDeep(List<List<List<T>>> values) {
    return List.copyOf(
        values.stream().map(polygons -> polygons.stream().map(List::copyOf).toList()).toList());
  }
}
