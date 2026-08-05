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
import kotlinx.cinterop.toCValues
import kotlinx.cinterop.value
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_BUFFER
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_CONTENT
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_SDF
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_STRETCH_X
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_STRETCH_Y
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
import org.maplibre.nativeffi.internal.c.MLN_STYLE_SOURCE_INFO_BOUNDS
import org.maplibre.nativeffi.internal.c.MLN_STYLE_SOURCE_INFO_RASTER_ENCODING
import org.maplibre.nativeffi.internal.c.MLN_STYLE_SOURCE_INFO_TILEJSON
import org.maplibre.nativeffi.internal.c.MLN_STYLE_SOURCE_INFO_TILE_SIZE
import org.maplibre.nativeffi.internal.c.MLN_STYLE_SOURCE_INFO_URL
import org.maplibre.nativeffi.internal.c.MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TRANSITION_OPTION_DELAY
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TRANSITION_OPTION_DURATION
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
import org.maplibre.nativeffi.internal.c.mln_canonical_tile_id
import org.maplibre.nativeffi.internal.c.mln_geojson_source_options
import org.maplibre.nativeffi.internal.c.mln_geojson_source_options_default
import org.maplibre.nativeffi.internal.c.mln_image_stretch
import org.maplibre.nativeffi.internal.c.mln_premultiplied_rgba8_image
import org.maplibre.nativeffi.internal.c.mln_premultiplied_rgba8_image_default
import org.maplibre.nativeffi.internal.c.mln_string_view
import org.maplibre.nativeffi.internal.c.mln_style_id_list_count
import org.maplibre.nativeffi.internal.c.mln_style_id_list_destroy
import org.maplibre.nativeffi.internal.c.mln_style_id_list_get
import org.maplibre.nativeffi.internal.c.mln_style_image_info
import org.maplibre.nativeffi.internal.c.mln_style_image_options
import org.maplibre.nativeffi.internal.c.mln_style_image_options_default
import org.maplibre.nativeffi.internal.c.mln_style_source_info
import org.maplibre.nativeffi.internal.c.mln_style_string_list_count
import org.maplibre.nativeffi.internal.c.mln_style_string_list_destroy
import org.maplibre.nativeffi.internal.c.mln_style_string_list_get
import org.maplibre.nativeffi.internal.c.mln_style_tile_source_options
import org.maplibre.nativeffi.internal.c.mln_style_tile_source_options_default
import org.maplibre.nativeffi.internal.c.mln_style_transition_options
import org.maplibre.nativeffi.internal.c.mln_style_transition_options_default
import org.maplibre.nativeffi.internal.lifecycle.NativeStyleIdList
import org.maplibre.nativeffi.internal.lifecycle.NativeStyleStringList
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.ImageContent
import org.maplibre.nativeffi.style.ImageStretch
import org.maplibre.nativeffi.style.RasterDemEncoding
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleImageTextFit
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileJson
import org.maplibre.nativeffi.style.TileScheme
import org.maplibre.nativeffi.style.TileSourceOptions
import org.maplibre.nativeffi.style.VectorTileEncoding

/** Copies style-owned list and metadata handles into Kotlin values. */
@OptIn(ExperimentalForeignApi::class, ExperimentalUnsignedTypes::class)
internal object StyleStructs {
  fun canonicalTileId(value: CanonicalTileId): CValue<mln_canonical_tile_id> = cValue {
    require(value.z >= 0) { "canonical tile z must be non-negative" }
    require(value.x in 0..UInt.MAX_VALUE.toLong()) { "canonical tile x is out of range" }
    require(value.y in 0..UInt.MAX_VALUE.toLong()) { "canonical tile y is out of range" }
    z = value.z.toUInt()
    x = value.x.toUInt()
    y = value.y.toUInt()
  }

  fun canonicalTileId(value: mln_canonical_tile_id): CanonicalTileId =
    CanonicalTileId(checkedInt(value.z, "canonical tile z"), value.x.toLong(), value.y.toLong())

  fun premultipliedRgba8Image(
    value: PremultipliedRgba8Image,
    scope: MemScope,
  ): CPointer<mln_premultiplied_rgba8_image> {
    val native = scope.alloc<mln_premultiplied_rgba8_image>()
    mln_premultiplied_rgba8_image_default().place(native.ptr)
    native.width = value.width.toUInt()
    native.height = value.height.toUInt()
    native.stride = value.stride.toUInt()
    native.pixels = value.pixels.toUByteArray().toCValues().getPointer(scope)
    native.byte_length = value.pixels.size.toULong()
    return native.ptr
  }

