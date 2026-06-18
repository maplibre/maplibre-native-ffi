package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
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
import org.maplibre.nativeffi.geo.FeatureIdentifier
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_IDENTIFIER_TYPE_STRING
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
import org.maplibre.nativeffi.internal.c.MLN_GEOMETRY_TYPE_EMPTY
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_BOOL
import org.maplibre.nativeffi.internal.c.MLN_JSON_VALUE_TYPE_STRING
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_SOURCE_ID
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_STATE
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING
import org.maplibre.nativeffi.internal.c.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
import org.maplibre.nativeffi.internal.c.mln_feature
import org.maplibre.nativeffi.internal.c.mln_geometry
import org.maplibre.nativeffi.internal.c.mln_json_member
import org.maplibre.nativeffi.internal.c.mln_json_value
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

@OptIn(ExperimentalForeignApi::class)
class QueryStructsTest {
  // BND-060, BND-061, BND-066, BND-106: query options and copied query results.

  @Test
  fun featureQueryResultCopiesFeaturePropertiesSourceLayerStateAndUnknownIds() {
    var destroys = 0
    val features = memScoped {
      val result = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_feature_query_result>()
      val geometry = alloc<mln_geometry>()
      geometry.size = sizeOf<mln_geometry>().toUInt()
      geometry.type = MLN_GEOMETRY_TYPE_EMPTY
      val propertyValue = alloc<mln_json_value>()
      propertyValue.size = sizeOf<mln_json_value>().toUInt()
      propertyValue.type = MLN_JSON_VALUE_TYPE_STRING
      CoreStructs.setStringView(propertyValue.data.string_value, "park", this)
      val property = alloc<mln_json_member>()
      CoreStructs.setStringView(property.key, "kind", this)
      property.value = propertyValue.ptr
      val state = alloc<mln_json_value>()
      state.size = sizeOf<mln_json_value>().toUInt()
      state.type = MLN_JSON_VALUE_TYPE_BOOL
      state.data.bool_value = true

      QueryStructs.featureQueryResult(
        result,
        counter = { _, outCount ->
          outCount[0] = 2UL
          MaplibreStatus.OK.nativeCode
        },
        getter = { _, index, outFeature ->
          val queried = outFeature.pointed
          queried.feature.size = sizeOf<mln_feature>().toUInt()
          queried.feature.geometry = geometry.ptr
          if (index == 0UL) {
            queried.fields =
              MLN_QUERIED_FEATURE_SOURCE_ID or
                MLN_QUERIED_FEATURE_SOURCE_LAYER_ID or
                MLN_QUERIED_FEATURE_STATE
            queried.feature.properties = property.ptr
            queried.feature.property_count = 1UL
            queried.feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_STRING
            CoreStructs.setStringView(queried.feature.identifier.string_value, "id-1", this)
            CoreStructs.setStringView(queried.source_id, "source", this)
            CoreStructs.setStringView(queried.source_layer_id, "layer", this)
            queried.state = state.ptr
          } else {
            queried.fields = 0U
            queried.feature.property_count = 0UL
            queried.feature.identifier_type = 999U
          }
          MaplibreStatus.OK.nativeCode
        },
        destroyer = { destroys++ },
      )
    }

    assertEquals(1, destroys)
    assertEquals(2, features.size)
    assertEquals(Geometry.Empty, features[0].feature.geometry)
    assertEquals(
      listOf(JsonValue.Member("kind", JsonValue.StringValue("park"))),
      features[0].feature.properties,
    )
    assertEquals(FeatureIdentifier.StringValue("id-1"), features[0].feature.identifier)
    assertEquals("source", features[0].sourceId)
    assertEquals("layer", features[0].sourceLayerId)
    assertEquals(JsonValue.Bool(true), features[0].state)
    assertEquals(FeatureIdentifier.Unknown(999), features[1].feature.identifier)
  }

