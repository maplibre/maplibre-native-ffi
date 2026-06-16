package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.set
import kotlinx.cinterop.sizeOf
import org.maplibre.nativeffi.error.MaplibreStatus
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
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_STRING
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_UINT
import org.maplibre.nativeffi.internal.c.mln_feature
import org.maplibre.nativeffi.internal.c.mln_geometry
import org.maplibre.nativeffi.internal.c.mln_json_member
import org.maplibre.nativeffi.internal.c.mln_json_value
import org.maplibre.nativeffi.json.JsonValue

@OptIn(ExperimentalForeignApi::class)
class ValueStructsTest {
  // BND-064, BND-065, BND-066, BND-067: value snapshots, GeoJSON, handle cleanup, and JSON shape.

  @Test
  fun jsonMaterializerPreservesObjectOrderAndDuplicateKeys() {
    val json =
      JsonValue.ObjectValue(
        listOf(
          JsonValue.Member("name", JsonValue.StringValue("first")),
          JsonValue.Member("name", JsonValue.StringValue("second")),
        )
      )

    memScoped {
      val native = ValueStructs.jsonValue(json, this).pointed

      assertEquals(MLN_JSON_VALUE_TYPE_OBJECT, native.type)
      assertEquals(2UL, native.data.object_value.member_count)
      assertEquals("name", CoreStructs.stringView(native.data.object_value.members!![0].key))
      assertEquals("name", CoreStructs.stringView(native.data.object_value.members!![1].key))
      assertEquals(
        JsonValue.StringValue("first"),
        ValueStructs.jsonSnapshot(native.data.object_value.members!![0].value),
      )
      assertEquals(
        JsonValue.StringValue("second"),
        ValueStructs.jsonSnapshot(native.data.object_value.members!![1].value),
      )
    }
  }

  @Test
  fun jsonRoundTripPreservesNestedContainersAndIntegerWidths() {
    val json =
      JsonValue.ObjectValue(
        listOf(
          JsonValue.Member("value", JsonValue.Int(-1L)),
          JsonValue.Member("value", JsonValue.UInt(-1L)),
          JsonValue.Member(
            "nested",
            JsonValue.Array(
              listOf(JsonValue.Bool(true), JsonValue.DoubleValue(1.5), JsonValue.Null)
            ),
          ),
        )
      )

    memScoped {
      val native = ValueStructs.jsonValue(json, this)

      assertEquals(json, ValueStructs.jsonSnapshot(native))
    }
  }

  @Test
  fun jsonStringViewsPreserveEmbeddedNulBytes() {
    val key = "a\u0000key"
    val value = "b\u0000value"
    val json = JsonValue.ObjectValue(listOf(JsonValue.Member(key, JsonValue.StringValue(value))))

    memScoped {
      val native = ValueStructs.jsonValue(json, this).pointed
      val member = native.data.object_value.members!![0]

      assertEquals(key, CoreStructs.stringView(member.key))
      assertEquals(JsonValue.StringValue(value), ValueStructs.jsonSnapshot(member.value))
      assertEquals(json, ValueStructs.jsonSnapshot(native.ptr))
    }
  }

  @Test
  fun geometryMaterializerCopiesCoordinateSpans() {
    val geometry = Geometry.LineString(listOf(LatLng(1.0, 2.0), LatLng(3.0, 4.0)))

    memScoped {
      val native = ValueStructs.geometry(geometry, this).pointed

      assertEquals(MLN_GEOMETRY_TYPE_LINE_STRING, native.type)
      assertEquals(2UL, native.data.line_string.coordinate_count)
      assertEquals(1.0, native.data.line_string.coordinates!![0].latitude)
      assertEquals(4.0, native.data.line_string.coordinates!![1].longitude)
    }
  }

  @Test
  fun geometrySnapshotPreservesUnknownTaggedValueDiscriminator() {
    memScoped {
      val native = alloc<mln_geometry>()
      native.size = sizeOf<mln_geometry>().toUInt()
      native.type = 999U

      assertEquals(
        Geometry.Unknown(999, sizeOf<mln_geometry>().toInt()),
        ValueStructs.geometrySnapshot(native.ptr),
      )
    }
  }

