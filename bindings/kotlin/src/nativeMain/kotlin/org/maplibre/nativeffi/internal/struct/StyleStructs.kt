package org.maplibre.nativeffi.internal.struct

import cnames.structs.mln_style_id_list
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
import org.maplibre.nativeffi.internal.c.mln_canonical_tile_id
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
    return native.ptr
  }

  fun styleImageInfo(value: mln_style_image_info): StyleImageInfo =
    StyleImageInfo(
      checkedInt(value.width, "style image width"),
      checkedInt(value.height, "style image height"),
      checkedInt(value.stride, "style image stride"),
      checkedLong(value.byte_length, "style image byte length"),
      value.pixel_ratio,
      value.sdf,
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

  fun stringViewArray(values: List<String>, scope: MemScope): CPointer<mln_string_view>? {
    if (values.isEmpty()) return null
    val array = scope.allocArray<mln_string_view>(values.size)
    values.forEachIndexed { index, value -> CoreStructs.setStringView(array[index], value, scope) }
    return array
  }

  fun styleIdList(list: CPointer<mln_style_id_list>): List<String> =
    styleIdList(
      list,
      counter = ::mln_style_id_list_count,
      getter = ::mln_style_id_list_get,
      destroyer = ::mln_style_id_list_destroy,
    )

  fun styleIdList(
    list: CPointer<mln_style_id_list>,
    counter: (CPointer<mln_style_id_list>, CPointer<ULongVar>) -> Int,
    getter: (CPointer<mln_style_id_list>, ULong, CPointer<mln_string_view>) -> Int,
    destroyer: (CPointer<mln_style_id_list>) -> Unit,
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

  fun sourceInfo(value: mln_style_source_info, attribution: String?): SourceInfo =
    SourceInfo(SourceType.fromNative(value.type), value.is_volatile, attribution)
}
