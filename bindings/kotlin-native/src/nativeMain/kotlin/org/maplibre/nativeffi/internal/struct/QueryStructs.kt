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
import org.maplibre.nativeffi.geo.Feature
import org.maplibre.nativeffi.geo.ScreenBox
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_FEATURE_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID
import org.maplibre.nativeffi.internal.c.MLN_FEATURE_STATE_SELECTOR_STATE_KEY
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_SOURCE_ID
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_SOURCE_LAYER_ID
import org.maplibre.nativeffi.internal.c.MLN_QUERIED_FEATURE_STATE
import org.maplibre.nativeffi.internal.c.MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS
import org.maplibre.nativeffi.internal.c.MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
import org.maplibre.nativeffi.internal.c.mln_feature_collection
import org.maplibre.nativeffi.internal.c.mln_feature_extension_result_destroy
import org.maplibre.nativeffi.internal.c.mln_feature_extension_result_get
import org.maplibre.nativeffi.internal.c.mln_feature_extension_result_info
import org.maplibre.nativeffi.internal.c.mln_feature_query_result_count
import org.maplibre.nativeffi.internal.c.mln_feature_query_result_destroy
import org.maplibre.nativeffi.internal.c.mln_feature_query_result_get
import org.maplibre.nativeffi.internal.c.mln_feature_state_selector
import org.maplibre.nativeffi.internal.c.mln_queried_feature
import org.maplibre.nativeffi.internal.c.mln_rendered_feature_query_options
import org.maplibre.nativeffi.internal.c.mln_rendered_feature_query_options_default
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry_box
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry_line_string
import org.maplibre.nativeffi.internal.c.mln_rendered_query_geometry_point
import org.maplibre.nativeffi.internal.c.mln_screen_box
import org.maplibre.nativeffi.internal.c.mln_source_feature_query_options
import org.maplibre.nativeffi.internal.c.mln_source_feature_query_options_default
import org.maplibre.nativeffi.internal.c.mln_string_view
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.query.FeatureExtensionResult
import org.maplibre.nativeffi.query.FeatureStateSelector
import org.maplibre.nativeffi.query.QueriedFeature
import org.maplibre.nativeffi.query.RenderedFeatureQueryOptions
import org.maplibre.nativeffi.query.RenderedQueryGeometry
import org.maplibre.nativeffi.query.SourceFeatureQueryOptions

/** Materializes feature query descriptors at the C boundary. */
@OptIn(ExperimentalForeignApi::class)
internal object QueryStructs {
  fun featureQueryResult(
    result: CPointer<cnames.structs.mln_feature_query_result>,
    counter: (CPointer<cnames.structs.mln_feature_query_result>, CPointer<ULongVar>) -> Int =
      ::mln_feature_query_result_count,
    getter:
      (
        CPointer<cnames.structs.mln_feature_query_result>, ULong, CPointer<mln_queried_feature>,
      ) -> Int =
      ::mln_feature_query_result_get,
    destroyer: (CPointer<cnames.structs.mln_feature_query_result>) -> Unit =
      ::mln_feature_query_result_destroy,
  ): List<QueriedFeature> =
    try {
      memScoped {
        val outCount = alloc<ULongVar>()
        Status.check(counter(result, outCount.ptr))
        List(checkedInt(outCount.value, "queried feature count")) { index ->
          val outFeature = alloc<mln_queried_feature>()
          outFeature.size = sizeOf<mln_queried_feature>().toUInt()
          Status.check(getter(result, index.toULong(), outFeature.ptr))
          queriedFeature(outFeature)
        }
      }
    } finally {
      destroyer(result)
    }

  fun featureExtensionResult(
    result: CPointer<cnames.structs.mln_feature_extension_result>,
    getter:
      (
        CPointer<cnames.structs.mln_feature_extension_result>,
        CPointer<mln_feature_extension_result_info>,
      ) -> Int =
      ::mln_feature_extension_result_get,
    destroyer: (CPointer<cnames.structs.mln_feature_extension_result>) -> Unit =
      ::mln_feature_extension_result_destroy,
  ): FeatureExtensionResult =
    try {
      memScoped {
        val info = alloc<mln_feature_extension_result_info>()
        info.size = sizeOf<mln_feature_extension_result_info>().toUInt()
        Status.check(getter(result, info.ptr))
        when (info.type) {
          MLN_FEATURE_EXTENSION_RESULT_TYPE_VALUE ->
            FeatureExtensionResult.Value(ValueStructs.jsonSnapshot(info.data.value))
          MLN_FEATURE_EXTENSION_RESULT_TYPE_FEATURE_COLLECTION ->
            FeatureExtensionResult.FeatureCollection(
              featureCollection(info.data.feature_collection)
            )
          else -> FeatureExtensionResult.Unknown(info.type.toInt())
        }
      }
    } finally {
      destroyer(result)
    }

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
      val sourceLayerIdSnapshot = sourceLayerIds.toList()
      native.fields = native.fields or MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS
      native.source_layer_ids = stringViewArray(sourceLayerIdSnapshot, scope)
      native.source_layer_id_count = sourceLayerIdSnapshot.size.toULong()
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

  private fun queriedFeature(value: mln_queried_feature): QueriedFeature {
    val fields = value.fields
    val sourceId =
      if ((fields and MLN_QUERIED_FEATURE_SOURCE_ID) != 0U) CoreStructs.stringView(value.source_id)
      else null
    val sourceLayerId =
      if ((fields and MLN_QUERIED_FEATURE_SOURCE_LAYER_ID) != 0U)
        CoreStructs.stringView(value.source_layer_id)
      else null
    val state =
      if ((fields and MLN_QUERIED_FEATURE_STATE) != 0U && value.state != null) {
        ValueStructs.jsonSnapshot(value.state)
      } else {
        null
      }
    return QueriedFeature(
      ValueStructs.featureSnapshot(value.feature.ptr),
      sourceId,
      sourceLayerId,
      state,
    )
  }

  private fun checkedInt(value: ULong, name: String): Int {
    require(value <= Int.MAX_VALUE.toULong()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  private fun featureCollection(value: mln_feature_collection): List<Feature> =
    List(checkedInt(value.feature_count, "feature extension feature count")) { index ->
      ValueStructs.featureSnapshot(value.features!![index].ptr)
    }

  private fun screenBox(value: ScreenBox): CValue<mln_screen_box> = cValue {
    min.x = value.min.x
    min.y = value.min.y
    max.x = value.max.x
    max.y = value.max.y
  }

  private fun stringViewArray(values: List<String>, scope: MemScope): CPointer<mln_string_view>? {
    if (values.isEmpty()) return null
    val array = scope.allocArray<mln_string_view>(values.size)
    values.forEachIndexed { index, value -> CoreStructs.setStringView(array[index], value, scope) }
    return array
  }
}
