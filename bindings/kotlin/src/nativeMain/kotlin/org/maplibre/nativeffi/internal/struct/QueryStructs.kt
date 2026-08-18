package org.maplibre.nativeffi.internal.struct

import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CValue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.cValue
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.value
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_SOURCE_ID
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_STATE
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
import org.maplibre.nativeffi.internal.c.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
import org.maplibre.nativeffi.internal.c.mln_buffer_view
import org.maplibre.nativeffi.internal.c.mln_feature_state_selector
import org.maplibre.nativeffi.internal.c.mln_queried_feature
import org.maplibre.nativeffi.internal.c.mln_queried_feature_default
import org.maplibre.nativeffi.internal.c.mln_queried_feature_list_count
import org.maplibre.nativeffi.internal.c.mln_queried_feature_list_destroy
import org.maplibre.nativeffi.internal.c.mln_queried_feature_list_get
import org.maplibre.nativeffi.internal.c.mln_rendered_feature_query_options
import org.maplibre.nativeffi.internal.c.mln_rendered_feature_query_options_default
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry_box
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry_line_string
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry_point
import org.maplibre.nativeffi.internal.c.mln_screen_box
import org.maplibre.nativeffi.internal.c.mln_source_feature_query_options
import org.maplibre.nativeffi.internal.c.mln_source_feature_query_options_default
import org.maplibre.nativeffi.internal.lifecycle.NativeQueriedFeatureList
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
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
    when (value) {
      is RenderedQueryGeometry.Point ->
        mln_rendered_query_geometry_point(CoreStructs.screenPoint(value.point)).place(native.ptr)
      is RenderedQueryGeometry.Box ->
        mln_rendered_query_geometry_box(screenBox(value.box)).place(native.ptr)
      is RenderedQueryGeometry.LineString ->
        mln_rendered_query_geometry_line_string(
            CoreStructs.screenPointArray(value.points, scope),
            value.points.size.toULong(),
          )
          .place(native.ptr)
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
      val layerIdSnapshot = layerIds.toList()
      native.fields = native.fields or MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
      native.layer_ids = stringViewArray(layerIdSnapshot, scope)
      native.layer_id_count = layerIdSnapshot.size.toULong()
    }
    value.filterTransit?.let { filter ->
      native.filter = ByteStructs.bufferViewPointer(filter, scope)
    }
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
      val sourceLayerIdSnapshot = sourceLayerIds.toList()
      native.fields = native.fields or MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
      native.source_layer_ids = stringViewArray(sourceLayerIdSnapshot, scope)
      native.source_layer_id_count = sourceLayerIdSnapshot.size.toULong()
    }
    value.filterTransit?.let { filter ->
      native.filter = ByteStructs.bufferViewPointer(filter, scope)
    }
    return native.ptr
  }

  fun queriedFeatureList(list: NativeQueriedFeatureList): List<QueriedFeature> =
    queriedFeatureList(
      list.rawHandleValue,
      counter = ::mln_queried_feature_list_count,
      getter = ::mln_queried_feature_list_get,
      destroyer = ::mln_queried_feature_list_destroy,
    )

  fun queriedFeatureList(
    list: ULong,
    counter: (ULong, CPointer<ULongVar>) -> Int,
    getter: (ULong, ULong, CPointer<mln_queried_feature>) -> Int,
    destroyer: (ULong) -> Unit,
  ): List<QueriedFeature> =
    try {
      memScoped {
        val outCount = alloc<ULongVar>()
        Status.check(counter(list, outCount.ptr))
        List(checkedInt(outCount.value, "queried feature count")) { index ->
          val outFeature = alloc<mln_queried_feature>()
          mln_queried_feature_default().place(outFeature.ptr)
          Status.check(getter(list, index.toULong(), outFeature.ptr))
          queriedFeature(outFeature)
        }
      }
    } finally {
      destroyer(list)
    }

  fun queriedFeatures(values: CPointer<mln_queried_feature>?, count: ULong): List<QueriedFeature> {
    val size = checkedInt(count, "queried feature count")
    if (size == 0) return emptyList()
    requireNotNull(values) { "native completion omitted its queried features" }
    return List(size) { index -> queriedFeature(values[index]) }
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

  private fun screenBox(value: ScreenBox): CValue<mln_screen_box> = cValue {
    min.x = value.min.x
    min.y = value.min.y
    max.x = value.max.x
    max.y = value.max.y
  }

  private fun queriedFeature(value: mln_queried_feature): QueriedFeature {
    val fields = value.fields
    return QueriedFeature(
      ByteStructs.copyBufferView(value.feature),
      if (fields and MLN_QUERIED_FEATURE_SOURCE_ID != 0u) CoreStructs.stringView(value.source_id)
      else null,
      if (fields and MLN_QUERIED_FEATURE_SOURCE_LAYER_ID != 0u)
        CoreStructs.stringView(value.source_layer_id)
      else null,
      if (fields and MLN_QUERIED_FEATURE_STATE != 0u) ByteStructs.copyBufferView(value.state)
      else null,
    )
  }

  private fun stringViewArray(values: List<String>, scope: MemScope): CPointer<mln_buffer_view>? {
    if (values.isEmpty()) return null
    val array = scope.allocArray<mln_buffer_view>(values.size)
    values.forEachIndexed { index, value -> CoreStructs.setStringView(array[index], value, scope) }
    return array
  }

  private fun checkedInt(value: ULong, name: String): Int {
    require(value <= Int.MAX_VALUE.toULong()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }
}
