package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING
import org.maplibre.nativeffi.internal.c.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

@OptIn(ExperimentalForeignApi::class)
class QueryStructsTest {
  @Test
  fun renderedQueryGeometryMaterializesBoxesAndLineStrings() {
    memScoped {
      val box =
        QueryStructs.renderedQueryGeometry(
            RenderedQueryGeometry.box(ScreenBox(ScreenPoint(1.0, 2.0), ScreenPoint(3.0, 4.0))),
            this,
          )
          .pointed
      assertEquals(MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX, box.type)
      assertEquals(1.0, box.data.box.min.x)
      assertEquals(4.0, box.data.box.max.y)

      val line =
        QueryStructs.renderedQueryGeometry(
            RenderedQueryGeometry.lineString(listOf(ScreenPoint(5.0, 6.0), ScreenPoint(7.0, 8.0))),
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
              filter = JsonValue.array(listOf(JsonValue.of("has"), JsonValue.of("name")))
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
