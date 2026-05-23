package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_IDENTIFIER_TYPE_STRING
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_TYPE_FEATURE
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_LINE_STRING
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_OBJECT
import org.maplibre.nativeffi.json.JsonValue

@OptIn(ExperimentalForeignApi::class)
class ValueStructsTest {
  @Test
  fun jsonMaterializerPreservesObjectOrderAndDuplicateKeys() {
    val json =
      JsonValue.obj(
        listOf(
          JsonValue.Member("name", JsonValue.of("first")),
          JsonValue.Member("name", JsonValue.of("second")),
        )
      )

    memScoped {
      val native = ValueStructs.jsonValue(json, this).pointed

      assertEquals(MLN_JSON_VALUE_TYPE_OBJECT, native.type)
      assertEquals(2UL, native.data.object_value.member_count)
    }
  }

  @Test
  fun geometryMaterializerCopiesCoordinateSpans() {
    val geometry = Geometry.lineString(listOf(LatLng(1.0, 2.0), LatLng(3.0, 4.0)))

    memScoped {
      val native = ValueStructs.geometry(geometry, this).pointed

      assertEquals(MLN_GEOMETRY_TYPE_LINE_STRING, native.type)
      assertEquals(2UL, native.data.line_string.coordinate_count)
      assertEquals(1.0, native.data.line_string.coordinates!![0].latitude)
      assertEquals(4.0, native.data.line_string.coordinates!![1].longitude)
    }
  }

  @Test
  fun geoJsonMaterializerWritesFeatureDescriptors() {
    val feature =
      Feature(
        Geometry.point(LatLng(1.0, 2.0)),
        listOf(JsonValue.Member("visible", JsonValue.of(true))),
        FeatureIdentifier.of("id-1"),
      )

    memScoped {
      val native = ValueStructs.geoJson(GeoJson.feature(feature), this).pointed

      assertEquals(MLN_GEOJSON_TYPE_FEATURE, native.type)
      assertEquals(1UL, native.data.feature!!.pointed.property_count)
      assertEquals(
        MLN_FEATURE_IDENTIFIER_TYPE_STRING,
        native.data.feature!!.pointed.identifier_type,
      )
    }
  }
}
