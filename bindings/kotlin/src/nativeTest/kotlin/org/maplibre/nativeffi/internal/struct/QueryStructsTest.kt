package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertFalse
import kotlin.test.assertNotNull
import kotlin.test.assertNull
import kotlin.test.assertTrue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.set
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_SOURCE_ID
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_STATE
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING
import org.maplibre.nativeffi.internal.c.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
import org.maplibre.nativeffi.internal.lifecycle.SyntheticHandles
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.memory.toCSize
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

@OptIn(ExperimentalForeignApi::class)
class QueryStructsTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-060, BND-061, BND-066, BND-106.

  @Test
  fun renderedQueryGeometryMaterializesBoxesAndLineStrings(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
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
              RenderedQueryGeometry.LineString(
                listOf(ScreenPoint(5.0, 6.0), ScreenPoint(7.0, 8.0))
              ),
              this,
            )
            .pointed
        assertEquals(MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING, line.type)
        assertEquals(2UL, line.data.line_string.point_count.toULong())
        assertEquals(7.0, line.data.line_string.points!![1].x)
      }
    }

  @Test
  fun queryOptionsTrackOptionalFieldsSeparatelyFromFilters(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      memScoped {
        val rendered =
          QueryStructs.renderedFeatureQueryOptions(
              RenderedFeatureQueryOptions().apply {
                layerIds = listOf("roads", "labels")
                filter = "[\"has\",\"name\"]".encodeToByteArray()
              },
              this,
            )!!
            .pointed
        assertTrue((rendered.fields and MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS) != 0U)
        assertEquals(2UL, rendered.layer_id_count.toULong())
        assertNotNull(rendered.filter)

        val source =
          QueryStructs.sourceFeatureQueryOptions(
              SourceFeatureQueryOptions().apply { sourceLayerIds = listOf("water") },
              this,
            )!!
            .pointed
        assertTrue((source.fields and MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS) != 0U)
        assertEquals(1UL, source.source_layer_id_count.toULong())
      }
    }

  @Test
  fun featureStateSelectorKeepsStateKeyDependentOnFeatureId(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      assertFailsWith<InvalidArgumentException> {
        FeatureStateSelector("source").stateKey = "hover"
      }

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

  @Test
  fun queriedFeatureListCopiesHitsAndDestroysNativeHandle() {
    var destroys = 0
    val feature = """{"type":"Feature","id":"feature-1"}""".encodeToByteArray()
    val state = """{"hover":true}""".encodeToByteArray()
    val hits = memScoped {
      QueryStructs.queriedFeatureList(
        SyntheticHandles.queriedFeatureList().rawHandleValue,
        counter = { _, outCount ->
          outCount[0] = 1.toCSize()
          MaplibreStatus.OK.nativeCode
        },
        getter = { _, _, outFeature ->
          val native = outFeature.pointed
          ByteStructs.setBufferView(native.feature, feature, this)
          native.fields = MLN_QUERIED_FEATURE_SOURCE_ID or MLN_QUERIED_FEATURE_STATE
          CoreStructs.setStringView(native.source_id, "point", this)
          ByteStructs.setBufferView(native.state, state, this)
          MaplibreStatus.OK.nativeCode
        },
        destroyer = { destroys++ },
      )
    }

    assertEquals(1, hits.size)
    assertContentEquals(feature, hits[0].feature)
    assertEquals("point", hits[0].sourceId)
    assertNull(hits[0].sourceLayerId)
    assertContentEquals(state, hits[0].state)
    assertEquals(1, destroys)
  }

  @Test
  fun queriedFeatureListDestroysNativeHandleWhenCopyFails() {
    memScoped {
      var destroys = 0
      assertFailsWith<IllegalArgumentException> {
        QueryStructs.queriedFeatureList(
          SyntheticHandles.queriedFeatureList().rawHandleValue,
          counter = { _, outCount ->
            outCount[0] = (Int.MAX_VALUE.toULong() + 1UL).toCSize()
            MaplibreStatus.OK.nativeCode
          },
          getter = { _, _, _ -> MaplibreStatus.OK.nativeCode },
          destroyer = { destroys++ },
        )
      }
      assertEquals(1, destroys)
    }
  }
}
