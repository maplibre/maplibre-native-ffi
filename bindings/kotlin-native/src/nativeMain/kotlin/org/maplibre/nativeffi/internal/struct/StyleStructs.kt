package org.maplibre.nativeffi.internal.struct

import cnames.structs.mln_style_id_list
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.toCValues
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_SDF
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_SCHEME
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
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
import org.maplibre.nativeffi.internal.c.mln_style_tile_source_options
import org.maplibre.nativeffi.internal.c.mln_style_tile_source_options_default
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Copies style-owned list and metadata handles into Kotlin values. */
@OptIn(ExperimentalForeignApi::class, ExperimentalUnsignedTypes::class)
internal object StyleStructs {
  fun premultipliedRgba8Image(
    value: PremultipliedRgba8Image,
    scope: MemScope,
  ): CPointer<mln_premultiplied_rgba8_image> {
    val native = scope.alloc<mln_premultiplied_rgba8_image>()
    mln_premultiplied_rgba8_image_default().place(native.ptr)
    native.width = value.width
    native.height = value.height
    native.stride = value.stride
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
    return native.ptr
  }

  fun styleImageInfo(value: mln_style_image_info): StyleImageInfo =
    StyleImageInfo(
      value.width,
      value.height,
      value.stride,
      value.byte_length,
      value.pixel_ratio,
      value.sdf,
    )

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
      native.scheme = it.nativeValue
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
      native.tile_size = it
    }
    value.vectorEncoding?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING
      native.vector_encoding = it.nativeValue
    }
    value.rasterDemEncoding?.let {
      native.fields = native.fields or MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING
      native.raster_encoding = it.nativeValue
    }
    return native.ptr
  }

  fun stringViewArray(values: List<String>, scope: MemScope): CPointer<mln_string_view>? {
    if (values.isEmpty()) return null
    val array = scope.allocArray<mln_string_view>(values.size)
    values.forEachIndexed { index, value -> CoreStructs.setStringView(array[index], value, scope) }
    return array
  }

  fun styleIdList(list: CPointer<mln_style_id_list>): List<String> =
    try {
      memScoped {
        val outCount = alloc<ULongVar>()
        Status.check(mln_style_id_list_count(list, outCount.ptr))
        List(outCount.value.toInt()) { index ->
          val outId = alloc<mln_string_view>()
          Status.check(mln_style_id_list_get(list, index.toULong(), outId.ptr))
          CoreStructs.stringView(outId)
        }
      }
    } finally {
      mln_style_id_list_destroy(list)
    }

  fun sourceInfo(value: mln_style_source_info, attribution: String?): SourceInfo =
    SourceInfo(SourceType.fromNative(value.type), value.id_size, value.is_volatile, attribution)
}
