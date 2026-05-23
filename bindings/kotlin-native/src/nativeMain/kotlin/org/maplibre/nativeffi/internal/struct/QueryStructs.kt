package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.get
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT
import org.maplibre.nativeffi.internal.c.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
import org.maplibre.nativeffi.internal.c.mln_feature_state_selector
import org.maplibre.nativeffi.internal.c.mln_rendered_feature_query_options
import org.maplibre.nativeffi.internal.c.mln_rendered_feature_query_options_default
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry
import org.maplibre.nativeffi.internal.c.mln_source_feature_query_options
import org.maplibre.nativeffi.internal.c.mln_source_feature_query_options_default
import org.maplibre.nativeffi.internal.c.mln_string_view
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Materializes feature query descriptors at the C boundary. */
@OptIn(ExperimentalForeignApi::class)
internal object QueryStructs {
  fun renderedQueryGeometry(
    value: RenderedQueryGeometry,
    scope: MemScope,
  ): CPointer<mln_rendered_query_geometry> {
    val native = scope.alloc<mln_rendered_query_geometry>()
    native.size = sizeOf<mln_rendered_query_geometry>().toUInt()
    when (value) {
      is RenderedQueryGeometry.Point -> {
        native.type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT
        native.data.point.x = value.point.x
        native.data.point.y = value.point.y
      }
      is RenderedQueryGeometry.Box -> {
        native.type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX
        setScreenBox(native.data.box, value.box)
      }
      is RenderedQueryGeometry.LineString -> {
        native.type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING
        native.data.line_string.points = CoreStructs.screenPointArray(value.points, scope)
        native.data.line_string.point_count = value.points.size.toULong()
      }
    }
    return native.ptr
  }

  fun renderedFeatureQueryOptions(
    value: RenderedFeatureQueryOptions?,
    scope: MemScope,
  ): CPointer<mln_rendered_feature_query_options>? {
    if (value == null) return null
    val native = scope.alloc<mln_rendered_feature_query_options>()
    mln_rendered_feature_query_options_default().place(native.ptr)
    value.layerIds?.let { layerIds ->
      native.fields = native.fields or MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
      native.layer_ids = stringViewArray(layerIds, scope)
      native.layer_id_count = layerIds.size.toULong()
    }
    value.filter?.let { filter -> native.filter = ValueStructs.jsonValue(filter, scope) }
    return native.ptr
  }

  fun sourceFeatureQueryOptions(
    value: SourceFeatureQueryOptions?,
    scope: MemScope,
  ): CPointer<mln_source_feature_query_options>? {
    if (value == null) return null
    val native = scope.alloc<mln_source_feature_query_options>()
    mln_source_feature_query_options_default().place(native.ptr)
    value.sourceLayerIds?.let { sourceLayerIds ->
      native.fields = native.fields or MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
      native.source_layer_ids = stringViewArray(sourceLayerIds, scope)
      native.source_layer_id_count = sourceLayerIds.size.toULong()
    }
    value.filter?.let { filter -> native.filter = ValueStructs.jsonValue(filter, scope) }
    return native.ptr
  }

  fun featureStateSelector(
    value: FeatureStateSelector,
    scope: MemScope,
  ): CPointer<mln_feature_state_selector> {
    val native = scope.alloc<mln_feature_state_selector>()
    native.size = sizeOf<mln_feature_state_selector>().toUInt()
    native.fields = 0U
    CoreStructs.setStringView(native.source_id, value.sourceId, scope)
    CoreStructs.setStringView(native.source_layer_id, "", scope)
    CoreStructs.setStringView(native.feature_id, "", scope)
    CoreStructs.setStringView(native.state_key, "", scope)
    value.sourceLayerId?.let { sourceLayerId ->
      native.fields = native.fields or MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
      CoreStructs.setStringView(native.source_layer_id, sourceLayerId, scope)
    }
    value.featureId?.let { featureId ->
      native.fields = native.fields or MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
      CoreStructs.setStringView(native.feature_id, featureId, scope)
    }
    value.stateKey?.let { stateKey ->
      native.fields = native.fields or MLN_FEATURE_STATE_SELECTOR_STATE_KEY
      CoreStructs.setStringView(native.state_key, stateKey, scope)
    }
    return native.ptr
  }

  private fun setScreenBox(
    native: org.maplibre.nativeffi.internal.c.mln_screen_box,
    value: ScreenBox,
  ) {
    native.min.x = value.min.x
    native.min.y = value.min.y
    native.max.x = value.max.x
    native.max.y = value.max.y
  }

  private fun stringViewArray(values: List<String>, scope: MemScope): CPointer<mln_string_view>? {
    if (values.isEmpty()) return null
    val array = scope.allocArray<mln_string_view>(values.size)
    values.forEachIndexed { index, value -> CoreStructs.setStringView(array[index], value, scope) }
    return array
  }
}
