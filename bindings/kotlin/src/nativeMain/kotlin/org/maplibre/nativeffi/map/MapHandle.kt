package org.maplibre.nativeffi.map

import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CValue
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.MemScope
import kotlinx.cinterop.UByteVar
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.readValue
import kotlinx.cinterop.reinterpret
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.value
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraSnapshot
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.c.MLN_STYLE_LAYER_INFO_SOURCE_ID
import org.maplibre.nativeffi.internal.c.MLN_STYLE_LAYER_INFO_SOURCE_LAYER
import org.maplibre.nativeffi.internal.c.MLN_STYLE_SOURCE_INFO_TILEJSON
import org.maplibre.nativeffi.internal.c.MLN_STYLE_SOURCE_INFO_URL
import org.maplibre.nativeffi.internal.c.mln_buffer_view
import org.maplibre.nativeffi.internal.c.mln_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_camera_query_result
import org.maplibre.nativeffi.internal.c.mln_camera_update_default
import org.maplibre.nativeffi.internal.c.mln_image_stretch
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_logical_extent
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
import org.maplibre.nativeffi.internal.c.mln_map_camera_query_start
import org.maplibre.nativeffi.internal.c.mln_map_camera_query_take_result
import org.maplibre.nativeffi.internal.c.mln_map_camera_snapshot_get
import org.maplibre.nativeffi.internal.c.mln_map_close_start
import org.maplibre.nativeffi.internal.c.mln_map_copy_layer_source_id_start
import org.maplibre.nativeffi.internal.c.mln_map_copy_layer_source_id_take_result
import org.maplibre.nativeffi.internal.c.mln_map_copy_layer_source_layer_start
import org.maplibre.nativeffi.internal.c.mln_map_copy_layer_source_layer_take_result
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_image_premultiplied_rgba8_start
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_image_premultiplied_rgba8_take_result
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_image_stretches_start
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_image_stretches_take_result
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_source_attribution_start
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_source_attribution_take_result
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_source_url_start
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_source_url_take_result
import org.maplibre.nativeffi.internal.c.mln_map_create_start
import org.maplibre.nativeffi.internal.c.mln_map_create_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_image_source_coordinates_start
import org.maplibre.nativeffi.internal.c.mln_map_get_image_source_coordinates_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_layer_filter_start
import org.maplibre.nativeffi.internal.c.mln_map_get_layer_filter_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_layer_property_start
import org.maplibre.nativeffi.internal.c.mln_map_get_layer_property_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_style_image_info_start
import org.maplibre.nativeffi.internal.c.mln_map_get_style_image_info_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_style_layer_info_start
import org.maplibre.nativeffi.internal.c.mln_map_get_style_layer_info_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_style_layer_json_start
import org.maplibre.nativeffi.internal.c.mln_map_get_style_layer_json_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_style_light_property_start
import org.maplibre.nativeffi.internal.c.mln_map_get_style_light_property_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_style_source_info_start
import org.maplibre.nativeffi.internal.c.mln_map_get_style_source_info_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_style_source_tile_urls_start
import org.maplibre.nativeffi.internal.c.mln_map_get_style_source_tile_urls_take_result
import org.maplibre.nativeffi.internal.c.mln_map_get_style_transition_options_start
import org.maplibre.nativeffi.internal.c.mln_map_get_style_transition_options_take_result
import org.maplibre.nativeffi.internal.c.mln_map_invalidate_custom_geometry_source_region
import org.maplibre.nativeffi.internal.c.mln_map_invalidate_custom_geometry_source_tile
import org.maplibre.nativeffi.internal.c.mln_map_list_style_layer_ids_start
import org.maplibre.nativeffi.internal.c.mln_map_list_style_layer_ids_take_result
import org.maplibre.nativeffi.internal.c.mln_map_list_style_source_ids_start
import org.maplibre.nativeffi.internal.c.mln_map_list_style_source_ids_take_result
import org.maplibre.nativeffi.internal.c.mln_map_loaded_style_json_start
import org.maplibre.nativeffi.internal.c.mln_map_loaded_style_json_take_result
import org.maplibre.nativeffi.internal.c.mln_map_move_style_layer
import org.maplibre.nativeffi.internal.c.mln_map_options
import org.maplibre.nativeffi.internal.c.mln_map_options_default
import org.maplibre.nativeffi.internal.c.mln_map_projection_create_start
import org.maplibre.nativeffi.internal.c.mln_map_projection_create_take_result
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_image
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_layer
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_source
import org.maplibre.nativeffi.internal.c.mln_map_request_repaint
import org.maplibre.nativeffi.internal.c.mln_map_request_still_image_start
import org.maplibre.nativeffi.internal.c.mln_map_resize
import org.maplibre.nativeffi.internal.c.mln_map_set_bounds
import org.maplibre.nativeffi.internal.c.mln_map_set_custom_geometry_source_tile_data
import org.maplibre.nativeffi.internal.c.mln_map_set_debug_options
import org.maplibre.nativeffi.internal.c.mln_map_set_event_mask
import org.maplibre.nativeffi.internal.c.mln_map_set_free_camera_options
import org.maplibre.nativeffi.internal.c.mln_map_set_geojson_source_data
import org.maplibre.nativeffi.internal.c.mln_map_set_geojson_source_url
import org.maplibre.nativeffi.internal.c.mln_map_set_image_source_coordinates
import org.maplibre.nativeffi.internal.c.mln_map_set_image_source_image
import org.maplibre.nativeffi.internal.c.mln_map_set_image_source_url
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_filter
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_max_zoom
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_min_zoom
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_property
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_source_id
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_source_layer
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_visibility
import org.maplibre.nativeffi.internal.c.mln_map_set_location_indicator_accuracy_radius
import org.maplibre.nativeffi.internal.c.mln_map_set_location_indicator_bearing
import org.maplibre.nativeffi.internal.c.mln_map_set_location_indicator_image_name
import org.maplibre.nativeffi.internal.c.mln_map_set_location_indicator_location
import org.maplibre.nativeffi.internal.c.mln_map_set_rendering_stats_view_enabled
import org.maplibre.nativeffi.internal.c.mln_map_set_style_image
import org.maplibre.nativeffi.internal.c.mln_map_set_style_json
import org.maplibre.nativeffi.internal.c.mln_map_set_style_light_json
import org.maplibre.nativeffi.internal.c.mln_map_set_style_light_property
import org.maplibre.nativeffi.internal.c.mln_map_set_style_transition_options
import org.maplibre.nativeffi.internal.c.mln_map_set_style_url
import org.maplibre.nativeffi.internal.c.mln_map_set_tile_options
import org.maplibre.nativeffi.internal.c.mln_map_set_viewport_options
import org.maplibre.nativeffi.internal.c.mln_map_snapshot
import org.maplibre.nativeffi.internal.c.mln_map_snapshot_get
import org.maplibre.nativeffi.internal.c.mln_map_style_url_start
import org.maplibre.nativeffi.internal.c.mln_map_style_url_take_result
import org.maplibre.nativeffi.internal.c.mln_map_update_camera
import org.maplibre.nativeffi.internal.c.mln_operation_release
import org.maplibre.nativeffi.internal.c.mln_style_image_info_default
import org.maplibre.nativeffi.internal.c.mln_style_layer_info
import org.maplibre.nativeffi.internal.c.mln_style_source_info
import org.maplibre.nativeffi.internal.c.mln_style_transition_options_default
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.internal.lifecycle.asHandle
import org.maplibre.nativeffi.internal.lifecycle.mapHandle
import org.maplibre.nativeffi.internal.lifecycle.mapProjectionHandle
import org.maplibre.nativeffi.internal.lifecycle.ownedBufferHandle
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.lifecycle.styleIdListHandle
import org.maplibre.nativeffi.internal.lifecycle.styleStringListHandle
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ByteStructs
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.MapStructs
import org.maplibre.nativeffi.internal.struct.StyleStructs
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.render.RenderSessionAttachOptions
import org.maplibre.nativeffi.render.RenderSessionAttachment
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.startOperation
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.ImageStretch
import org.maplibre.nativeffi.style.LayerInfo
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Owned any-thread native map handle. */
@OptIn(ExperimentalForeignApi::class)
public actual class MapHandle
private constructor(
  private val runtime: RuntimeHandle,
  handle: NativeMap,
  cachedEventMask: RuntimeEventMask,
) {
  private val runtimeRetention = runtime.retainChild("MapHandle")
  private val state = HandleState("MapHandle", handle, runtime)
  private var cachedEventMask =
    RuntimeEventMask(cachedEventMask.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)
  private val customGeometrySources =
    CustomGeometrySourceRegistry<CustomGeometrySourceState> { it.close() }

  public actual var eventMask: RuntimeEventMask
    get() = cachedEventMask
    set(value) {
      command { outCommandId ->
        Status.check(
          mln_map_set_event_mask(
            state.requireLive().rawHandleValue,
            value.nativeValue.toULong(),
            outCommandId,
          )
        )
      }
      cachedEventMask =
        RuntimeEventMask(value.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)
    }

  public actual fun setStyleUrl(url: String): ULong = command { outCommandId ->
    MemoryUtil.requireValidCString(url)
    Status.check(mln_map_set_style_url(state.requireLive().rawHandleValue, url, outCommandId))
  }

  public actual fun setStyleJson(json: ByteArray): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_set_style_json(
          state.requireLive().rawHandleValue,
          ByteStructs.bufferView(json, this),
          outCommandId,
        )
      )
    }
  }

  public actual suspend fun loadedStyleJson(): ByteArray =
    takeStyleBuffer(::mln_map_loaded_style_json_start, ::mln_map_loaded_style_json_take_result)

  public actual suspend fun styleUrl(): String =
    takeStyleBuffer(::mln_map_style_url_start, ::mln_map_style_url_take_result).decodeToString()

  public actual fun addStyleSourceJson(sourceId: String, sourceJson: ByteArray): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_add_style_source_json(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            ByteStructs.bufferView(sourceJson, this),
            outCommandId,
          )
        )
      }
    }

  public actual fun removeStyleSource(sourceId: String): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_remove_style_source(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          outCommandId,
        )
      )
    }
  }

  public actual suspend fun styleSourceInfo(sourceId: String): SourceInfo? = memScoped {
    val outInfo = alloc<mln_style_source_info>()
    outInfo.size = sizeOf<mln_style_source_info>().toUInt()
    val outFound = alloc<BooleanVar>()
    Status.check(
      ordered(
        { outOperation ->
          mln_map_get_style_source_info_start(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            outOperation,
          )
        },
        { operation ->
          mln_map_get_style_source_info_take_result(operation, outInfo.ptr, outFound.ptr)
        },
      )
    )
    if (!outFound.value) return@memScoped null
    val attribution = copyStyleSourceAttribution(sourceId, outInfo)
    val url = copyStyleSourceUrl(sourceId, outInfo)
    val tileUrls = copyStyleSourceTileUrls(sourceId, outInfo)
    StyleStructs.sourceInfo(outInfo, attribution, url, tileUrls)
  }

  public actual suspend fun styleSourceIds(): List<String> = memScoped {
    val outList = alloc<ULongVar>()
    outList.value = 0uL
    Status.check(
      ordered(
        { outOperation ->
          mln_map_list_style_source_ids_start(state.requireLive().rawHandleValue, outOperation)
        },
        { operation -> mln_map_list_style_source_ids_take_result(operation, outList.ptr) },
      )
    )
    StyleStructs.styleIdList(outList.value.asHandle("mln_map_list_style_ids", ::styleIdListHandle))
  }

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_add_geojson_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.geoJsonSourceOptions(options, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun addGeoJsonSourceData(
    sourceId: String,
    data: ByteArray,
    options: GeoJsonSourceOptions?,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_add_geojson_source_data(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          ByteStructs.bufferView(data, this),
          StyleStructs.geoJsonSourceOptions(options, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun setGeoJsonSourceUrl(sourceId: String, url: String): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_geojson_source_url(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            CoreStructs.stringView(url, this),
            outCommandId,
          )
        )
      }
    }

  public actual fun setGeoJsonSourceData(sourceId: String, data: ByteArray): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_geojson_source_data(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            ByteStructs.bufferView(data, this),
            outCommandId,
          )
        )
      }
    }

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): ULong =
    command { outCommandId
      -> // The release callback captures the registry rather than this map, so a map a
      // host leaks with a live source still reports as leaked.
      val registry = customGeometrySources
      val sourceState = CustomGeometrySourceState(options) { registry.remove(sourceId) }
      registry.install(sourceId, sourceState) {
        memScoped {
          Status.check(
            mln_map_add_custom_geometry_source(
              state.requireLive().rawHandleValue,
              CoreStructs.stringView(sourceId, this),
              sourceState.descriptor(),
              outCommandId,
            )
          )
        }
      }
    }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_set_custom_geometry_source_tile_data(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.canonicalTileId(tileId),
          ByteStructs.bufferView(data, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_invalidate_custom_geometry_source_tile(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.canonicalTileId(tileId),
          outCommandId,
        )
      )
    }
  }

  public actual fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_invalidate_custom_geometry_source_region(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngBounds(bounds),
          outCommandId,
        )
      )
    }
  }

  internal fun customGeometrySourceCountForTesting(): Int = customGeometrySources.size

  public actual fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_add_vector_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.tileSourceOptions(options, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    val tileSnapshot = tiles.toList()
    memScoped {
      Status.check(
        mln_map_add_vector_source_tiles(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.stringViewArray(tileSnapshot, this),
          tileSnapshot.size.toULong(),
          StyleStructs.tileSourceOptions(options, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_add_raster_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.tileSourceOptions(options, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    val tileSnapshot = tiles.toList()
    memScoped {
      Status.check(
        mln_map_add_raster_source_tiles(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.stringViewArray(tileSnapshot, this),
          tileSnapshot.size.toULong(),
          StyleStructs.tileSourceOptions(options, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_add_raster_dem_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.tileSourceOptions(options, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): ULong = command { outCommandId ->
    val tileSnapshot = tiles.toList()
    memScoped {
      Status.check(
        mln_map_add_raster_dem_source_tiles(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.stringViewArray(tileSnapshot, this),
          tileSnapshot.size.toULong(),
          StyleStructs.tileSourceOptions(options, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_set_style_image(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(imageId, this),
          StyleStructs.premultipliedRgba8Image(image, this),
          StyleStructs.styleImageOptions(options, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun removeStyleImage(imageId: String): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_remove_style_image(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(imageId, this),
          outCommandId,
        )
      )
    }
  }

  public actual suspend fun styleImageInfo(imageId: String): StyleImageInfo? = memScoped {
    val outInfo = mln_style_image_info_default().getPointer(this)
    val outFound = alloc<BooleanVar>()
    Status.check(
      ordered(
        { outOperation ->
          mln_map_get_style_image_info_start(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(imageId, this),
            outOperation,
          )
        },
        { operation -> mln_map_get_style_image_info_take_result(operation, outInfo, outFound.ptr) },
      )
    )
    if (outFound.value) StyleStructs.styleImageInfo(outInfo.pointed) else null
  }

  public actual suspend fun styleImageStretches(
    imageId: String
  ): Pair<List<ImageStretch>, List<ImageStretch>>? = memScoped {
    val handle = state.requireLive().rawHandleValue
    val outXCount = alloc<ULongVar>()
    val outYCount = alloc<ULongVar>()
    val outFound = alloc<BooleanVar>()
    Status.check(
      ordered(
        { outOperation ->
          mln_map_copy_style_image_stretches_start(
            handle,
            CoreStructs.stringView(imageId, this),
            outOperation,
          )
        },
        { operation ->
          mln_map_copy_style_image_stretches_take_result(
            operation,
            null,
            0UL,
            outXCount.ptr,
            null,
            0UL,
            outYCount.ptr,
            outFound.ptr,
          )
        },
      )
    )
    if (!outFound.value) return@memScoped null

    val xCount = checkedInt(outXCount.value, "style image stretch x count")
    val yCount = checkedInt(outYCount.value, "style image stretch y count")
    val rawX = if (xCount == 0) null else allocArray<mln_image_stretch>(xCount)
    val rawY = if (yCount == 0) null else allocArray<mln_image_stretch>(yCount)
    Status.check(
      ordered(
        { outOperation ->
          mln_map_copy_style_image_stretches_start(
            handle,
            CoreStructs.stringView(imageId, this),
            outOperation,
          )
        },
        { operation ->
          mln_map_copy_style_image_stretches_take_result(
            operation,
            rawX,
            xCount.toULong(),
            outXCount.ptr,
            rawY,
            yCount.toULong(),
            outYCount.ptr,
            outFound.ptr,
          )
        },
      )
    )
    val toList = { array: CPointer<mln_image_stretch>?, count: Int ->
      List(count) { index -> ImageStretch(array!![index].from, array[index].to) }
    }
    toList(rawX, xCount) to toList(rawY, yCount)
  }

  public actual suspend fun copyStyleImagePremultipliedRgba8(imageId: String): StyleImage? =
    memScoped {
      val info = styleImageInfo(imageId) ?: return@memScoped null
      val outPixels = alloc<ULongVar>()
      outPixels.value = 0uL
      val outFound = alloc<BooleanVar>()
      Status.check(
        ordered(
          { outOperation ->
            mln_map_copy_style_image_premultiplied_rgba8_start(
              state.requireLive().rawHandleValue,
              CoreStructs.stringView(imageId, this),
              outOperation,
            )
          },
          { operation ->
            mln_map_copy_style_image_premultiplied_rgba8_take_result(
              operation,
              outPixels.ptr,
              outFound.ptr,
            )
          },
        )
      )
      if (!outFound.value) return@memScoped null
      StyleImage(
        PremultipliedRgba8Image(
          info.width,
          info.height,
          info.stride,
          ByteStructs.ownedBuffer(outPixels.value.asHandle("mln_buffer", ::ownedBufferHandle)),
        ),
        info.pixelRatio,
        info.sdf,
      )
    }

  public actual fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): ULong = command { outCommandId ->
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      Status.check(
        mln_map_add_image_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
          CoreStructs.stringView(url, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): ULong = command { outCommandId ->
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      Status.check(
        mln_map_add_image_source_image(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
          StyleStructs.premultipliedRgba8Image(image, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_image_source_url(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            CoreStructs.stringView(url, this),
            outCommandId,
          )
        )
      }
    }

  public actual fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_image_source_image(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            StyleStructs.premultipliedRgba8Image(image, this),
            outCommandId,
          )
        )
      }
    }

  public actual fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>): ULong =
    command { outCommandId ->
      val coordinateSnapshot = coordinates.toList()
      memScoped {
        Status.check(
          mln_map_set_image_source_coordinates(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            CoreStructs.latLngArray(coordinateSnapshot, this),
            coordinateSnapshot.size.toULong(),
            outCommandId,
          )
        )
      }
    }

  public actual suspend fun imageSourceCoordinates(sourceId: String): List<LatLng>? = memScoped {
    val outCoordinates = allocArray<mln_lat_lng>(4)
    val outCoordinateCount = alloc<ULongVar>()
    val outFound = alloc<BooleanVar>()
    Status.check(
      ordered(
        { outOperation ->
          mln_map_get_image_source_coordinates_start(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            outOperation,
          )
        },
        { operation ->
          mln_map_get_image_source_coordinates_take_result(
            operation,
            outCoordinates,
            4UL,
            outCoordinateCount.ptr,
            outFound.ptr,
          )
        },
      )
    )
    if (outFound.value)
      CoreStructs.latLngArray(
        outCoordinates,
        checkedInt(outCoordinateCount.value, "image source coordinate count"),
      )
    else null
  }

  private suspend fun copyStyleSourceAttribution(
    sourceId: String,
    info: mln_style_source_info,
  ): String? {
    if (!info.has_attribution) return null
    return memScoped {
      val outAttribution = alloc<ULongVar>()
      outAttribution.value = 0uL
      val outFound = alloc<BooleanVar>()
      Status.check(
        ordered(
          { outOperation ->
            mln_map_copy_style_source_attribution_start(
              state.requireLive().rawHandleValue,
              CoreStructs.stringView(sourceId, this),
              outOperation,
            )
          },
          { operation ->
            mln_map_copy_style_source_attribution_take_result(
              operation,
              outAttribution.ptr,
              outFound.ptr,
            )
          },
        )
      )
      if (outFound.value)
        ByteStructs.ownedBuffer(outAttribution.value.asHandle("mln_buffer", ::ownedBufferHandle))
          .decodeToString()
      else null
    }
  }

  private suspend fun copyStyleSourceUrl(sourceId: String, info: mln_style_source_info): String? {
    if (info.fields and MLN_STYLE_SOURCE_INFO_URL == 0u) return null
    return memScoped {
      val outUrl = alloc<ULongVar>()
      outUrl.value = 0uL
      val outFound = alloc<BooleanVar>()
      Status.check(
        ordered(
          { outOperation ->
            mln_map_copy_style_source_url_start(
              state.requireLive().rawHandleValue,
              CoreStructs.stringView(sourceId, this),
              outOperation,
            )
          },
          { operation ->
            mln_map_copy_style_source_url_take_result(operation, outUrl.ptr, outFound.ptr)
          },
        )
      )
      if (outFound.value)
        ByteStructs.ownedBuffer(outUrl.value.asHandle("mln_buffer", ::ownedBufferHandle))
          .decodeToString()
      else null
    }
  }

  private suspend fun copyStyleSourceTileUrls(
    sourceId: String,
    info: mln_style_source_info,
  ): List<String>? {
    if (info.fields and MLN_STYLE_SOURCE_INFO_TILEJSON == 0u) return null
    return memScoped {
      val outList = alloc<ULongVar>()
      outList.value = 0uL
      val outFound = alloc<BooleanVar>()
      Status.check(
        ordered(
          { outOperation ->
            mln_map_get_style_source_tile_urls_start(
              state.requireLive().rawHandleValue,
              CoreStructs.stringView(sourceId, this),
              outOperation,
            )
          },
          { operation ->
            mln_map_get_style_source_tile_urls_take_result(operation, outList.ptr, outFound.ptr)
          },
        )
      )
      check(outFound.value) { "style source disappeared while its metadata was copied" }
      StyleStructs.styleStringList(
        outList.value.asHandle("mln_map_get_style_source_tile_urls", ::styleStringListHandle)
      )
    }
  }

  public actual fun addStyleLayerJson(layerJson: ByteArray, beforeLayerId: String): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_add_style_layer_json(
            state.requireLive().rawHandleValue,
            ByteStructs.bufferView(layerJson, this),
            CoreStructs.stringView(beforeLayerId, this),
            outCommandId,
          )
        )
      }
    }

  public actual fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_add_hillshade_layer(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(beforeLayerId, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_add_color_relief_layer(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(beforeLayerId, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun addLocationIndicatorLayer(layerId: String, beforeLayerId: String): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_add_location_indicator_layer(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            CoreStructs.stringView(beforeLayerId, this),
            outCommandId,
          )
        )
      }
    }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_set_location_indicator_location(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.latLng(coordinate),
          altitude,
          outCommandId,
        )
      )
    }
  }

  public actual fun setLocationIndicatorBearing(layerId: String, bearing: Double): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_location_indicator_bearing(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            bearing,
            outCommandId,
          )
        )
      }
    }

  public actual fun setLocationIndicatorAccuracyRadius(layerId: String, radius: Double): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_location_indicator_accuracy_radius(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            radius,
            outCommandId,
          )
        )
      }
    }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_set_location_indicator_image_name(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          imageKind.nativeValue.toUInt(),
          CoreStructs.stringView(imageId, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun removeStyleLayer(layerId: String): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_remove_style_layer(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          outCommandId,
        )
      )
    }
  }

  public actual suspend fun styleLayerInfo(layerId: String): LayerInfo? = memScoped {
    val outInfo = alloc<mln_style_layer_info>()
    outInfo.size = sizeOf<mln_style_layer_info>().toUInt()
    val outFound = alloc<BooleanVar>()
    Status.check(
      ordered(
        { outOperation ->
          mln_map_get_style_layer_info_start(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            outOperation,
          )
        },
        { operation ->
          mln_map_get_style_layer_info_take_result(operation, outInfo.ptr, outFound.ptr)
        },
      )
    )
    if (!outFound.value) return@memScoped null
    val hasSourceId = (outInfo.fields and MLN_STYLE_LAYER_INFO_SOURCE_ID.toUInt()) != 0u
    val hasSourceLayer = (outInfo.fields and MLN_STYLE_LAYER_INFO_SOURCE_LAYER.toUInt()) != 0u
    val sourceId =
      if (hasSourceId) {
        try {
          layerSourceId(layerId)
        } catch (_: InvalidArgumentException) {
          return@memScoped null
        }
      } else null
    val sourceLayer =
      if (hasSourceLayer) {
        try {
          layerSourceLayer(layerId)
        } catch (_: InvalidArgumentException) {
          return@memScoped null
        }
      } else null
    LayerInfo(
      ByteStructs.copyBufferView(outInfo.type).decodeToString(),
      outInfo.min_zoom,
      outInfo.max_zoom,
      StyleLayerVisibility.fromNative(outInfo.visibility),
      sourceId,
      sourceLayer,
    )
  }

  public actual suspend fun styleLayerIds(): List<String> = memScoped {
    val outList = alloc<ULongVar>()
    outList.value = 0uL
    Status.check(
      ordered(
        { outOperation ->
          mln_map_list_style_layer_ids_start(state.requireLive().rawHandleValue, outOperation)
        },
        { operation -> mln_map_list_style_layer_ids_take_result(operation, outList.ptr) },
      )
    )
    StyleStructs.styleIdList(outList.value.asHandle("mln_map_list_style_ids", ::styleIdListHandle))
  }

  public actual fun moveStyleLayer(layerId: String, beforeLayerId: String): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_move_style_layer(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            CoreStructs.stringView(beforeLayerId, this),
            outCommandId,
          )
        )
      }
    }

  public actual suspend fun styleLayerJson(layerId: String): ByteArray? = memScoped {
    val outLayer = alloc<ULongVar>()
    val outFound = alloc<BooleanVar>()
    outLayer.value = 0uL
    Status.check(
      ordered(
        { outOperation ->
          mln_map_get_style_layer_json_start(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            outOperation,
          )
        },
        { operation ->
          mln_map_get_style_layer_json_take_result(operation, outLayer.ptr, outFound.ptr)
        },
      )
    )
    if (outFound.value)
      ByteStructs.ownedBuffer(outLayer.value.asHandle("mln_buffer", ::ownedBufferHandle))
    else null
  }

  public actual fun setStyleLightJson(lightJson: ByteArray): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_set_style_light_json(
          state.requireLive().rawHandleValue,
          ByteStructs.bufferView(lightJson, this),
          outCommandId,
        )
      )
    }
  }

  public actual fun setStyleLightProperty(propertyName: String, value: ByteArray): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_style_light_property(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(propertyName, this),
            ByteStructs.bufferView(value, this),
            outCommandId,
          )
        )
      }
    }

  public actual suspend fun styleLightProperty(propertyName: String): ByteArray? = memScoped {
    val outValue = alloc<ULongVar>()
    outValue.value = 0uL
    Status.check(
      ordered(
        { outOperation ->
          mln_map_get_style_light_property_start(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(propertyName, this),
            outOperation,
          )
        },
        { operation -> mln_map_get_style_light_property_take_result(operation, outValue.ptr) },
      )
    )
    if (outValue.value == 0uL) null
    else ByteStructs.ownedBuffer(outValue.value.asHandle("mln_buffer", ::ownedBufferHandle))
  }

  public actual fun setStyleTransitionOptions(options: StyleTransitionOptions): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_style_transition_options(
            state.requireLive().rawHandleValue,
            StyleStructs.styleTransitionOptions(options, this),
            outCommandId,
          )
        )
      }
    }

  public actual suspend fun styleTransitionOptions(): StyleTransitionOptions = memScoped {
    val outOptions = mln_style_transition_options_default().getPointer(this)
    Status.check(
      ordered(
        { outOperation ->
          mln_map_get_style_transition_options_start(
            state.requireLive().rawHandleValue,
            outOperation,
          )
        },
        { operation -> mln_map_get_style_transition_options_take_result(operation, outOptions) },
      )
    )
    StyleStructs.styleTransitionOptions(outOptions.pointed)
  }

  public actual fun setLayerProperty(
    layerId: String,
    propertyName: String,
    value: ByteArray,
  ): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_set_layer_property(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(propertyName, this),
          ByteStructs.bufferView(value, this),
          outCommandId,
        )
      )
    }
  }

  public actual suspend fun layerProperty(layerId: String, propertyName: String): ByteArray? =
    memScoped {
      val outValue = alloc<ULongVar>()
      outValue.value = 0uL
      Status.check(
        ordered(
          { outOperation ->
            mln_map_get_layer_property_start(
              state.requireLive().rawHandleValue,
              CoreStructs.stringView(layerId, this),
              CoreStructs.stringView(propertyName, this),
              outOperation,
            )
          },
          { operation -> mln_map_get_layer_property_take_result(operation, outValue.ptr) },
        )
      )
      if (outValue.value == 0uL) null
      else ByteStructs.ownedBuffer(outValue.value.asHandle("mln_buffer", ::ownedBufferHandle))
    }

  public actual fun setLayerFilter(layerId: String, filter: ByteArray): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_layer_filter(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            ByteStructs.bufferViewPointer(filter, this),
            outCommandId,
          )
        )
      }
    }

  public actual fun clearLayerFilter(layerId: String): ULong = command { outCommandId ->
    memScoped {
      Status.check(
        mln_map_set_layer_filter(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          null,
          outCommandId,
        )
      )
    }
  }

  public actual suspend fun layerFilter(layerId: String): ByteArray? = memScoped {
    val outFilter = alloc<ULongVar>()
    outFilter.value = 0uL
    Status.check(
      ordered(
        { outOperation ->
          mln_map_get_layer_filter_start(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            outOperation,
          )
        },
        { operation -> mln_map_get_layer_filter_take_result(operation, outFilter.ptr) },
      )
    )
    if (outFilter.value == 0uL) null
    else ByteStructs.ownedBuffer(outFilter.value.asHandle("mln_buffer", ::ownedBufferHandle))
  }

  public actual fun setLayerSourceLayer(layerId: String, sourceLayer: String): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_layer_source_layer(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            CoreStructs.stringView(sourceLayer, this),
            outCommandId,
          )
        )
      }
    }

  public actual suspend fun layerSourceLayer(layerId: String): String =
    takeLayerBuffer(
        layerId,
        ::mln_map_copy_layer_source_layer_start,
        ::mln_map_copy_layer_source_layer_take_result,
      )
      .decodeToString()

  public actual fun setLayerSourceId(layerId: String, sourceId: String): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_layer_source_id(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            CoreStructs.stringView(sourceId, this),
            outCommandId,
          )
        )
      }
    }

  public actual suspend fun layerSourceId(layerId: String): String =
    takeLayerBuffer(
        layerId,
        ::mln_map_copy_layer_source_id_start,
        ::mln_map_copy_layer_source_id_take_result,
      )
      .decodeToString()

  /**
   * Probes the required length, then copies. A null buffer with zero capacity is a size probe the C
   * API answers with OK.
   */
  private fun copyMapText(
    copy: (ULong, CPointer<ByteVar>?, ULong, CPointer<ULongVar>) -> Int
  ): String = memScoped {
    val handle = state.requireLive().rawHandleValue
    val outSize = alloc<ULongVar>()
    Status.check(copy(handle, null, 0UL, outSize.ptr))
    val required = checkedInt(outSize.value, "map text size")
    if (required == 0) return@memScoped ""

    val buffer = allocArray<ByteVar>(required)
    val outCopied = alloc<ULongVar>()
    Status.check(copy(handle, buffer, required.toULong(), outCopied.ptr))
    buffer.readBytes(checkedInt(outCopied.value, "map copied text size")).decodeToString()
  }

  private fun copyMapBytes(
    copy: (ULong, CPointer<UByteVar>?, ULong, CPointer<ULongVar>) -> Int
  ): ByteArray = memScoped {
    val handle = state.requireLive().rawHandleValue
    val outSize = alloc<ULongVar>()
    Status.check(copy(handle, null, 0UL, outSize.ptr))
    val required = checkedInt(outSize.value, "map byte size")
    if (required == 0) return@memScoped ByteArray(0)

    val buffer = allocArray<UByteVar>(required)
    val outCopied = alloc<ULongVar>()
    Status.check(copy(handle, buffer, required.toULong(), outCopied.ptr))
    buffer.reinterpret<ByteVar>().readBytes(checkedInt(outCopied.value, "map copied byte size"))
  }

  private fun copyLayerText(
    layerId: String,
    copy: (ULong, CValue<mln_buffer_view>, CPointer<ByteVar>?, ULong, CPointer<ULongVar>) -> Int,
  ): String = memScoped {
    val handle = state.requireLive().rawHandleValue
    val outSize = alloc<ULongVar>()
    Status.check(copy(handle, CoreStructs.stringView(layerId, this), null, 0UL, outSize.ptr))
    val required = checkedInt(outSize.value, "layer text size")
    if (required == 0) return@memScoped ""

    val buffer = allocArray<ByteVar>(required)
    val outCopied = alloc<ULongVar>()
    Status.check(
      copy(handle, CoreStructs.stringView(layerId, this), buffer, required.toULong(), outCopied.ptr)
    )
    buffer.readBytes(checkedInt(outCopied.value, "layer copied text size")).decodeToString()
  }

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_layer_min_zoom(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            minZoom,
            outCommandId,
          )
        )
      }
    }

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_layer_max_zoom(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            maxZoom,
            outCommandId,
          )
        )
      }
    }

  public actual fun setLayerVisibility(layerId: String, visibility: StyleLayerVisibility): ULong =
    command { outCommandId ->
      memScoped {
        Status.check(
          mln_map_set_layer_visibility(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            visibility.nativeValue.toUInt(),
            outCommandId,
          )
        )
      }
    }

  public actual fun requestRepaint(): Long = memScoped {
    val outCommand = alloc<ULongVar>()
    Status.check(mln_map_request_repaint(state.requireLive().rawHandleValue, outCommand.ptr))
    outCommand.value.toLong()
  }

  public actual suspend fun requestStillImage() {
    val operation = startOperation { outOperation ->
      mln_map_request_still_image_start(state.requireLive().rawHandleValue, outOperation)
    }
    try {
      runtime.awaitOperation(operation)
    } finally {
      mln_operation_release(operation)
    }
  }

  public actual fun snapshot(): MapSnapshot = memScoped {
    val value = alloc<mln_map_snapshot>()
    value.size = sizeOf<mln_map_snapshot>().toUInt()
    Status.check(mln_map_snapshot_get(state.requireLive().rawHandleValue, value.ptr))
    val extent = value.logical_extent
    MapSnapshot(
      value.generation.toLong(),
      MapStructs.debugOptions(value.debug_options),
      MapStructs.cameraOptions(value.camera),
      MapSize(extent.width.toInt(), extent.height.toInt(), extent.scale_factor),
      MapStructs.projectionModeOptions(value.projection_mode),
      MapStructs.viewportOptions(value.viewport),
      value.fully_loaded,
      value.rendering_stats_view_enabled,
      value.repaint_demand,
      value.latest_render_update_generation.toLong(),
      MapStructs.tileOptions(value.tile),
      MapStructs.boundOptions(value.bounds),
      MapStructs.freeCameraOptions(value.free_camera),
    )
  }

  public actual fun setDebugOptions(options: Set<DebugOption>): Long = memScoped {
    val outCommand = alloc<ULongVar>()
    Status.check(
      mln_map_set_debug_options(
        state.requireLive().rawHandleValue,
        MapStructs.debugOptionMask(options),
        outCommand.ptr,
      )
    )
    outCommand.value.toLong()
  }

  public actual fun setRenderingStatsViewEnabled(enabled: Boolean): Long = memScoped {
    val outCommand = alloc<ULongVar>()
    Status.check(
      mln_map_set_rendering_stats_view_enabled(
        state.requireLive().rawHandleValue,
        enabled,
        outCommand.ptr,
      )
    )
    outCommand.value.toLong()
  }

  public actual fun setViewportOptions(options: ViewportOptions): Long = memScoped {
    val outCommand = alloc<ULongVar>()
    Status.check(
      mln_map_set_viewport_options(
        state.requireLive().rawHandleValue,
        MapStructs.viewportOptions(options, this),
        outCommand.ptr,
      )
    )
    outCommand.value.toLong()
  }

  public actual fun setTileOptions(options: TileOptions): Long = memScoped {
    val outCommand = alloc<ULongVar>()
    Status.check(
      mln_map_set_tile_options(
        state.requireLive().rawHandleValue,
        MapStructs.tileOptions(options, this),
        outCommand.ptr,
      )
    )
    outCommand.value.toLong()
  }

  public actual fun setBounds(options: BoundOptions): Long = memScoped {
    val outCommand = alloc<ULongVar>()
    Status.check(
      mln_map_set_bounds(
        state.requireLive().rawHandleValue,
        MapStructs.boundOptions(options, this),
        outCommand.ptr,
      )
    )
    outCommand.value.toLong()
  }

  public actual fun setFreeCameraOptions(options: FreeCameraOptions): Long = memScoped {
    val outCommand = alloc<ULongVar>()
    Status.check(
      mln_map_set_free_camera_options(
        state.requireLive().rawHandleValue,
        MapStructs.freeCameraOptions(options, this),
        outCommand.ptr,
      )
    )
    outCommand.value.toLong()
  }

  public actual fun resize(size: MapSize): Long = memScoped {
    val extent = alloc<mln_logical_extent>()
    extent.width = size.width.toUInt()
    extent.height = size.height.toUInt()
    extent.scale_factor = size.scaleFactor
    val outCommand = alloc<ULongVar>()
    Status.check(
      mln_map_resize(state.requireLive().rawHandleValue, extent.readValue(), outCommand.ptr)
    )
    outCommand.value.toLong()
  }

  public actual fun cameraSnapshot(): CameraSnapshot = memScoped {
    val outCamera = mln_camera_options_default().getPointer(this)
    val outGeneration = alloc<ULongVar>()
    Status.check(
      mln_map_camera_snapshot_get(state.requireLive().rawHandleValue, outCamera, outGeneration.ptr)
    )
    CameraSnapshot(outGeneration.value.toLong(), MapStructs.cameraOptions(outCamera.pointed))
  }

  public actual fun updateCamera(update: CameraUpdate): Long = memScoped {
    val nativeUpdate = mln_camera_update_default().getPointer(this)
    nativeUpdate.pointed.mode = update.mode.nativeValue.toUInt()
    val camera = MapStructs.cameraOptions(update.camera, this).pointed
    nativeUpdate.pointed.camera.size = camera.size
    nativeUpdate.pointed.camera.fields = camera.fields
    nativeUpdate.pointed.camera.latitude = camera.latitude
    nativeUpdate.pointed.camera.longitude = camera.longitude
    nativeUpdate.pointed.camera.center_altitude = camera.center_altitude
    nativeUpdate.pointed.camera.padding.top = camera.padding.top
    nativeUpdate.pointed.camera.padding.left = camera.padding.left
    nativeUpdate.pointed.camera.padding.bottom = camera.padding.bottom
    nativeUpdate.pointed.camera.padding.right = camera.padding.right
    nativeUpdate.pointed.camera.anchor.x = camera.anchor.x
    nativeUpdate.pointed.camera.anchor.y = camera.anchor.y
    nativeUpdate.pointed.camera.zoom = camera.zoom
    nativeUpdate.pointed.camera.bearing = camera.bearing
    nativeUpdate.pointed.camera.pitch = camera.pitch
    nativeUpdate.pointed.camera.roll = camera.roll
    nativeUpdate.pointed.camera.field_of_view = camera.field_of_view
    val animation = MapStructs.animationOptions(update.animation, this).pointed
    nativeUpdate.pointed.animation.size = animation.size
    nativeUpdate.pointed.animation.fields = animation.fields
    nativeUpdate.pointed.animation.duration_ms = animation.duration_ms
    nativeUpdate.pointed.animation.velocity = animation.velocity
    nativeUpdate.pointed.animation.min_zoom = animation.min_zoom
    nativeUpdate.pointed.animation.easing.x1 = animation.easing.x1
    nativeUpdate.pointed.animation.easing.y1 = animation.easing.y1
    nativeUpdate.pointed.animation.easing.x2 = animation.easing.x2
    nativeUpdate.pointed.animation.easing.y2 = animation.easing.y2
    nativeUpdate.pointed.animation.transition_id = animation.transition_id
    nativeUpdate.pointed.gesture_phase = update.gesturePhase.nativeValue.toUInt()
    nativeUpdate.pointed.gesture_id = update.gestureId.toULong()
    nativeUpdate.pointed.animation_id = update.animationId.toULong()
    val outCommand = alloc<ULongVar>()
    Status.check(
      mln_map_update_camera(state.requireLive().rawHandleValue, nativeUpdate, outCommand.ptr)
    )
    outCommand.value.toLong()
  }

  public actual suspend fun queryCamera(): CameraSnapshot {
    val operation = startOperation { outOperation ->
      mln_map_camera_query_start(state.requireLive().rawHandleValue, outOperation)
    }
    try {
      runtime.awaitOperation(operation)
      return memScoped {
        val result = alloc<mln_camera_query_result>()
        result.size = sizeOf<mln_camera_query_result>().toUInt()
        Status.check(mln_map_camera_query_take_result(operation, result.ptr))
        CameraSnapshot(result.generation.toLong(), MapStructs.cameraOptions(result.camera))
      }
    } finally {
      mln_operation_release(operation)
    }
  }

  public actual fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachMetalOwnedTexture(this, descriptor, options)

  public actual fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachMetalBorrowedTexture(this, descriptor, options)

  public actual fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachVulkanOwnedTexture(this, descriptor, options)

  public actual fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachVulkanBorrowedTexture(this, descriptor, options)

  public actual fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachOpenGLOwnedTexture(this, descriptor, options)

  public actual fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    RenderSessionHandle.attachOpenGLBorrowedTexture(this, descriptor, options)

  public actual fun attachMetalSurface(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment = RenderSessionHandle.attachMetalSurface(this, descriptor, options)

  public actual fun attachVulkanSurface(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment = RenderSessionHandle.attachVulkanSurface(this, descriptor, options)

  public actual fun attachOpenGLSurface(
    descriptor: OpenGLSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment = RenderSessionHandle.attachOpenGLSurface(this, descriptor, options)

  public actual suspend fun createProjection(): MapProjectionHandle {
    val operation = startOperation { outOperation ->
      mln_map_projection_create_start(state.requireLive().rawHandleValue, outOperation)
    }
    try {
      runtime.awaitOperation(operation)
      return memScoped {
        val outProjection = alloc<ULongVar>()
        outProjection.value = 0uL
        Status.check(mln_map_projection_create_take_result(operation, outProjection.ptr))
        MapProjectionHandle(
          outProjection.value.asHandle(
            "mln_map_projection_create_take_result",
            ::mapProjectionHandle,
          )
        )
      }
    } finally {
      mln_operation_release(operation)
    }
  }

  public actual suspend fun close() {
    if (!state.beginClose()) return
    val operation =
      try {
        startOperation { outOperation ->
          mln_map_close_start(state.handleForClose().rawHandleValue, outOperation)
        }
      } catch (error: Throwable) {
        state.abortClose()
        throw error
      }
    try {
      runtime.awaitOperation(operation)
    } catch (error: Throwable) {
      state.abortClose()
      throw error
    } finally {
      mln_operation_release(operation)
    }
    state.completeClose {
      runtime.unregisterMap(this)
      runtimeRetention.close()
    }
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  internal fun nativeHandle(): NativeMap = state.requireLive()

  internal fun nativeHandleId(): Long = state.handleId()

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    state.retainChild(childTypeName)

  private fun checkedInt(value: ULong, name: String): Int {
    require(value <= Int.MAX_VALUE.toULong()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  private suspend fun takeStyleBuffer(
    start: (ULong, CPointer<ULongVar>) -> Int,
    take: (ULong, CPointer<ULongVar>) -> Int,
  ): ByteArray = memScoped {
    val outBuffer = alloc<ULongVar>()
    outBuffer.value = 0uL
    Status.check(
      ordered(
        { outOperation -> start(state.requireLive().rawHandleValue, outOperation) },
        { operation -> take(operation, outBuffer.ptr) },
      )
    )
    ByteStructs.ownedBuffer(outBuffer.value.asHandle("mln_buffer", ::ownedBufferHandle))
  }

  private suspend fun takeLayerBuffer(
    layerId: String,
    start: (ULong, CValue<mln_buffer_view>, CPointer<ULongVar>) -> Int,
    take: (ULong, CPointer<ULongVar>) -> Int,
  ): ByteArray = memScoped {
    val outBuffer = alloc<ULongVar>()
    outBuffer.value = 0uL
    Status.check(
      ordered(
        { outOperation ->
          start(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            outOperation,
          )
        },
        { operation -> take(operation, outBuffer.ptr) },
      )
    )
    ByteStructs.ownedBuffer(outBuffer.value.asHandle("mln_buffer", ::ownedBufferHandle))
  }

  public actual companion object {
    public actual suspend fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle =
      memScoped {
        val operation = startOperation { outOperation ->
          mln_map_create_start(
            runtime.nativeHandle().rawHandleValue,
            mapOptions(options, this),
            outOperation,
          )
        }
        try {
          runtime.awaitOperation(operation)
          val outMap = alloc<ULongVar>()
          outMap.value = 0uL
          Status.check(mln_map_create_take_result(operation, outMap.ptr))
          val map =
            MapHandle(
              runtime,
              outMap.value.asHandle("mln_map_create_take_result", ::mapHandle),
              options.eventMask,
            )
          runtime.registerMap(map)
          map
        } finally {
          mln_operation_release(operation)
        }
      }

    internal fun mapOptionsForTesting(options: MapOptions, inspect: (mln_map_options) -> Unit) {
      memScoped { inspect(mapOptions(options, this).pointed) }
    }

    private fun mapOptions(options: MapOptions, scope: MemScope): CPointer<mln_map_options> {
      val nativeOptions = scope.alloc<mln_map_options>()
      mln_map_options_default().place(nativeOptions.ptr)
      options.width?.let {
        Status.requireArgument(it >= 0) { "width must be non-negative" }
        nativeOptions.initial_extent.width = it.toUInt()
      }
      options.height?.let {
        Status.requireArgument(it >= 0) { "height must be non-negative" }
        nativeOptions.initial_extent.height = it.toUInt()
      }
      options.scaleFactor?.let { nativeOptions.initial_extent.scale_factor = it }
      options.mapMode?.let {
        Status.requireArgument(it.isKnown) {
          "Unknown map mode cannot be used as input: ${it.nativeValue}"
        }
        nativeOptions.map_mode = it.nativeValue.toUInt()
      }
      options.fastPforEnabled?.let { nativeOptions.fast_pfor_enabled = it }
      nativeOptions.event_mask = options.eventMask.nativeValue.toULong()
      return nativeOptions.ptr
    }
  }

  private suspend inline fun ordered(
    start: (CPointer<ULongVar>) -> Int,
    take: (ULong) -> Int,
  ): Int {
    val operation = startOperation(start)
    try {
      runtime.awaitOperation(operation)
      return take(operation)
    } finally {
      mln_operation_release(operation)
    }
  }
}

@OptIn(ExperimentalForeignApi::class)
private inline fun command(call: (CPointer<ULongVar>) -> Unit): ULong = memScoped {
  val outCommandId = alloc<ULongVar>()
  outCommandId.value = 0uL
  call(outCommandId.ptr)
  outCommandId.value
}
