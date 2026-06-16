package org.maplibre.nativejni.internal.struct;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;

import java.util.List;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.geo.Feature;
import org.maplibre.nativejni.geo.FeatureIdentifier;
import org.maplibre.nativejni.geo.GeoJson;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.internal.javacpp.JavaCppValues;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.json.JsonValue;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.map.MapOptions;
import org.maplibre.nativejni.runtime.RuntimeHandle;

// Support invariant for BND-065, BND-066, BND-067, and BND-068: these tests use
// JavaCPP structs and fault injection to cover native materialization paths not reliably forced by
// public APIs.
final class StyleStructsTest {
  @Test
  void bnd065AndBnd067GeoJsonFeatureMaterializesPropertiesAndUnsignedIdentifier() {
    var properties =
        List.of(
            new JsonValue.Member("name", JsonValue.of("first")),
            new JsonValue.Member("name", JsonValue.unsigned(-1L)),
            new JsonValue.Member("signed", JsonValue.of(-2L)));
    var geoJson =
        GeoJson.feature(
            new Feature(
                Geometry.point(new LatLng(1, 2)), properties, FeatureIdentifier.unsigned(-1L)));

    try (var scope = StyleStructs.geoJson(geoJson)) {
      var feature = scope.value().data_feature();
      assertEquals(MaplibreNativeC.MLN_FEATURE_IDENTIFIER_TYPE_UINT, feature.identifier_type());
      assertEquals(-1L, feature.identifier_uint_value());
      assertEquals(properties.size(), feature.property_count());

      var first = feature.properties().getPointer(0);
      assertEquals("name", JavaCppValues.string(first.key()));
      assertEquals(JsonValue.of("first"), JavaCppValues.jsonValue(first.value()));

      var duplicate = feature.properties().getPointer(1);
      assertEquals("name", JavaCppValues.string(duplicate.key()));
      assertEquals(JsonValue.unsigned(-1L), JavaCppValues.jsonValue(duplicate.value()));

      var signed = feature.properties().getPointer(2);
      assertEquals("signed", JavaCppValues.string(signed.key()));
      assertEquals(JsonValue.of(-2L), JavaCppValues.jsonValue(signed.value()));
    }
  }

  @Test
  void bnd062AndBnd068UnknownFeatureIdentifierTypePreservesRawTagForOutputOnly() {
    try (var feature = new MaplibreNativeC.mln_feature()) {
      feature.size(feature.sizeof());
      feature.identifier_type(1234);

      assertEquals(new FeatureIdentifier.Unknown(1234), QueryStructs.featureIdentifier(feature));
    }

    var geoJson =
        GeoJson.feature(
            new Feature(
                Geometry.point(new LatLng(1, 2)), List.of(), new FeatureIdentifier.Unknown(1234)));

    assertThrows(IllegalArgumentException.class, () -> StyleStructs.geoJson(geoJson));
  }

  @Test
  void bnd066StyleIdListCopyFailureDestroysNativeList() {
    try (var runtime = RuntimeHandle.create()) {
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        StyleStructs.resetStyleIdListDestroyCountForTesting();
        var failure = new IllegalStateException("injected style ID list copy failure");

        StyleStructs.failNextStyleIdListCopyForTesting(failure);
        assertSame(failure, assertThrows(IllegalStateException.class, map::styleSourceIds));

        assertEquals(1, StyleStructs.styleIdListDestroyCountForTesting());
      }
    }
  }
}
