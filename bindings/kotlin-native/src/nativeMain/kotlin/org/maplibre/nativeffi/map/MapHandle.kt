package org.maplibre.nativeffi.map

import cnames.structs.mln_json_snapshot
import cnames.structs.mln_map
import cnames.structs.mln_style_id_list
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.UIntVar
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.value
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.mln_bound_options_default
import org.maplibre.nativeffi.internal.c.mln_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_free_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_lat_lng_bounds
import org.maplibre.nativeffi.internal.c.mln_map_add_color_relief_layer
import org.maplibre.nativeffi.internal.c.mln_map_add_custom_geometry_source
import org.maplibre.nativeffi.internal.c.mln_map_add_geojson_source_data
import org.maplibre.nativeffi.internal.c.mln_map_add_geojson_source_url
import org.maplibre.nativeffi.internal.c.mln_map_add_hillshade_layer
import org.maplibre.nativeffi.internal.c.mln_map_add_image_source_image
import org.maplibre.nativeffi.internal.c.mln_map_add_image_source_url
import org.maplibre.nativeffi.internal.c.mln_map_add_location_indicator_layer
import org.maplibre.nativeffi.internal.c.mln_map_add_raster_dem_source_tiles
import org.maplibre.nativeffi.internal.c.mln_map_add_raster_dem_source_url
import org.maplibre.nativeffi.internal.c.mln_map_add_raster_source_tiles
import org.maplibre.nativeffi.internal.c.mln_map_add_raster_source_url
import org.maplibre.nativeffi.internal.c.mln_map_add_style_layer_json
import org.maplibre.nativeffi.internal.c.mln_map_add_style_source_json
import org.maplibre.nativeffi.internal.c.mln_map_add_vector_source_tiles
import org.maplibre.nativeffi.internal.c.mln_map_add_vector_source_url
import org.maplibre.nativeffi.internal.c.mln_map_camera_for_geometry
import org.maplibre.nativeffi.internal.c.mln_map_camera_for_lat_lng_bounds
import org.maplibre.nativeffi.internal.c.mln_map_camera_for_lat_lngs
import org.maplibre.nativeffi.internal.c.mln_map_cancel_transitions
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_image_premultiplied_rgba8
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_source_attribution
import org.maplibre.nativeffi.internal.c.mln_map_create
import org.maplibre.nativeffi.internal.c.mln_map_destroy
import org.maplibre.nativeffi.internal.c.mln_map_dump_debug_logs
import org.maplibre.nativeffi.internal.c.mln_map_ease_to
import org.maplibre.nativeffi.internal.c.mln_map_fly_to
import org.maplibre.nativeffi.internal.c.mln_map_get_bounds
import org.maplibre.nativeffi.internal.c.mln_map_get_camera
import org.maplibre.nativeffi.internal.c.mln_map_get_debug_options
import org.maplibre.nativeffi.internal.c.mln_map_get_free_camera_options
import org.maplibre.nativeffi.internal.c.mln_map_get_image_source_coordinates
import org.maplibre.nativeffi.internal.c.mln_map_get_layer_filter
import org.maplibre.nativeffi.internal.c.mln_map_get_layer_property
import org.maplibre.nativeffi.internal.c.mln_map_get_projection_mode
import org.maplibre.nativeffi.internal.c.mln_map_get_rendering_stats_view_enabled
import org.maplibre.nativeffi.internal.c.mln_map_get_style_image_info
import org.maplibre.nativeffi.internal.c.mln_map_get_style_layer_json
import org.maplibre.nativeffi.internal.c.mln_map_get_style_layer_type
import org.maplibre.nativeffi.internal.c.mln_map_get_style_light_property
import org.maplibre.nativeffi.internal.c.mln_map_get_style_source_info
import org.maplibre.nativeffi.internal.c.mln_map_get_style_source_type
import org.maplibre.nativeffi.internal.c.mln_map_get_tile_options
import org.maplibre.nativeffi.internal.c.mln_map_get_viewport_options
import org.maplibre.nativeffi.internal.c.mln_map_invalidate_custom_geometry_source_region
import org.maplibre.nativeffi.internal.c.mln_map_invalidate_custom_geometry_source_tile
import org.maplibre.nativeffi.internal.c.mln_map_is_fully_loaded
import org.maplibre.nativeffi.internal.c.mln_map_jump_to
import org.maplibre.nativeffi.internal.c.mln_map_lat_lng_bounds_for_camera
import org.maplibre.nativeffi.internal.c.mln_map_lat_lng_bounds_for_camera_unwrapped
import org.maplibre.nativeffi.internal.c.mln_map_lat_lng_for_pixel
import org.maplibre.nativeffi.internal.c.mln_map_lat_lngs_for_pixels
import org.maplibre.nativeffi.internal.c.mln_map_list_style_layer_ids
import org.maplibre.nativeffi.internal.c.mln_map_list_style_source_ids
import org.maplibre.nativeffi.internal.c.mln_map_move_by
import org.maplibre.nativeffi.internal.c.mln_map_move_by_animated
import org.maplibre.nativeffi.internal.c.mln_map_move_style_layer
import org.maplibre.nativeffi.internal.c.mln_map_options
import org.maplibre.nativeffi.internal.c.mln_map_options_default
import org.maplibre.nativeffi.internal.c.mln_map_pitch_by
import org.maplibre.nativeffi.internal.c.mln_map_pitch_by_animated
import org.maplibre.nativeffi.internal.c.mln_map_pixel_for_lat_lng
import org.maplibre.nativeffi.internal.c.mln_map_pixels_for_lat_lngs
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_image
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_layer
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_source
import org.maplibre.nativeffi.internal.c.mln_map_request_repaint
import org.maplibre.nativeffi.internal.c.mln_map_request_still_image
import org.maplibre.nativeffi.internal.c.mln_map_rotate_by
import org.maplibre.nativeffi.internal.c.mln_map_rotate_by_animated
import org.maplibre.nativeffi.internal.c.mln_map_scale_by
import org.maplibre.nativeffi.internal.c.mln_map_scale_by_animated
import org.maplibre.nativeffi.internal.c.mln_map_set_bounds
import org.maplibre.nativeffi.internal.c.mln_map_set_custom_geometry_source_tile_data
import org.maplibre.nativeffi.internal.c.mln_map_set_debug_options
import org.maplibre.nativeffi.internal.c.mln_map_set_free_camera_options
import org.maplibre.nativeffi.internal.c.mln_map_set_geojson_source_data
import org.maplibre.nativeffi.internal.c.mln_map_set_geojson_source_url
import org.maplibre.nativeffi.internal.c.mln_map_set_image_source_coordinates
import org.maplibre.nativeffi.internal.c.mln_map_set_image_source_image
import org.maplibre.nativeffi.internal.c.mln_map_set_image_source_url
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_filter
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_property
import org.maplibre.nativeffi.internal.c.mln_map_set_location_indicator_accuracy_radius
import org.maplibre.nativeffi.internal.c.mln_map_set_location_indicator_bearing
import org.maplibre.nativeffi.internal.c.mln_map_set_location_indicator_image_name
import org.maplibre.nativeffi.internal.c.mln_map_set_location_indicator_location
import org.maplibre.nativeffi.internal.c.mln_map_set_projection_mode
import org.maplibre.nativeffi.internal.c.mln_map_set_rendering_stats_view_enabled
import org.maplibre.nativeffi.internal.c.mln_map_set_style_image
import org.maplibre.nativeffi.internal.c.mln_map_set_style_json
import org.maplibre.nativeffi.internal.c.mln_map_set_style_light_json
import org.maplibre.nativeffi.internal.c.mln_map_set_style_light_property
import org.maplibre.nativeffi.internal.c.mln_map_set_style_url
import org.maplibre.nativeffi.internal.c.mln_map_set_tile_options
import org.maplibre.nativeffi.internal.c.mln_map_set_viewport_options
import org.maplibre.nativeffi.internal.c.mln_map_style_image_exists
import org.maplibre.nativeffi.internal.c.mln_map_style_layer_exists
import org.maplibre.nativeffi.internal.c.mln_map_style_source_exists
import org.maplibre.nativeffi.internal.c.mln_map_tile_options_default
import org.maplibre.nativeffi.internal.c.mln_map_viewport_options_default
import org.maplibre.nativeffi.internal.c.mln_projection_mode_default
import org.maplibre.nativeffi.internal.c.mln_screen_point
import org.maplibre.nativeffi.internal.c.mln_style_image_info_default
import org.maplibre.nativeffi.internal.c.mln_style_source_info
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.MapStructs
import org.maplibre.nativeffi.internal.struct.StyleStructs
import org.maplibre.nativeffi.internal.struct.ValueStructs
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Owned native map handle. Close it on the map owner thread. */
@OptIn(ExperimentalForeignApi::class)
public class MapHandle
private constructor(private val runtime: RuntimeHandle, handle: CPointer<mln_map>) : AutoCloseable {
  private val state = HandleState("MapHandle", handle, runtime)
  private val customGeometrySources = mutableMapOf<String, CustomGeometrySourceState>()

  public fun setStyleUrl(url: String) {
    MemoryUtil.requireValidCString(url)
    Status.check(mln_map_set_style_url(state.requireLive(), url))
  }

  public fun setStyleJson(json: String) {
    MemoryUtil.requireValidCString(json)
    Status.check(mln_map_set_style_json(state.requireLive(), json))
    clearCustomGeometrySources()
  }

  public fun addStyleSourceJson(sourceId: String, sourceJson: JsonValue) {
    memScoped {
      Status.check(
        mln_map_add_style_source_json(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          ValueStructs.jsonValue(sourceJson, this),
        )
      )
    }
  }

  public fun removeStyleSource(sourceId: String): Boolean = memScoped {
    val outRemoved = alloc<BooleanVar>()
    Status.check(
      mln_map_remove_style_source(
        state.requireLive(),
        CoreStructs.stringView(sourceId, this),
        outRemoved.ptr,
      )
    )
    val removed = outRemoved.value
    if (removed) closeCustomGeometrySource(sourceId)
    removed
  }

  public fun styleSourceExists(sourceId: String): Boolean = memScoped {
    val outExists = alloc<BooleanVar>()
    Status.check(
      mln_map_style_source_exists(
        state.requireLive(),
        CoreStructs.stringView(sourceId, this),
        outExists.ptr,
      )
    )
    outExists.value
  }

  public fun styleSourceType(sourceId: String): SourceType? = memScoped {
    val outType = alloc<UIntVar>()
    val outFound = alloc<BooleanVar>()
    Status.check(
      mln_map_get_style_source_type(
        state.requireLive(),
        CoreStructs.stringView(sourceId, this),
        outType.ptr,
        outFound.ptr,
      )
    )
    if (outFound.value) SourceType.fromNative(outType.value) else null
  }

  public fun styleSourceInfo(sourceId: String): SourceInfo? = memScoped {
    val outInfo = alloc<mln_style_source_info>()
    outInfo.size = sizeOf<mln_style_source_info>().toUInt()
    val outFound = alloc<BooleanVar>()
    Status.check(
      mln_map_get_style_source_info(
        state.requireLive(),
        CoreStructs.stringView(sourceId, this),
        outInfo.ptr,
        outFound.ptr,
      )
    )
    if (!outFound.value) return@memScoped null
    val attribution = copyStyleSourceAttribution(sourceId, outInfo)
    StyleStructs.sourceInfo(outInfo, attribution)
  }

  public fun styleSourceIds(): List<String> = memScoped {
    val outList = alloc<CPointerVarOf<CPointer<mln_style_id_list>>>()
    outList.value = null
    Status.check(mln_map_list_style_source_ids(state.requireLive(), outList.ptr))
    StyleStructs.styleIdList(requireNotNull(outList.value))
  }

  public fun addGeoJsonSourceUrl(sourceId: String, url: String) {
    memScoped {
      Status.check(
        mln_map_add_geojson_source_url(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
        )
      )
    }
  }

  public fun addGeoJsonSourceData(sourceId: String, data: GeoJson) {
    memScoped {
      Status.check(
        mln_map_add_geojson_source_data(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          ValueStructs.geoJson(data, this),
        )
      )
    }
  }

  public fun setGeoJsonSourceUrl(sourceId: String, url: String) {
    memScoped {
      Status.check(
        mln_map_set_geojson_source_url(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
        )
      )
    }
  }

  public fun setGeoJsonSourceData(sourceId: String, data: GeoJson) {
    memScoped {
      Status.check(
        mln_map_set_geojson_source_data(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          ValueStructs.geoJson(data, this),
        )
      )
    }
  }

  public fun addCustomGeometrySource(sourceId: String, options: CustomGeometrySourceOptions) {
    val sourceState = CustomGeometrySourceState(options)
    try {
      memScoped {
        Status.check(
          mln_map_add_custom_geometry_source(
            state.requireLive(),
            CoreStructs.stringView(sourceId, this),
            sourceState.descriptor(),
          )
        )
      }
      customGeometrySources.put(sourceId, sourceState)?.close()
    } catch (error: Throwable) {
      sourceState.close()
      throw error
    }
  }

  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: GeoJson,
  ) {
    memScoped {
      Status.check(
        mln_map_set_custom_geometry_source_tile_data(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          StyleStructs.canonicalTileId(tileId),
          ValueStructs.geoJson(data, this),
        )
      )
    }
  }

  public fun invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileId) {
    memScoped {
      Status.check(
        mln_map_invalidate_custom_geometry_source_tile(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          StyleStructs.canonicalTileId(tileId),
        )
      )
    }
  }

  public fun invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds) {
    memScoped {
      Status.check(
        mln_map_invalidate_custom_geometry_source_region(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngBounds(bounds),
        )
      )
    }
  }

  internal fun customGeometrySourceCountForTesting(): Int = customGeometrySources.size

  private fun closeCustomGeometrySource(sourceId: String) {
    customGeometrySources.remove(sourceId)?.close()
  }

  private fun clearCustomGeometrySources() {
    customGeometrySources.values.forEach { it.close() }
    customGeometrySources.clear()
  }

  internal fun releaseDetachedCustomGeometrySources() {
    memScoped {
      val iterator = customGeometrySources.iterator()
      while (iterator.hasNext()) {
        val entry = iterator.next()
        val outType = alloc<UIntVar>()
        val outFound = alloc<BooleanVar>()
        val status =
          mln_map_get_style_source_type(
            state.requireLive(),
            CoreStructs.stringView(entry.key, this),
            outType.ptr,
            outFound.ptr,
          )
        if (status != org.maplibre.nativeffi.error.MaplibreStatus.OK.nativeCode) continue
        if (!outFound.value || SourceType.fromNative(outType.value) != SourceType.CUSTOM_VECTOR) {
          entry.value.close()
          iterator.remove()
        }
      }
    }
  }

  public fun addVectorSourceUrl(sourceId: String, url: String, options: TileSourceOptions? = null) {
    memScoped {
      Status.check(
        mln_map_add_vector_source_url(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.tileSourceOptions(options, this),
        )
      )
    }
  }

  public fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions? = null,
  ) {
    val tileSnapshot = tiles.toList()
    memScoped {
      Status.check(
        mln_map_add_vector_source_tiles(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          StyleStructs.stringViewArray(tileSnapshot, this),
          tileSnapshot.size.toULong(),
          StyleStructs.tileSourceOptions(options, this),
        )
      )
    }
  }

  public fun addRasterSourceUrl(sourceId: String, url: String, options: TileSourceOptions? = null) {
    memScoped {
      Status.check(
        mln_map_add_raster_source_url(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.tileSourceOptions(options, this),
        )
      )
    }
  }

  public fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions? = null,
  ) {
    val tileSnapshot = tiles.toList()
    memScoped {
      Status.check(
        mln_map_add_raster_source_tiles(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          StyleStructs.stringViewArray(tileSnapshot, this),
          tileSnapshot.size.toULong(),
          StyleStructs.tileSourceOptions(options, this),
        )
      )
    }
  }

  public fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions? = null,
  ) {
    memScoped {
      Status.check(
        mln_map_add_raster_dem_source_url(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.tileSourceOptions(options, this),
        )
      )
    }
  }

  public fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions? = null,
  ) {
    val tileSnapshot = tiles.toList()
    memScoped {
      Status.check(
        mln_map_add_raster_dem_source_tiles(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          StyleStructs.stringViewArray(tileSnapshot, this),
          tileSnapshot.size.toULong(),
          StyleStructs.tileSourceOptions(options, this),
        )
      )
    }
  }

  public fun setStyleImage(imageId: String, image: PremultipliedRgba8Image) {
    setStyleImage(imageId, image, StyleImageOptions())
  }

  public fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ) {
    memScoped {
      Status.check(
        mln_map_set_style_image(
          state.requireLive(),
          CoreStructs.stringView(imageId, this),
          StyleStructs.premultipliedRgba8Image(image, this),
          StyleStructs.styleImageOptions(options, this),
        )
      )
    }
  }

  public fun removeStyleImage(imageId: String): Boolean = memScoped {
    val outRemoved = alloc<BooleanVar>()
    Status.check(
      mln_map_remove_style_image(
        state.requireLive(),
        CoreStructs.stringView(imageId, this),
        outRemoved.ptr,
      )
    )
    outRemoved.value
  }

  public fun styleImageExists(imageId: String): Boolean = memScoped {
    val outExists = alloc<BooleanVar>()
    Status.check(
      mln_map_style_image_exists(
        state.requireLive(),
        CoreStructs.stringView(imageId, this),
        outExists.ptr,
      )
    )
    outExists.value
  }

  public fun styleImageInfo(imageId: String): StyleImageInfo? = memScoped {
    val outInfo = mln_style_image_info_default().getPointer(this)
    val outFound = alloc<BooleanVar>()
    Status.check(
      mln_map_get_style_image_info(
        state.requireLive(),
        CoreStructs.stringView(imageId, this),
        outInfo,
        outFound.ptr,
      )
    )
    if (outFound.value) StyleStructs.styleImageInfo(outInfo.pointed) else null
  }

  public fun copyStyleImagePremultipliedRgba8(imageId: String): StyleImage? = memScoped {
    val info = styleImageInfo(imageId) ?: return@memScoped null
    val outPixels =
      allocArray<UByteVar>(checkedInt(info.byteLength.toULong(), "style image byte length"))
    val outByteLength = alloc<ULongVar>()
    val outFound = alloc<BooleanVar>()
    Status.check(
      mln_map_copy_style_image_premultiplied_rgba8(
        state.requireLive(),
        CoreStructs.stringView(imageId, this),
        outPixels,
        info.byteLength.toULong(),
        outByteLength.ptr,
        outFound.ptr,
      )
    )
    if (!outFound.value) return@memScoped null
    StyleImage(
      PremultipliedRgba8Image(
        info.width,
        info.height,
        info.stride,
        outPixels
          .reinterpret<kotlinx.cinterop.ByteVar>()
          .readBytes(checkedInt(outByteLength.value, "style image copied byte length")),
      ),
      info.pixelRatio,
      info.sdf,
    )
  }

  public fun addImageSourceUrl(sourceId: String, coordinates: List<LatLng>, url: String) {
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      Status.check(
        mln_map_add_image_source_url(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
          CoreStructs.stringView(url, this),
        )
      )
    }
  }

  public fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ) {
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      Status.check(
        mln_map_add_image_source_image(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
          StyleStructs.premultipliedRgba8Image(image, this),
        )
      )
    }
  }

  public fun setImageSourceUrl(sourceId: String, url: String) {
    memScoped {
      Status.check(
        mln_map_set_image_source_url(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
        )
      )
    }
  }

  public fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image) {
    memScoped {
      Status.check(
        mln_map_set_image_source_image(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          StyleStructs.premultipliedRgba8Image(image, this),
        )
      )
    }
  }

  public fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>) {
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      Status.check(
        mln_map_set_image_source_coordinates(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
        )
      )
    }
  }

  public fun imageSourceCoordinates(sourceId: String): List<LatLng>? = memScoped {
    val outCoordinates = allocArray<mln_lat_lng>(4)
    val outCoordinateCount = alloc<ULongVar>()
    val outFound = alloc<BooleanVar>()
    Status.check(
      mln_map_get_image_source_coordinates(
        state.requireLive(),
        CoreStructs.stringView(sourceId, this),
        outCoordinates,
        4UL,
        outCoordinateCount.ptr,
        outFound.ptr,
      )
    )
    if (outFound.value)
      CoreStructs.latLngArray(
        outCoordinates,
        checkedInt(outCoordinateCount.value, "image source coordinate count"),
      )
    else null
  }

  private fun copyStyleSourceAttribution(sourceId: String, info: mln_style_source_info): String? {
    if (!info.has_attribution) return null
    if (info.attribution_size == 0UL) return ""
    return memScoped {
      val outAttribution =
        allocArray<ByteVar>(checkedInt(info.attribution_size, "style source attribution size"))
      val outAttributionSize = alloc<ULongVar>()
      val outFound = alloc<BooleanVar>()
      Status.check(
        mln_map_copy_style_source_attribution(
          state.requireLive(),
          CoreStructs.stringView(sourceId, this),
          outAttribution,
          info.attribution_size,
          outAttributionSize.ptr,
          outFound.ptr,
        )
      )
      if (outFound.value)
        outAttribution
          .readBytes(checkedInt(outAttributionSize.value, "style source copied attribution size"))
          .decodeToString()
      else null
    }
  }

  public fun addStyleLayerJson(layerJson: JsonValue) {
    addStyleLayerJson(layerJson, "")
  }

  public fun addStyleLayerJson(layerJson: JsonValue, beforeLayerId: String) {
    memScoped {
      Status.check(
        mln_map_add_style_layer_json(
          state.requireLive(),
          ValueStructs.jsonValue(layerJson, this),
          CoreStructs.stringView(beforeLayerId, this),
        )
      )
    }
  }

  public fun addHillshadeLayer(layerId: String, sourceId: String) {
    addHillshadeLayer(layerId, sourceId, "")
  }

  public fun addHillshadeLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    memScoped {
      Status.check(
        mln_map_add_hillshade_layer(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(beforeLayerId, this),
        )
      )
    }
  }

  public fun addColorReliefLayer(layerId: String, sourceId: String) {
    addColorReliefLayer(layerId, sourceId, "")
  }

  public fun addColorReliefLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    memScoped {
      Status.check(
        mln_map_add_color_relief_layer(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(beforeLayerId, this),
        )
      )
    }
  }

  public fun addLocationIndicatorLayer(layerId: String) {
    addLocationIndicatorLayer(layerId, "")
  }

  public fun addLocationIndicatorLayer(layerId: String, beforeLayerId: String) {
    memScoped {
      Status.check(
        mln_map_add_location_indicator_layer(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(beforeLayerId, this),
        )
      )
    }
  }

  public fun setLocationIndicatorLocation(layerId: String, coordinate: LatLng, altitude: Double) {
    memScoped {
      Status.check(
        mln_map_set_location_indicator_location(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          CoreStructs.latLng(coordinate),
          altitude,
        )
      )
    }
  }

  public fun setLocationIndicatorBearing(layerId: String, bearing: Double) {
    memScoped {
      Status.check(
        mln_map_set_location_indicator_bearing(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          bearing,
        )
      )
    }
  }

  public fun setLocationIndicatorAccuracyRadius(layerId: String, radius: Double) {
    memScoped {
      Status.check(
        mln_map_set_location_indicator_accuracy_radius(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          radius,
        )
      )
    }
  }

  public fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ) {
    memScoped {
      Status.check(
        mln_map_set_location_indicator_image_name(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          imageKind.nativeValue.toUInt(),
          CoreStructs.stringView(imageId, this),
        )
      )
    }
  }

  public fun removeStyleLayer(layerId: String): Boolean = memScoped {
    val outRemoved = alloc<BooleanVar>()
    Status.check(
      mln_map_remove_style_layer(
        state.requireLive(),
        CoreStructs.stringView(layerId, this),
        outRemoved.ptr,
      )
    )
    outRemoved.value
  }

  public fun styleLayerExists(layerId: String): Boolean = memScoped {
    val outExists = alloc<BooleanVar>()
    Status.check(
      mln_map_style_layer_exists(
        state.requireLive(),
        CoreStructs.stringView(layerId, this),
        outExists.ptr,
      )
    )
    outExists.value
  }

  public fun styleLayerType(layerId: String): String? = memScoped {
    val outType = alloc<org.maplibre.nativeffi.internal.c.mln_string_view>()
    val outFound = alloc<BooleanVar>()
    Status.check(
      mln_map_get_style_layer_type(
        state.requireLive(),
        CoreStructs.stringView(layerId, this),
        outType.ptr,
        outFound.ptr,
      )
    )
    if (outFound.value) CoreStructs.stringView(outType) else null
  }

  public fun styleLayerIds(): List<String> = memScoped {
    val outList = alloc<CPointerVarOf<CPointer<mln_style_id_list>>>()
    outList.value = null
    Status.check(mln_map_list_style_layer_ids(state.requireLive(), outList.ptr))
    StyleStructs.styleIdList(requireNotNull(outList.value))
  }

  public fun moveStyleLayer(layerId: String, beforeLayerId: String = "") {
    memScoped {
      Status.check(
        mln_map_move_style_layer(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(beforeLayerId, this),
        )
      )
    }
  }

  public fun styleLayerJson(layerId: String): JsonValue? = memScoped {
    val outLayer = alloc<CPointerVarOf<CPointer<mln_json_snapshot>>>()
    val outFound = alloc<BooleanVar>()
    outLayer.value = null
    Status.check(
      mln_map_get_style_layer_json(
        state.requireLive(),
        CoreStructs.stringView(layerId, this),
        outLayer.ptr,
        outFound.ptr,
      )
    )
    if (outFound.value) ValueStructs.jsonSnapshotHandle(outLayer.value) else null
  }

  public fun setStyleLightJson(lightJson: JsonValue) {
    memScoped {
      Status.check(
        mln_map_set_style_light_json(state.requireLive(), ValueStructs.jsonValue(lightJson, this))
      )
    }
  }

  public fun setStyleLightProperty(propertyName: String, value: JsonValue) {
    memScoped {
      Status.check(
        mln_map_set_style_light_property(
          state.requireLive(),
          CoreStructs.stringView(propertyName, this),
          ValueStructs.jsonValue(value, this),
        )
      )
    }
  }

  public fun styleLightProperty(propertyName: String): JsonValue? = memScoped {
    val outValue = alloc<CPointerVarOf<CPointer<mln_json_snapshot>>>()
    outValue.value = null
    Status.check(
      mln_map_get_style_light_property(
        state.requireLive(),
        CoreStructs.stringView(propertyName, this),
        outValue.ptr,
      )
    )
    ValueStructs.jsonSnapshotHandle(outValue.value)
  }

  public fun setLayerProperty(layerId: String, propertyName: String, value: JsonValue) {
    memScoped {
      Status.check(
        mln_map_set_layer_property(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(propertyName, this),
          ValueStructs.jsonValue(value, this),
        )
      )
    }
  }

  public fun layerProperty(layerId: String, propertyName: String): JsonValue? = memScoped {
    val outValue = alloc<CPointerVarOf<CPointer<mln_json_snapshot>>>()
    outValue.value = null
    Status.check(
      mln_map_get_layer_property(
        state.requireLive(),
        CoreStructs.stringView(layerId, this),
        CoreStructs.stringView(propertyName, this),
        outValue.ptr,
      )
    )
    ValueStructs.jsonSnapshotHandle(outValue.value)
  }

  public fun setLayerFilter(layerId: String, filter: JsonValue) {
    memScoped {
      Status.check(
        mln_map_set_layer_filter(
          state.requireLive(),
          CoreStructs.stringView(layerId, this),
          ValueStructs.jsonValue(filter, this),
        )
      )
    }
  }

  public fun clearLayerFilter(layerId: String) {
    memScoped {
      Status.check(
        mln_map_set_layer_filter(state.requireLive(), CoreStructs.stringView(layerId, this), null)
      )
    }
  }

  public fun layerFilter(layerId: String): JsonValue? = memScoped {
    val outFilter = alloc<CPointerVarOf<CPointer<mln_json_snapshot>>>()
    outFilter.value = null
    Status.check(
      mln_map_get_layer_filter(
        state.requireLive(),
        CoreStructs.stringView(layerId, this),
        outFilter.ptr,
      )
    )
    ValueStructs.jsonSnapshotHandle(outFilter.value)
  }

  public fun requestRepaint() {
    Status.check(mln_map_request_repaint(state.requireLive()))
  }

  public fun requestStillImage() {
    Status.check(mln_map_request_still_image(state.requireLive()))
  }

  public var debugOptions: Set<DebugOption>
    get() = memScoped {
      val outOptions = alloc<UIntVar>()
      Status.check(mln_map_get_debug_options(state.requireLive(), outOptions.ptr))
      DebugOption.entries.filterTo(mutableSetOf()) {
        (outOptions.value.toInt() and it.nativeMask) != 0
      }
    }
    set(options) {
      val mask = options.fold(0) { acc, option -> acc or option.nativeMask }
      Status.check(mln_map_set_debug_options(state.requireLive(), mask.toUInt()))
    }

  public var isRenderingStatsViewEnabled: Boolean
    get() = memScoped {
      val outEnabled = alloc<BooleanVar>()
      Status.check(mln_map_get_rendering_stats_view_enabled(state.requireLive(), outEnabled.ptr))
      outEnabled.value
    }
    set(enabled) {
      Status.check(mln_map_set_rendering_stats_view_enabled(state.requireLive(), enabled))
    }

  public val isFullyLoaded: Boolean
    get() = memScoped {
      val outLoaded = alloc<BooleanVar>()
      Status.check(mln_map_is_fully_loaded(state.requireLive(), outLoaded.ptr))
      outLoaded.value
    }

  public fun dumpDebugLogs() {
    Status.check(mln_map_dump_debug_logs(state.requireLive()))
  }

  public var viewportOptions: ViewportOptions
    get() = memScoped {
      val outOptions = mln_map_viewport_options_default().getPointer(this)
      Status.check(mln_map_get_viewport_options(state.requireLive(), outOptions))
      MapStructs.viewportOptions(outOptions.pointed)
    }
    set(options) {
      memScoped {
        Status.check(
          mln_map_set_viewport_options(
            state.requireLive(),
            MapStructs.viewportOptions(options, this),
          )
        )
      }
    }

  public var tileOptions: TileOptions
    get() = memScoped {
      val outOptions = mln_map_tile_options_default().getPointer(this)
      Status.check(mln_map_get_tile_options(state.requireLive(), outOptions))
      MapStructs.tileOptions(outOptions.pointed)
    }
    set(options) {
      memScoped {
        Status.check(
          mln_map_set_tile_options(state.requireLive(), MapStructs.tileOptions(options, this))
        )
      }
    }

  public val camera: CameraOptions
    get() = memScoped {
      val outCamera = mln_camera_options_default().getPointer(this)
      Status.check(mln_map_get_camera(state.requireLive(), outCamera))
      MapStructs.cameraOptions(outCamera.pointed)
    }

  public fun jumpTo(camera: CameraOptions) {
    memScoped {
      Status.check(mln_map_jump_to(state.requireLive(), MapStructs.cameraOptions(camera, this)))
    }
  }

  public fun easeTo(camera: CameraOptions, animation: AnimationOptions? = null) {
    memScoped {
      Status.check(
        mln_map_ease_to(
          state.requireLive(),
          MapStructs.cameraOptions(camera, this),
          animation?.let { MapStructs.animationOptions(it, this) },
        )
      )
    }
  }

  public fun flyTo(camera: CameraOptions, animation: AnimationOptions? = null) {
    memScoped {
      Status.check(
        mln_map_fly_to(
          state.requireLive(),
          MapStructs.cameraOptions(camera, this),
          animation?.let { MapStructs.animationOptions(it, this) },
        )
      )
    }
  }

  public fun moveBy(deltaX: Double, deltaY: Double) {
    Status.check(mln_map_move_by(state.requireLive(), deltaX, deltaY))
  }

  public fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions? = null) {
    memScoped {
      Status.check(
        mln_map_move_by_animated(
          state.requireLive(),
          deltaX,
          deltaY,
          animation?.let { MapStructs.animationOptions(it, this) },
        )
      )
    }
  }

  public fun scaleBy(scale: Double, anchor: ScreenPoint? = null) {
    Status.check(
      mln_map_scale_by(state.requireLive(), scale, anchor?.let { CoreStructs.screenPoint(it) })
    )
  }

  public fun scaleByAnimated(scale: Double) {
    scaleByAnimated(scale, null, null)
  }

  public fun scaleByAnimated(scale: Double, anchor: ScreenPoint?) {
    scaleByAnimated(scale, anchor, null)
  }

  public fun scaleByAnimated(scale: Double, animation: AnimationOptions?) {
    scaleByAnimated(scale, null, animation)
  }

  public fun scaleByAnimated(scale: Double, anchor: ScreenPoint?, animation: AnimationOptions?) {
    memScoped {
      Status.check(
        mln_map_scale_by_animated(
          state.requireLive(),
          scale,
          anchor?.let { CoreStructs.screenPoint(it) },
          animation?.let { MapStructs.animationOptions(it, this) },
        )
      )
    }
  }

  public fun rotateBy(first: ScreenPoint, second: ScreenPoint) {
    Status.check(
      mln_map_rotate_by(
        state.requireLive(),
        CoreStructs.screenPoint(first),
        CoreStructs.screenPoint(second),
      )
    )
  }

  public fun rotateByAnimated(
    first: ScreenPoint,
    second: ScreenPoint,
    animation: AnimationOptions? = null,
  ) {
    memScoped {
      Status.check(
        mln_map_rotate_by_animated(
          state.requireLive(),
          CoreStructs.screenPoint(first),
          CoreStructs.screenPoint(second),
          animation?.let { MapStructs.animationOptions(it, this) },
        )
      )
    }
  }

  public fun pitchBy(pitch: Double) {
    Status.check(mln_map_pitch_by(state.requireLive(), pitch))
  }

  public fun pitchByAnimated(pitch: Double, animation: AnimationOptions? = null) {
    memScoped {
      Status.check(
        mln_map_pitch_by_animated(
          state.requireLive(),
          pitch,
          animation?.let { MapStructs.animationOptions(it, this) },
        )
      )
    }
  }

  public fun cancelTransitions() {
    Status.check(mln_map_cancel_transitions(state.requireLive()))
  }

  public fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions? = null,
  ): CameraOptions = memScoped {
    val outCamera = mln_camera_options_default().getPointer(this)
    Status.check(
      mln_map_camera_for_lat_lng_bounds(
        state.requireLive(),
        CoreStructs.latLngBounds(bounds),
        fitOptions?.let { MapStructs.cameraFitOptions(it, this) },
        outCamera,
      )
    )
    MapStructs.cameraOptions(outCamera.pointed)
  }

  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions? = null,
  ): CameraOptions = memScoped {
    val coordinateSnapshot = coordinates.toList()
    val outCamera = mln_camera_options_default().getPointer(this)
    Status.check(
      mln_map_camera_for_lat_lngs(
        state.requireLive(),
        CoreStructs.latLngArray(coordinateSnapshot, this),
        coordinateSnapshot.size.toULong(),
        fitOptions?.let { MapStructs.cameraFitOptions(it, this) },
        outCamera,
      )
    )
    MapStructs.cameraOptions(outCamera.pointed)
  }

  public fun cameraForGeometry(
    geometry: Geometry,
    fitOptions: CameraFitOptions? = null,
  ): CameraOptions = memScoped {
    val outCamera = mln_camera_options_default().getPointer(this)
    Status.check(
      mln_map_camera_for_geometry(
        state.requireLive(),
        ValueStructs.geometry(geometry, this),
        fitOptions?.let { MapStructs.cameraFitOptions(it, this) },
        outCamera,
      )
    )
    MapStructs.cameraOptions(outCamera.pointed)
  }

  public fun latLngBoundsForCamera(camera: CameraOptions): LatLngBounds = memScoped {
    val outBounds = alloc<mln_lat_lng_bounds>()
    Status.check(
      mln_map_lat_lng_bounds_for_camera(
        state.requireLive(),
        MapStructs.cameraOptions(camera, this),
        outBounds.ptr,
      )
    )
    CoreStructs.latLngBounds(outBounds)
  }

  public fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): LatLngBounds = memScoped {
    val outBounds = alloc<mln_lat_lng_bounds>()
    Status.check(
      mln_map_lat_lng_bounds_for_camera_unwrapped(
        state.requireLive(),
        MapStructs.cameraOptions(camera, this),
        outBounds.ptr,
      )
    )
    CoreStructs.latLngBounds(outBounds)
  }

  public var bounds: BoundOptions
    get() = memScoped {
      val outOptions = mln_bound_options_default().getPointer(this)
      Status.check(mln_map_get_bounds(state.requireLive(), outOptions))
      MapStructs.boundOptions(outOptions.pointed)
    }
    set(options) {
      memScoped {
        Status.check(
          mln_map_set_bounds(state.requireLive(), MapStructs.boundOptions(options, this))
        )
      }
    }

  public var freeCameraOptions: FreeCameraOptions
    get() = memScoped {
      val outOptions = mln_free_camera_options_default().getPointer(this)
      Status.check(mln_map_get_free_camera_options(state.requireLive(), outOptions))
      MapStructs.freeCameraOptions(outOptions.pointed)
    }
    set(options) {
      memScoped {
        Status.check(
          mln_map_set_free_camera_options(
            state.requireLive(),
            MapStructs.freeCameraOptions(options, this),
          )
        )
      }
    }

  public var projectionMode: ProjectionModeOptions
    get() = memScoped {
      val outMode = mln_projection_mode_default().getPointer(this)
      Status.check(mln_map_get_projection_mode(state.requireLive(), outMode))
      MapStructs.projectionModeOptions(outMode.pointed)
    }
    set(mode) {
      memScoped {
        Status.check(
          mln_map_set_projection_mode(
            state.requireLive(),
            MapStructs.projectionModeOptions(mode, this),
          )
        )
      }
    }

  public fun pixelForLatLng(coordinate: LatLng): ScreenPoint = memScoped {
    val outPoint = alloc<mln_screen_point>()
    Status.check(
      mln_map_pixel_for_lat_lng(state.requireLive(), CoreStructs.latLng(coordinate), outPoint.ptr)
    )
    CoreStructs.screenPoint(outPoint)
  }

  public fun latLngForPixel(point: ScreenPoint): LatLng = memScoped {
    val outCoordinate = alloc<mln_lat_lng>()
    Status.check(
      mln_map_lat_lng_for_pixel(
        state.requireLive(),
        CoreStructs.screenPoint(point),
        outCoordinate.ptr,
      )
    )
    CoreStructs.latLng(outCoordinate)
  }

  public fun pixelsForLatLngs(coordinates: List<LatLng>): List<ScreenPoint> = memScoped {
    val coordinateSnapshot = coordinates.toList()
    if (coordinateSnapshot.isEmpty()) return@memScoped emptyList()
    val outPoints = allocArray<mln_screen_point>(coordinateSnapshot.size)
    Status.check(
      mln_map_pixels_for_lat_lngs(
        state.requireLive(),
        CoreStructs.latLngArray(coordinateSnapshot, this),
        coordinateSnapshot.size.toULong(),
        outPoints,
      )
    )
    CoreStructs.screenPointArray(outPoints, coordinateSnapshot.size)
  }

  public fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng> = memScoped {
    val pointSnapshot = points.toList()
    if (pointSnapshot.isEmpty()) return@memScoped emptyList()
    val outCoordinates = allocArray<mln_lat_lng>(pointSnapshot.size)
    Status.check(
      mln_map_lat_lngs_for_pixels(
        state.requireLive(),
        CoreStructs.screenPointArray(pointSnapshot, this),
        pointSnapshot.size.toULong(),
        outCoordinates,
      )
    )
    CoreStructs.latLngArray(outCoordinates, pointSnapshot.size)
  }

  public fun attachMetalOwnedTexture(descriptor: MetalOwnedTextureDescriptor): RenderSessionHandle =
    RenderSessionHandle.attachMetalOwnedTexture(this, descriptor)

  public fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachMetalBorrowedTexture(this, descriptor)

  public fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachVulkanOwnedTexture(this, descriptor)

  public fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachVulkanBorrowedTexture(this, descriptor)

  public fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachOpenGLOwnedTexture(this, descriptor)

  public fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): RenderSessionHandle = RenderSessionHandle.attachOpenGLBorrowedTexture(this, descriptor)

  public fun attachMetalSurface(descriptor: MetalSurfaceDescriptor): RenderSessionHandle =
    RenderSessionHandle.attachMetalSurface(this, descriptor)

  public fun attachVulkanSurface(descriptor: VulkanSurfaceDescriptor): RenderSessionHandle =
    RenderSessionHandle.attachVulkanSurface(this, descriptor)

  public fun attachOpenGLSurface(descriptor: OpenGLSurfaceDescriptor): RenderSessionHandle =
    RenderSessionHandle.attachOpenGLSurface(this, descriptor)

  public fun createProjection(): MapProjectionHandle = MapProjectionHandle.create(this)

  override fun close() {
    state.closeOnce(::mln_map_destroy) {
      clearCustomGeometrySources()
      runtime.unregisterMap(this)
    }
  }

  public val isClosed: Boolean
    get() = state.isReleased()

  public fun runtime(): RuntimeHandle = runtime

  internal fun nativeHandle(): CPointer<mln_map> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()

  private fun checkedInt(value: ULong, name: String): Int {
    require(value <= Int.MAX_VALUE.toULong()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  public companion object {
    public fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle = memScoped {
      val nativeOptions = alloc<mln_map_options>()
      mln_map_options_default().place(nativeOptions.ptr)
      options.width?.let { nativeOptions.width = it.toUInt() }
      options.height?.let { nativeOptions.height = it.toUInt() }
      options.scaleFactor?.let { nativeOptions.scale_factor = it }
      options.mapMode?.let { nativeOptions.map_mode = it.nativeValue.toUInt() }

      val outMap = alloc<CPointerVarOf<CPointer<mln_map>>>()
      outMap.value = null
      Status.check(mln_map_create(runtime.nativeHandle(), nativeOptions.ptr, outMap.ptr))
      val map = MapHandle(runtime, requireNotNull(outMap.value) { "mln_map_create returned null" })
      runtime.registerMap(map)
      map
    }
  }
}
