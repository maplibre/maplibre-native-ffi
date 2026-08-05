package org.maplibre.nativeffi.internal.struct

import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNull
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.set
import kotlinx.cinterop.sizeOf
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES
import org.maplibre.nativeffi.internal.c.MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO
import org.maplibre.nativeffi.internal.c.MLN_STYLE_IMAGE_OPTION_SDF
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM
import org.maplibre.nativeffi.internal.c.MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE
import org.maplibre.nativeffi.internal.c.mln_geojson_source_options
import org.maplibre.nativeffi.internal.c.mln_geojson_source_options_default
import org.maplibre.nativeffi.internal.c.mln_style_image_info
import org.maplibre.nativeffi.internal.c.mln_style_image_options
import org.maplibre.nativeffi.internal.c.mln_style_source_info
import org.maplibre.nativeffi.internal.c.mln_style_tile_source_options
import org.maplibre.nativeffi.internal.lifecycle.SyntheticHandles
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.TileSourceOptions

@OptIn(ExperimentalForeignApi::class)
class StyleStructsTest : org.maplibre.nativeffi.NativeTestBase() {
  // BND-060, BND-061, BND-062, BND-066: style options, source info, and list cleanup.

  @Test
  fun styleOptionsInitializeDefaultsAndPresentZeroMasks() {
    memScoped {
      val absentImage = StyleStructs.styleImageOptions(null, this).pointed
      assertEquals(sizeOf<mln_style_image_options>().toUInt(), absentImage.size)
      assertEquals(0U, absentImage.fields)

      val image =
        StyleStructs.styleImageOptions(
            StyleImageOptions().apply {
              pixelRatio = 0.0f
              sdf = false
            },
            this,
          )
          .pointed
      assertEquals(MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO or MLN_STYLE_IMAGE_OPTION_SDF, image.fields)
      assertEquals(0.0f, image.pixel_ratio)
      assertEquals(false, image.sdf)

      val source =
        StyleStructs.tileSourceOptions(
            TileSourceOptions().apply {
              minZoom = 0.0
              tileSize = 0
              bounds = LatLngBounds(LatLng(0.0, 0.0), LatLng(0.0, 0.0))
            },
            this,
          )!!
          .pointed
      assertEquals(sizeOf<mln_style_tile_source_options>().toUInt(), source.size)
      assertEquals(
        MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM or
          MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS or
          MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE,
        source.fields,
      )
      assertEquals(0.0, source.min_zoom)
      assertEquals(0U, source.tile_size)
      assertEquals(0.0, source.bounds.southwest.latitude)
      assertEquals(0.0, source.bounds.northeast.longitude)
    }
  }

  @Test
  fun geoJsonSourceOptionsKeepNativeDefaultsAndMaskPresentZeroValues() {
    memScoped {
      assertNull(StyleStructs.geoJsonSourceOptions(null, this))

      val clusterProperties =
        JsonValue.ObjectValue(
          listOf(
            JsonValue.Member(
              "weight",
              JsonValue.Array(
                listOf(
                  JsonValue.StringValue("+"),
                  JsonValue.Array(
                    listOf(JsonValue.StringValue("get"), JsonValue.StringValue("weight"))
                  ),
                )
              ),
            )
          )
        )
      val options =
        StyleStructs.geoJsonSourceOptions(
            GeoJsonSourceOptions().apply {
              minZoom = 0.0
              clusterMinPoints = 0
              cluster = false
              this.clusterProperties = clusterProperties
            },
            this,
          )!!
          .pointed

      assertEquals(sizeOf<mln_geojson_source_options>().toUInt(), options.size)
      assertEquals(
        MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM or
          MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS or
          MLN_GEOJSON_SOURCE_OPTION_CLUSTER or
          MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES,
        options.fields,
      )
      assertEquals(0.0, options.min_zoom)
      assertEquals(0U, options.cluster_min_points)
      assertEquals(false, options.cluster)
      assertEquals(clusterProperties, ValueStructs.jsonSnapshot(options.cluster_properties))

      // Fields left absent keep the values mln_geojson_source_options_default() wrote.
      val defaults = alloc<mln_geojson_source_options>()
      mln_geojson_source_options_default().place(defaults.ptr)
      assertEquals(defaults.max_zoom, options.max_zoom)
      assertEquals(defaults.tolerance, options.tolerance)
      assertEquals(defaults.tile_size, options.tile_size)
      assertEquals(defaults.cluster_radius, options.cluster_radius)
    }
  }

