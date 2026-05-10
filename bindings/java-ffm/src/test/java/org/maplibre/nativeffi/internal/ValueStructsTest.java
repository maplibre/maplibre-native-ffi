package org.maplibre.nativeffi.internal;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.lang.foreign.Arena;
import java.math.BigInteger;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.Feature;
import org.maplibre.nativeffi.FeatureIdentifier;
import org.maplibre.nativeffi.GeoJson;
import org.maplibre.nativeffi.Geometry;
import org.maplibre.nativeffi.JsonValue;
import org.maplibre.nativeffi.LatLng;

final class ValueStructsTest {
  @Test
  void jsonValuesMaterializeAndCopyBack() {
    var value =
        JsonValue.object(
            List.of(
                new JsonValue.Member("null", JsonValue.nullValue()),
                new JsonValue.Member("bool", JsonValue.of(true)),
                new JsonValue.Member(
                    "uint", JsonValue.unsigned(new BigInteger("18446744073709551615"))),
                new JsonValue.Member("int", JsonValue.of(-7L)),
                new JsonValue.Member("double", JsonValue.of(1.25)),
                new JsonValue.Member("string", JsonValue.of("hello\u0000world")),
                new JsonValue.Member(
                    "array", JsonValue.array(List.of(JsonValue.of("a"), JsonValue.of("b"))))));

    JsonValue copied;
    try (var arena = Arena.ofConfined()) {
      copied = Structs.jsonValue(Structs.jsonValue(value, arena));
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
      copiedGeometry = Structs.geometry(Structs.geometry(collection, arena));
      copiedFeature = Structs.feature(Structs.feature(feature, arena));
      copiedGeoJson = Structs.geoJson(Structs.geoJson(geoJson, arena));
    }

    assertEquals(collection, copiedGeometry);
    assertEquals(feature, copiedFeature);
    assertEquals(geoJson, copiedGeoJson);
  }

  @Test
  void descriptorDepthErrorsAreReportedInJava() {
    JsonValue json = JsonValue.nullValue();
    for (var index = 0; index < JsonValue.MAX_DESCRIPTOR_DEPTH + 2; index++) {
      json = JsonValue.array(List.of(json));
    }
    var tooDeepJson = json;
    try (var arena = Arena.ofConfined()) {
      assertThrows(IllegalArgumentException.class, () -> Structs.jsonValue(tooDeepJson, arena));
    }

    Geometry geometry = Geometry.empty();
    for (var index = 0; index < Geometry.MAX_COLLECTION_DEPTH + 2; index++) {
      geometry = Geometry.collection(List.of(geometry));
    }
    var tooDeepGeometry = geometry;
    try (var arena = Arena.ofConfined()) {
      assertThrows(IllegalArgumentException.class, () -> Structs.geometry(tooDeepGeometry, arena));
    }
  }
}