  @Test
  fun featureQueryResultDestroysNativeHandleWhenCopyFails() {
    memScoped {
      var destroys = 0
      val result = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_feature_query_result>()
      val geometry = alloc<mln_geometry>()
      geometry.size = sizeOf<mln_geometry>().toUInt()
      geometry.type = MLN_GEOMETRY_TYPE_EMPTY

      assertFailsWith<IllegalArgumentException> {
        QueryStructs.featureQueryResult(
          result,
          counter = { _, outCount ->
            outCount[0] = 1UL
            MaplibreStatus.OK.nativeCode
          },
          getter = { _, _, outFeature ->
            outFeature.pointed.feature.size = sizeOf<mln_feature>().toUInt()
            outFeature.pointed.feature.geometry = geometry.ptr
            outFeature.pointed.feature.property_count = Int.MAX_VALUE.toULong() + 1UL
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )
      }

      assertEquals(1, destroys)
    }
  }

  @Test
  fun featureExtensionResultCopiesValueAndDestroysHandle() {
    memScoped {
      var destroys = 0
      val result = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_feature_extension_result>()
      val value = alloc<mln_json_value>()
      value.size = sizeOf<mln_json_value>().toUInt()
      value.type = MLN_JSON_VALUE_TYPE_BOOL
      value.data.bool_value = true

      val extension =
        QueryStructs.featureExtensionResult(
          result,
          getter = { _, outInfo ->
            outInfo.pointed.type = MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE
            outInfo.pointed.data.value = value.ptr
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )

      assertEquals(FeatureExtensionResult.Value(JsonValue.Bool(true)), extension)
      assertEquals(1, destroys)
    }
  }

  @Test
  fun featureExtensionResultCopiesFeatureCollectionAndDestroysHandle() {
    memScoped {
      var destroys = 0
      val result = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_feature_extension_result>()
      val feature = alloc<mln_feature>()
      val geometry = alloc<mln_geometry>()
      geometry.size = sizeOf<mln_geometry>().toUInt()
      geometry.type = MLN_GEOMETRY_TYPE_EMPTY
      feature.size = sizeOf<mln_feature>().toUInt()
      feature.geometry = geometry.ptr
      feature.property_count = 0UL
      feature.identifier_type = MLN_FEATURE_IDENTIFIER_TYPE_STRING
      CoreStructs.setStringView(feature.identifier.string_value, "cluster-child", this)

      val extension =
        QueryStructs.featureExtensionResult(
          result,
          getter = { _, outInfo ->
            outInfo.pointed.type = MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION
            outInfo.pointed.data.feature_collection.features = feature.ptr
            outInfo.pointed.data.feature_collection.feature_count = 1UL
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )

      val collection = extension as FeatureExtensionResult.FeatureCollection
      assertEquals(1, collection.features.size)
      assertEquals(
        FeatureIdentifier.StringValue("cluster-child"),
        collection.features.single().identifier,
      )
      assertEquals(Geometry.Empty, collection.features.single().geometry)
      assertEquals(1, destroys)
    }
  }

  @Test
  fun featureExtensionResultCopiesUnknownTypeAndDestroysHandle() {
    memScoped {
      var destroys = 0
      val result = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_feature_extension_result>()

      val value =
        QueryStructs.featureExtensionResult(
          result,
          getter = { _, outInfo ->
            outInfo.pointed.type = 999U
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )

      assertEquals(FeatureExtensionResult.Unknown(999), value)
      assertEquals(1, destroys)
    }
  }

  @Test
  fun featureExtensionResultDestroysNativeHandleWhenCopyFails() {
    memScoped {
      var destroys = 0
      val result = alloc<ByteVar>().ptr.reinterpret<cnames.structs.mln_feature_extension_result>()

      assertFailsWith<IllegalArgumentException> {
        QueryStructs.featureExtensionResult(
          result,
          getter = { _, outInfo ->
            outInfo.pointed.type = MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION
            outInfo.pointed.data.feature_collection.feature_count = Int.MAX_VALUE.toULong() + 1UL
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )
      }

      assertEquals(1, destroys)
    }
  }

  @Test
  fun renderedQueryGeometryMaterializesBoxesAndLineStrings() {
    memScoped {
      val box =
        QueryStructs.renderedQueryGeometry(
            RenderedQueryGeometry.Box(ScreenBox(ScreenPoint(1.0, 2.0), ScreenPoint(3.0, 4.0))),
            this,
          )
          .pointed
      assertEquals(MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX, box.type)
      assertEquals(1.0, box.data.box.min.x)
      assertEquals(4.0, box.data.box.max.y)

      val line =
        QueryStructs.renderedQueryGeometry(
            RenderedQueryGeometry.LineString(listOf(ScreenPoint(5.0, 6.0), ScreenPoint(7.0, 8.0))),
            this,
          )
          .pointed
      assertEquals(MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING, line.type)
      assertEquals(2UL, line.data.line_string.point_count)
      assertEquals(7.0, line.data.line_string.points!![1].x)
    }
  }

  @Test
  fun queryOptionsTrackOptionalFieldsSeparatelyFromFilters() {
    memScoped {
      val rendered =
        QueryStructs.renderedFeatureQueryOptions(
            RenderedFeatureQueryOptions().apply {
              layerIds = listOf("roads", "labels")
              filter =
                JsonValue.Array(listOf(JsonValue.StringValue("has"), JsonValue.StringValue("name")))
            },
            this,
          )!!
          .pointed
      assertTrue((rendered.fields and MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0U)
      assertEquals(2UL, rendered.layer_id_count)
      assertNotNull(rendered.filter)

      val source =
        QueryStructs.sourceFeatureQueryOptions(
            SourceFeatureQueryOptions().apply { sourceLayerIds = listOf("water") },
            this,
          )!!
          .pointed
      assertTrue((source.fields and MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0U)
      assertEquals(1UL, source.source_layer_id_count)
    }
  }

  @Test
  fun featureStateSelectorKeepsStateKeyDependentOnFeatureId() {
    assertFailsWith<IllegalStateException> { FeatureStateSelector("source").stateKey = "hover" }

    val selector =
      FeatureStateSelector("source").apply {
        sourceLayerId = "layer"
        featureId = "feature-1"
        stateKey = "hover"
      }
    memScoped {
      val native = QueryStructs.featureStateSelector(selector, this).pointed
      assertTrue((native.fields and MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID) != 0U)
      assertTrue((native.fields and MLN_FEATURE_STATE_SELECTOR_FEATURE_ID) != 0U)
      assertTrue((native.fields and MLN_FEATURE_STATE_SELECTOR_STATE_KEY) != 0U)
    }

    selector.featureId = null
    assertFalse(selector.featureId != null)
    assertFalse(selector.stateKey != null)
  }
}
