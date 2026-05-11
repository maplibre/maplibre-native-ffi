package org.maplibre.nativeffi.internal.struct;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.lang.foreign.Arena;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.geo.Feature;
import org.maplibre.nativeffi.geo.FeatureIdentifier;
import org.maplibre.nativeffi.geo.GeoJson;
import org.maplibre.nativeffi.geo.Geometry;
import org.maplibre.nativeffi.geo.LatLng;
import org.maplibre.nativeffi.json.JsonValue;

final class ValueStructsTest {
  @Test
  void jsonValuesMaterializeAndCopyBack() {
    var value =
        JsonValue.object(
            List.of(
                new JsonValue.Member("null", JsonValue.nullValue()),
                new JsonValue.Member("bool", JsonValue.of(true)),
                new JsonValue.Member("uint", JsonValue.unsigned(-1L)),
                new JsonValue.Member("int", JsonValue.of(-7L)),
                new JsonValue.Member("double", JsonValue.of(1.25)),
                new JsonValue.Member("string", JsonValue.of("hello\u0000world")),
                new JsonValue.Member(
                    "array", JsonValue.array(List.of(JsonValue.of("a"), JsonValue.of("b"))))));

    JsonValue copied;
    try (var arena = Arena.ofConfined()) {
      copied = ValueStructs.jsonValue(ValueStructs.jsonValue(value, arena));
    }

    assertEquals(value, copied);
  }

  @Test
  void geometryFeatureAndGeoJsonMaterializeAndCopyBack() {
    var polygon =
        Geometry.polygon(
            List.of(
                List.of(new LatLng(0, 0), new LatLng(0, 1), new LatLng(1, 1), new LatLng(0, 0))));
    var collection = Geometry.collection(List.of(Geometry.point(new LatLng(2, 3)), polygon));
    var feature =
        new Feature(
            collection,
            List.of(new JsonValue.Member("name", JsonValue.of("park"))),
            FeatureIdentifier.unsigned(42));
    var geoJson = GeoJson.featureCollection(List.of(feature));

    GeoJson copiedGeoJson;
    Feature copiedFeature;
    Geometry copiedGeometry;
    try (var arena = Arena.ofConfined()) {
      copiedGeometry = ValueStructs.geometry(ValueStructs.geometry(collection, arena));
      copiedFeature = ValueStructs.feature(ValueStructs.feature(feature, arena));
      copiedGeoJson = ValueStructs.geoJson(ValueStructs.geoJson(geoJson, arena));
    }

    assertEquals(collection, copiedGeometry);
    assertEquals(feature, copiedFeature);
    assertEquals(geoJson, copiedGeoJson);
  }

  @Test
  void descriptorDepthErrorsAreReportedInJava() {
    var tooDeepJson = nestedArray(JsonValue.MAX_DESCRIPTOR_DEPTH + 2);
    try (var arena = Arena.ofConfined()) {
      assertThrows(
          IllegalArgumentException.class, () -> ValueStructs.jsonValue(tooDeepJson, arena));
    }

    var tooDeepGeometry = nestedCollection(Geometry.MAX_COLLECTION_DEPTH + 2);
    try (var arena = Arena.ofConfined()) {
      assertThrows(
          IllegalArgumentException.class, () -> ValueStructs.geometry(tooDeepGeometry, arena));
    }
  }

  @Test
  void featureDepthCountsFeatureGeometryBoundary() {
    var feature = new Feature(nestedCollection(Geometry.MAX_COLLECTION_DEPTH), List.of());

    try (var arena = Arena.ofConfined()) {
      assertThrows(IllegalArgumentException.class, () -> ValueStructs.feature(feature, arena));
    }
  }

  @Test
  void geoJsonFeatureCollectionDepthCountsFeatureBoundary() {
    var feature = new Feature(nestedCollection(Geometry.MAX_COLLECTION_DEPTH - 1), List.of());
    var geoJson = GeoJson.featureCollection(List.of(feature));

    try (var arena = Arena.ofConfined()) {
      assertThrows(IllegalArgumentException.class, () -> ValueStructs.geoJson(geoJson, arena));
    }
  }

  @Test
  void geoJsonFeatureCollectionDepthCountsPropertyBoundary() {
    var feature =
        new Feature(
            Geometry.empty(),
            List.of(new JsonValue.Member("deep", nestedArray(JsonValue.MAX_DESCRIPTOR_DEPTH - 1))));
    var geoJson = GeoJson.featureCollection(List.of(feature));

    try (var arena = Arena.ofConfined()) {
      assertThrows(IllegalArgumentException.class, () -> ValueStructs.geoJson(geoJson, arena));
    }
  }

  private static JsonValue nestedArray(int arrayCount) {
    JsonValue json = JsonValue.nullValue();
    for (var index = 0; index < arrayCount; index++) {
      json = JsonValue.array(List.of(json));
    }
    return json;
  }

  private static Geometry nestedCollection(int collectionCount) {
    Geometry geometry = Geometry.empty();
    for (var index = 0; index < collectionCount; index++) {
      geometry = Geometry.collection(List.of(geometry));
    }
    return geometry;
  }
}