  @Test
  fun styleIdListCopiesIdsAndDestroysNativeHandle() {
    var destroys = 0
    val ids = memScoped {
      val list = SyntheticHandles.styleIdList()

      StyleStructs.styleIdList(
        list.rawHandleValue,
        counter = { _, outCount ->
          outCount[0] = 1UL
          MaplibreStatus.OK.nativeCode
        },
        getter = { _, _, outId ->
          CoreStructs.setStringView(outId.pointed, "roads", this)
          MaplibreStatus.OK.nativeCode
        },
        destroyer = { destroys++ },
      )
    }

    assertEquals(listOf("roads"), ids)
    assertEquals(1, destroys)
  }

  @Test
  fun styleIdListDestroysNativeHandleWhenCopyFails() {
    memScoped {
      var destroys = 0
      val list = SyntheticHandles.styleIdList()

      assertFailsWith<IllegalArgumentException> {
        StyleStructs.styleIdList(
          list.rawHandleValue,
          counter = { _, outCount ->
            outCount[0] = Int.MAX_VALUE.toULong() + 1UL
            MaplibreStatus.OK.nativeCode
          },
          getter = { _, _, _ -> MaplibreStatus.OK.nativeCode },
          destroyer = { destroys++ },
        )
      }

      assertEquals(1, destroys)
    }
  }

  @Test
  fun styleStringListCopiesValuesAndDestroysOnSuccessAndFailure() {
    memScoped {
      var destroys = 0
      val values =
        StyleStructs.styleStringList(
          123UL,
          counter = { _, outCount ->
            outCount[0] = 1UL
            MaplibreStatus.OK.nativeCode
          },
          getter = { _, _, outValue ->
            CoreStructs.setStringView(outValue.pointed, "tile", this)
            MaplibreStatus.OK.nativeCode
          },
          destroyer = { destroys++ },
        )
      assertEquals(listOf("tile"), values)
      assertEquals(1, destroys)

      assertFailsWith<IllegalArgumentException> {
        StyleStructs.styleStringList(
          456UL,
          counter = { _, outCount ->
            outCount[0] = Int.MAX_VALUE.toULong() + 1UL
            MaplibreStatus.OK.nativeCode
          },
          getter = { _, _, _ -> MaplibreStatus.OK.nativeCode },
          destroyer = { destroys++ },
        )
      }
      assertEquals(2, destroys)
    }
  }

  @Test
  fun sourceInfoPreservesUnknownTypeAndStableFields() {
    memScoped {
      val native = alloc<mln_style_source_info>()
      native.type = 999U
      native.is_volatile = true

      val info = StyleStructs.sourceInfo(native, "source attribution")

      assertEquals(SourceType(999), info.type)
      assertEquals(999, info.type.nativeValue)
      assertEquals(true, info.volatileSource)
      assertEquals("source attribution", info.attribution)
    }
  }

  @Test
  fun styleImageInfoCopiesMetadataAndRejectsOverflow() {
    memScoped {
      val native = alloc<mln_style_image_info>()
      native.width = 1U
      native.height = 2U
      native.stride = 8U
      native.byte_length = 16UL
      native.pixel_ratio = 2.0f
      native.sdf = true

      val info = StyleStructs.styleImageInfo(native)

      assertEquals(1, info.width)
      assertEquals(2, info.height)
      assertEquals(8, info.stride)
      assertEquals(16L, info.byteLength)
      assertEquals(2.0f, info.pixelRatio)
      assertEquals(true, info.sdf)

      native.byte_length = Long.MAX_VALUE.toULong() + 1UL
      assertFailsWith<IllegalArgumentException> { StyleStructs.styleImageInfo(native) }
    }
  }
}