  @Test
  fun unknownGeometryIsOutputOnly() {
    memScoped {
      assertFailsWith<IllegalArgumentException> {
        ValueStructs.geometry(Geometry.Unknown(999, 8), this)
      }
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
  fun jsonSnapshotPreservesUnknownTaggedValueDiscriminator() {
    memScoped {
      val native = alloc<mln_json_value>()
      native.size = sizeOf<mln_json_value>().toUInt()
      native.type = 999U

      assertEquals(
        JsonValue.Unknown(999, sizeOf<mln_json_value>().toInt()),
        ValueStructs.jsonSnapshot(native.ptr),
      )
    }
  }

  @Test
  fun jsonSnapshotHandleCopiesValueAndDestroysNativeHandle() {
    memScoped {
      var destroys = 0
      val snapshot = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_json_snapshot>()
      val native = alloc<mln_json_value>()
      native.type = org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_NULL

      val value =
        ValueStructs.jsonSnapshotHandle(
          snapshot,
          getter = { _, outValue ->
            outValue[0] = native.ptr
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )

      assertEquals(JsonValue.Null, value)
      assertEquals(1, destroys)
    }
  }

  @Test
  fun unknownJsonValueIsOutputOnly() {
    memScoped {
      assertFailsWith<IllegalArgumentException> {
        ValueStructs.jsonValue(JsonValue.Unknown(999, 8), this)
      }
    }
  }

  @Test
  fun jsonSnapshotHandleDestroysNativeHandleWhenCopyFails() {
    memScoped {
      var destroys = 0
      val snapshot = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_json_snapshot>()
      val native = alloc<mln_json_value>()
      native.type = MLN_JSON_VALUE_TYPE_OBJECT
      native.data.object_value.members = null
      native.data.object_value.member_count = Int.MAX_VALUE.toULong() + 1UL

      assertFailsWith<IllegalArgumentException> {
        ValueStructs.jsonSnapshotHandle(
          snapshot,
          getter = { _, outValue ->
            outValue[0] = native.ptr
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )
      }

      assertEquals(1, destroys)
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
  fun featureSnapshotCopiesPropertiesAndStringIdentifier() {
    val copied = memScoped {
      val geometry = alloc<mln_geometry>()
      geometry.size = sizeOf<mln_geometry>().toUInt()
      geometry.type = MLN_GEOMETRY_TYPE_EMPTY
      val propertyValue = alloc<mln_json_value>()
      propertyValue.size = sizeOf<mln_json_value>().toUInt()
      propertyValue.type = MLN_JSON_VALUE_TYPE_STRING
      CoreStructs.setStringView(propertyValue.data.string_value, "capital", this)
      val property = alloc<mln_json_member>()
      CoreStructs.setStringView(property.key, "kind", this)
      property.value = propertyValue.ptr
      val feature = alloc<mln_feature>()
      feature.size = sizeOf<mln_feature>().toUInt()
      feature.geometry = geometry.ptr
      feature.properties = property.ptr
      feature.property_count = 1UL
      feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_STRING
      CoreStructs.setStringView(feature.identifier.string_value, "id-1", this)

      ValueStructs.featureSnapshot(feature.ptr)
    }

    assertEquals(
      Feature(
        Geometry.Empty,
        listOf(JsonValue.Member("kind", JsonValue.StringValue("capital"))),
        FeatureIdentifier.StringValue("id-1"),
      ),
      copied,
    )
  }

  @Test
  fun featureSnapshotPreservesUnknownIdentifierType() {
    memScoped {
      val geometry = alloc<mln_geometry>()
      geometry.type = MLN_GEOMETRY_TYPE_EMPTY
      val feature = alloc<mln_feature>()
      feature.geometry = geometry.ptr
      feature.identifier_type = 999U

      assertEquals(
        FeatureIdentifier.Unknown(999),
        ValueStructs.featureSnapshot(feature.ptr).identifier,
      )
    }
  }

  @Test
  fun unknownFeatureIdentifierIsOutputOnly() {
    memScoped {
      assertFailsWith<IllegalArgumentException> {
        ValueStructs.feature(
          Feature(Geometry.Empty, emptyList(), FeatureIdentifier.Unknown(999)),
          this,
        )
      }
    }
  }

  @Test
  fun geoJsonMaterializerWritesFeatureDescriptors() {
    val feature =
      Feature(
        Geometry.Point(LatLng(1.0, 2.0)),
        listOf(JsonValue.Member("visible", JsonValue.Bool(true))),
        FeatureIdentifier.StringValue("id-1"),
      )

    memScoped {
      val native = ValueStructs.geoJson(GeoJson.FeatureValue(feature), this).pointed

      assertEquals(MLN_GEOJSON_TYPE_FEATURE, native.type)
      assertEquals(1UL, native.data.feature!!.pointed.property_count)
      assertEquals(
        MLN_FEATURE_IDENTIFIER_TYPE_STRING,
        native.data.feature!!.pointed.identifier_type,
      )
    }
  }
}