  fun styleImageOptions(
    value: StyleImageOptions?,
    scope: MemScope,
  ): CPointer<mln_style_image_options> {
    val native = scope.alloc<mln_style_image_options>()
    mln_style_image_options_default().place(native.ptr)
    value?.pixelRatio?.let {
      native.fields = native.fields or MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
      native.pixel_ratio = it
    }
    value?.sdf?.let {
      native.fields = native.fields or MLN_STYLE_IMAGE_OPTION_SDF
      native.sdf = it
    }
    value?.stretchX?.let {
      native.fields = native.fields or MLN_STYLE_IMAGE_OPTION_STRETCH_X
      native.stretch_x = imageStretchArray(it, scope)
      native.stretch_x_count = it.size.toULong()
    }
    value?.stretchY?.let {
      native.fields = native.fields or MLN_STYLE_IMAGE_OPTION_STRETCH_Y
      native.stretch_y = imageStretchArray(it, scope)
      native.stretch_y_count = it.size.toULong()
    }
    value?.content?.let {
      native.fields = native.fields or MLN_STYLE_IMAGE_OPTION_CONTENT
      native.content.left = it.left
      native.content.top = it.top
      native.content.right = it.right
      native.content.bottom = it.bottom
    }
    value?.textFitWidth?.let {
      native.fields = native.fields or MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH
      native.text_fit_width = it.nativeValue.toUInt()
    }
    value?.textFitHeight?.let {
      native.fields = native.fields or MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT
      native.text_fit_height = it.nativeValue.toUInt()
    }
    return native.ptr
  }

  fun styleTransitionOptions(
    value: StyleTransitionOptions,
    scope: MemScope,
  ): CPointer<mln_style_transition_options> {
    val native = scope.alloc<mln_style_transition_options>()
    mln_style_transition_options_default().place(native.ptr)
    value.enablePlacementTransitions?.let {
      native.fields = native.fields or MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS
      native.enable_placement_transitions = it
    }
    value.durationMs?.let {
      native.fields = native.fields or MLN_STYLE_TRANSITION_OPTION_DURATION
      native.duration_ms = it
    }
    value.delayMs?.let {
      native.fields = native.fields or MLN_STYLE_TRANSITION_OPTION_DELAY
      native.delay_ms = it
    }
    return native.ptr
  }

  fun styleTransitionOptions(value: mln_style_transition_options): StyleTransitionOptions =
    StyleTransitionOptions().apply {
      durationMs =
        if (value.fields and MLN_STYLE_TRANSITION_OPTION_DURATION != 0u) value.duration_ms else null
      delayMs =
        if (value.fields and MLN_STYLE_TRANSITION_OPTION_DELAY != 0u) value.delay_ms else null
      enablePlacementTransitions =
        if (value.fields and MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS != 0u)
          value.enable_placement_transitions
        else null
    }

  private fun imageStretchArray(
    stretches: List<ImageStretch>,
    scope: MemScope,
  ): CPointer<mln_image_stretch>? {
    if (stretches.isEmpty()) return null
    val array = scope.allocArray<mln_image_stretch>(stretches.size)
    stretches.forEachIndexed { index, stretch ->
      array[index].from = stretch.from
      array[index].to = stretch.to
    }
    return array
  }

  fun styleImageInfo(value: mln_style_image_info): StyleImageInfo =
    StyleImageInfo(
      checkedInt(value.width, "style image width"),
      checkedInt(value.height, "style image height"),
      checkedInt(value.stride, "style image stride"),
      checkedLong(value.byte_length, "style image byte length"),
      value.pixel_ratio,
      value.sdf,
      checkedLong(value.stretch_x_count, "style image stretch x count"),
      checkedLong(value.stretch_y_count, "style image stretch y count"),
      if (value.has_content)
        ImageContent(
          value.content.left,
          value.content.top,
          value.content.right,
          value.content.bottom,
        )
      else null,
      if (value.has_text_fit_width) StyleImageTextFit.fromNative(value.text_fit_width) else null,
      if (value.has_text_fit_height) StyleImageTextFit.fromNative(value.text_fit_height) else null,
    )

