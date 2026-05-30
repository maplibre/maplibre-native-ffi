package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_IDENTIFIER_TYPE_STRING
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_IDENTIFIER_TYPE_UINT
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_TYPE_FEATURE
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_EMPTY
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_LINE_STRING
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_OBJECT
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_UINT
import org.maplibre.nativeffi.internal.c.mln_feature
import org.maplibre.nativeffi.internal.c.mln_geometry
import org.maplibre.nativeffi.internal.c.mln_json_value
import org.maplibre.nativeffi.json.JsonValue

@OptIn(ExperimentalForeignApi::class)
class ValueStructsTest {
  @Test
  fun jsonMaterializerPreservesObjectOrderAndDuplicateKeys() {
    val json =
      JsonValue.`object`(
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
  fun jsonMaterializerPreservesUnsignedBitPatterns() {
    memScoped {
      val native = ValueStructs.jsonValue(JsonValue.UInt(-1L), this).pointed

      assertEquals(MLN_JSON_VALUE_TYPE_UINT, native.type)
      assertEquals(ULong.MAX_VALUE, native.data.uint_value)
    }
  }

  @Test
  fun jsonSnapshotPreservesUnsignedBitPatterns() {
    memScoped {
      val native = alloc<mln_json_value>()
      native.type = MLN_JSON_VALUE_TYPE_UINT
      native.data.uint_value = ULong.MAX_VALUE

      assertEquals(JsonValue.UInt(-1L), ValueStructs.jsonSnapshot(native.ptr))
    }
  }

  @Test
  fun featureMaterializerPreservesUnsignedIdentifierBitPatterns() {
    val feature = Feature(Geometry.Empty, emptyList(), FeatureIdentifier.UInt(-1L))

    memScoped {
      val native = ValueStructs.feature(feature, this).pointed

      assertEquals(MLN_FEATURE_IDENTIFIER_TYPE_UINT, native.identifier_type)
      assertEquals(ULong.MAX_VALUE, native.identifier.uint_value)
    }
  }

  @Test
  fun featureSnapshotPreservesUnsignedIdentifierBitPatterns() {
    memScoped {
      val geometry = alloc<mln_geometry>()
      geometry.type = MLN_GEOMETRY_TYPE_EMPTY
      val feature = alloc<mln_feature>()
      feature.geometry = geometry.ptr
      feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_UINT
      feature.identifier.uint_value = ULong.MAX_VALUE

      assertEquals(
        FeatureIdentifier.UInt(-1L),
        ValueStructs.featureSnapshot(feature.ptr).identifier,
      )
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
