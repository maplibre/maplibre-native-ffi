package org.maplibre.nativejni.internal.struct;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.util.List;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.geo.FeatureIdentifier;
import org.maplibre.nativejni.geo.GeoJson;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.json.JsonValue;

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

    var copied = ValueStructs.jsonValue(ValueStructs.jsonValue(value));

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

    var copiedGeometry = ValueStructs.geometry(ValueStructs.geometry(collection));
    var copiedFeature = ValueStructs.feature(ValueStructs.feature(feature));
    var copiedGeoJson = ValueStructs.geoJson(ValueStructs.geoJson(geoJson));

    assertEquals(collection, copiedGeometry);
    assertEquals(feature, copiedFeature);
    assertEquals(geoJson, copiedGeoJson);
  }

  @Test
  void descriptorDepthErrorsAreReportedInJava() {
    var tooDeepJson = nestedArray(JsonValue.MAX_DESCRIPTOR_DEPTH + 2);
    assertThrows(IllegalArgumentException.class, () -> ValueStructs.jsonValue(tooDeepJson));

    var tooDeepGeometry = nestedCollection(Geometry.MAX_COLLECTION_DEPTH + 2);
    assertThrows(IllegalArgumentException.class, () -> ValueStructs.geometry(tooDeepGeometry));
  }

  @Test
  void featureDepthCountsFeatureGeometryBoundary() {
    var feature = new Feature(nestedCollection(Geometry.MAX_COLLECTION_DEPTH), List.of());

    assertThrows(IllegalArgumentException.class, () -> ValueStructs.feature(feature));
  }

  @Test
  void geoJsonFeatureCollectionDepthCountsFeatureBoundary() {
    var feature = new Feature(nestedCollection(Geometry.MAX_COLLECTION_DEPTH - 1), List.of());
    var geoJson = GeoJson.featureCollection(List.of(feature));

    assertThrows(IllegalArgumentException.class, () -> ValueStructs.geoJson(geoJson));
  }

  @Test
  void geoJsonFeatureCollectionDepthCountsPropertyBoundary() {
    var feature =
        new Feature(
            Geometry.empty(),
            List.of(new JsonValue.Member("deep", nestedArray(JsonValue.MAX_DESCRIPTOR_DEPTH - 1))));
    var geoJson = GeoJson.featureCollection(List.of(feature));

    assertThrows(IllegalArgumentException.class, () -> ValueStructs.geoJson(geoJson));
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