  private fun checkedInt(value: UInt, name: String): Int {
    require(value <= Int.MAX_VALUE.toUInt()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  private fun checkedInt(value: ULong, name: String): Int {
    require(value <= Int.MAX_VALUE.toULong()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  private fun checkedLong(value: ULong, name: String): Long {
    require(value <= Long.MAX_VALUE.toULong()) { "$name exceeds Long.MAX_VALUE" }
    return value.toLong()
  }

  fun tileSourceOptions(
    value: TileSourceOptions?,
    scope: MemScope,
  ): CPointer<mln_style_tile_source_options>? {
    if (value == null) return null
    val native = scope.alloc<mln_style_tile_source_options>()
    mln_style_tile_source_options_default().place(native.ptr)
    value.minZoom?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
      native.min_zoom = it
    }
    value.maxZoom?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
      native.max_zoom = it
    }
    value.attribution?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
      CoreStructs.setStringView(native.attribution, it, scope)
    }
    value.scheme?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
      native.scheme = it.nativeValue.toUInt()
    }
    value.bounds?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
      native.bounds.southwest.latitude = it.southwest.latitude
      native.bounds.southwest.longitude = it.southwest.longitude
      native.bounds.northeast.latitude = it.northeast.latitude
      native.bounds.northeast.longitude = it.northeast.longitude
    }
    value.tileSize?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
      native.tile_size = it.toUInt()
    }
    value.vectorEncoding?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
      native.vector_encoding = it.nativeValue.toUInt()
    }
    value.rasterDemEncoding?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
      native.raster_encoding = it.nativeValue.toUInt()
    }
    return native.ptr
  }

  fun geoJsonSourceOptions(
    value: GeoJsonSourceOptions?,
    scope: MemScope,
  ): CPointer<mln_geojson_source_options>? {
    if (value == null) return null
    val native = scope.alloc<mln_geojson_source_options>()
    mln_geojson_source_options_default().place(native.ptr)
    value.minZoom?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
      native.min_zoom = it
    }
    value.maxZoom?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM
      native.max_zoom = it
    }
    value.tolerance?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_TOLERANCE
      native.tolerance = it
    }
    value.clusterMaxZoom?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM
      native.cluster_max_zoom = it
    }
    value.clusterProperties?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
      native.cluster_properties = ValueStructs.jsonValue(it, scope)
    }
    value.tileSize?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE
      native.tile_size = it.toUInt()
    }
    value.buffer?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_BUFFER
      native.buffer = it.toUInt()
    }
    value.clusterRadius?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS
      native.cluster_radius = it.toUInt()
    }
    value.clusterMinPoints?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
      native.cluster_min_points = it.toUInt()
    }
    value.lineMetrics?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS
      native.line_metrics = it
    }
    value.cluster?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_CLUSTER
      native.cluster = it
    }
    value.synchronousUpdate?.let {
      native.fields = native.fields or MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_UPDATE
      native.synchronous_update = it
    }
    return native.ptr
  }

  fun stringViewArray(values: List<String>, scope: MemScope): CPointer<mln_string_view>? {
    if (values.isEmpty()) return null
    val array = scope.allocArray<mln_string_view>(values.size)
    values.forEachIndexed { index, value -> CoreStructs.setStringView(array[index], value, scope) }
    return array
  }

  fun styleIdList(list: NativeStyleIdList): List<String> =
    styleIdList(
      list.rawHandleValue,
      counter = ::mln_style_id_list_count,
      getter = ::mln_style_id_list_get,
      destroyer = ::mln_style_id_list_destroy,
    )

  fun styleIdList(
    list: ULong,
    counter: (ULong, CPointer<ULongVar>) -> Int,
    getter: (ULong, ULong, CPointer<mln_string_view>) -> Int,
    destroyer: (ULong) -> Unit,
  ): List<String> =
    try {
      memScoped {
        val outCount = alloc<ULongVar>()
        Status.check(counter(list, outCount.ptr))
        List(checkedInt(outCount.value, "style id count")) { index ->
          val outId = alloc<mln_string_view>()
          Status.check(getter(list, index.toULong(), outId.ptr))
          CoreStructs.stringView(outId)
        }
      }
    } finally {
      destroyer(list)
    }

  fun styleStringList(list: NativeStyleStringList): List<String> =
    styleStringList(
      list.rawHandleValue,
      counter = ::mln_style_string_list_count,
      getter = ::mln_style_string_list_get,
      destroyer = ::mln_style_string_list_destroy,
    )

  fun styleStringList(
    list: ULong,
    counter: (ULong, CPointer<ULongVar>) -> Int,
    getter: (ULong, ULong, CPointer<mln_string_view>) -> Int,
    destroyer: (ULong) -> Unit,
  ): List<String> = styleIdList(list, counter, getter, destroyer)

  fun sourceInfo(
    value: mln_style_source_info,
    attribution: String?,
    url: String? = null,
    tileUrls: List<String>? = null,
  ): SourceInfo {
    val fields = value.fields
    return SourceInfo(
      SourceType.fromNative(value.type),
      value.is_volatile,
      attribution,
      if (fields and MLN_STYLE_SOURCE_INFO_URL != 0u) url else null,
      if (fields and MLN_STYLE_SOURCE_INFO_TILEJSON != 0u)
        TileJson(
          tileUrls.orEmpty(),
          value.min_zoom,
          value.max_zoom,
          TileScheme.fromNative(value.scheme),
          if (fields and MLN_STYLE_SOURCE_INFO_BOUNDS != 0u) CoreStructs.latLngBounds(value.bounds)
          else null,
        )
      else null,
      if (fields and MLN_STYLE_SOURCE_INFO_TILE_SIZE != 0u)
        checkedInt(value.tile_size, "style source tile size")
      else null,
      if (fields and MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING != 0u)
        VectorTileEncoding.fromNative(value.vector_encoding)
      else null,
      if (fields and MLN_STYLE_SOURCE_INFO_RASTER_ENCODING != 0u)
        RasterDemEncoding.fromNative(value.raster_encoding)
      else null,
    )
  }
}
