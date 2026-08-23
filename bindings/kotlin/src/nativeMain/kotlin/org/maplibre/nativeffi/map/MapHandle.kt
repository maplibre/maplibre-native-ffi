package org.maplibre.nativeffi.map

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
import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraDelta
import org.maplibre.nativeffi.camera.CameraSnapshot
import org.maplibre.nativeffi.camera.CameraUpdate
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.async.mapDeferred
import org.maplibre.nativeffi.internal.c.MLN_STYLE_LAYER_INFO_SOURCE_ID
import org.maplibre.nativeffi.internal.c.MLN_STYLE_LAYER_INFO_SOURCE_LAYER
import org.maplibre.nativeffi.internal.c.mln_buffer_view
import org.maplibre.nativeffi.internal.c.mln_camera_delta_default
import org.maplibre.nativeffi.internal.c.mln_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_camera_query_result
import org.maplibre.nativeffi.internal.c.mln_camera_update_default
import org.maplibre.nativeffi.internal.c.mln_completion
import org.maplibre.nativeffi.internal.c.mln_completion_result
import org.maplibre.nativeffi.internal.c.mln_image_stretch
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_logical_extent
import org.maplibre.nativeffi.internal.c.mln_map_add_color_relief_layer
import org.maplibre.nativeffi.internal.c.mln_map_add_custom_geometry_source
import org.maplibre.nativeffi.internal.c.mln_map_add_custom_mvt_vector_source
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
import org.maplibre.nativeffi.internal.c.mln_map_apply_camera_delta
import org.maplibre.nativeffi.internal.c.mln_map_camera_query
import org.maplibre.nativeffi.internal.c.mln_map_camera_snapshot_get
import org.maplibre.nativeffi.internal.c.mln_map_copy_layer_source_id
import org.maplibre.nativeffi.internal.c.mln_map_copy_layer_source_layer
import org.maplibre.nativeffi.internal.c.mln_map_copy_style_image_stretches
import org.maplibre.nativeffi.internal.c.mln_map_create
import org.maplibre.nativeffi.internal.c.mln_map_get_feature_state
import org.maplibre.nativeffi.internal.c.mln_map_get_image_source_coordinates
import org.maplibre.nativeffi.internal.c.mln_map_get_layer_filter
import org.maplibre.nativeffi.internal.c.mln_map_get_layer_property
import org.maplibre.nativeffi.internal.c.mln_map_get_style_image_info
import org.maplibre.nativeffi.internal.c.mln_map_get_style_layer_info
import org.maplibre.nativeffi.internal.c.mln_map_get_style_layer_json
import org.maplibre.nativeffi.internal.c.mln_map_get_style_light_property
import org.maplibre.nativeffi.internal.c.mln_map_get_style_source_info
import org.maplibre.nativeffi.internal.c.mln_map_get_style_transition_options
import org.maplibre.nativeffi.internal.c.mln_map_invalidate_custom_geometry_source_region
import org.maplibre.nativeffi.internal.c.mln_map_invalidate_custom_geometry_source_tile
import org.maplibre.nativeffi.internal.c.mln_map_invalidate_custom_mvt_vector_source_tile
import org.maplibre.nativeffi.internal.c.mln_map_list_style_layer_ids
import org.maplibre.nativeffi.internal.c.mln_map_list_style_source_ids
import org.maplibre.nativeffi.internal.c.mln_map_loaded_style_json
import org.maplibre.nativeffi.internal.c.mln_map_move_style_layer
import org.maplibre.nativeffi.internal.c.mln_map_options
import org.maplibre.nativeffi.internal.c.mln_map_options_default
import org.maplibre.nativeffi.internal.c.mln_map_projection_create
import org.maplibre.nativeffi.internal.c.mln_map_release
import org.maplibre.nativeffi.internal.c.mln_map_remove_feature_state
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_image
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_layer
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_source
import org.maplibre.nativeffi.internal.c.mln_map_request_repaint
import org.maplibre.nativeffi.internal.c.mln_map_request_still_image
import org.maplibre.nativeffi.internal.c.mln_map_resize
import org.maplibre.nativeffi.internal.c.mln_map_set_bounds
import org.maplibre.nativeffi.internal.c.mln_map_set_custom_geometry_source_tile_data
import org.maplibre.nativeffi.internal.c.mln_map_set_custom_mvt_vector_source_tile_data
import org.maplibre.nativeffi.internal.c.mln_map_set_custom_mvt_vector_source_tile_error
import org.maplibre.nativeffi.internal.c.mln_map_set_debug_options
import org.maplibre.nativeffi.internal.c.mln_map_set_event_mask
import org.maplibre.nativeffi.internal.c.mln_map_set_feature_state
import org.maplibre.nativeffi.internal.c.mln_map_set_free_camera_options
import org.maplibre.nativeffi.internal.c.mln_map_set_geojson_source_data
import org.maplibre.nativeffi.internal.c.mln_map_set_geojson_source_synchronous_tiling
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
import org.maplibre.nativeffi.internal.c.mln_map_style_url
import org.maplibre.nativeffi.internal.c.mln_map_update_camera
import org.maplibre.nativeffi.internal.c.mln_style_image_result
import org.maplibre.nativeffi.internal.c.mln_style_image_stretches_result
import org.maplibre.nativeffi.internal.c.mln_style_layer_result
import org.maplibre.nativeffi.internal.c.mln_style_source_result
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.internal.lifecycle.asHandle
import org.maplibre.nativeffi.internal.lifecycle.mapHandle
import org.maplibre.nativeffi.internal.lifecycle.mapProjectionHandle
import org.maplibre.nativeffi.internal.lifecycle.rawHandleValue
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.ByteStructs
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.MapStructs
import org.maplibre.nativeffi.internal.struct.QueryStructs
import org.maplibre.nativeffi.internal.struct.StyleStructs
import org.maplibre.nativeffi.query.FeatureStateSelector
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
import org.maplibre.nativeffi.runtime.CommandCompletion
import org.maplibre.nativeffi.runtime.RuntimeEventMask
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.CustomMvtVectorSourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceDataHandle
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
  private val state = HandleState("MapHandle", handle, runtime)
  private var cachedEventMask =
    RuntimeEventMask(cachedEventMask.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)
  private val customGeometrySources =
    CustomGeometrySourceRegistry<CustomGeometrySourceState> { it.close() }
  private val customMvtVectorSources =
    CustomGeometrySourceRegistry<CustomMvtVectorSourceState> { it.close() }

  public actual val eventMask: RuntimeEventMask
    get() = cachedEventMask

  public actual fun setEventMask(value: RuntimeEventMask): Deferred<CommandCompletion> =
    command { completion ->
        Status.check(
          mln_map_set_event_mask(
            state.requireLive().rawHandleValue,
            value.nativeValue.toULong(),
            completion,
          )
        )
      }
      .mapDeferred {
        cachedEventMask =
          RuntimeEventMask(value.nativeValue and RuntimeEventMask.ALL_MAP_EVENTS.nativeValue)
        it
      }

  public actual fun setStyleUrl(url: String): Deferred<CommandCompletion> = command { completion ->
    MemoryUtil.requireValidCString(url)
    Status.check(mln_map_set_style_url(state.requireLive().rawHandleValue, url, completion))
  }

  public actual fun setStyleJson(json: ByteArray): Deferred<CommandCompletion> =
    command { completion ->
      memScoped {
        Status.check(
          mln_map_set_style_json(
            state.requireLive().rawHandleValue,
            ByteStructs.bufferView(json, this),
            completion,
          )
        )
      }
    }

  public actual fun setFeatureState(
    selector: FeatureStateSelector,
    value: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_feature_state(
          state.requireLive().rawHandleValue,
          QueryStructs.featureStateSelector(selector, this),
          ByteStructs.bufferView(value, this),
          completion,
        )
      )
    }
  }

  public actual fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray> =
    memScoped {
      CompletionBridge.submit(
        { result -> checkNotNull(bufferCompletion(result)) },
        { completion ->
          mln_map_get_feature_state(
            state.requireLive().rawHandleValue,
            QueryStructs.featureStateSelector(selector, this),
            completion,
          )
        },
      )
    }

  public actual fun removeFeatureState(
    selector: FeatureStateSelector
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_remove_feature_state(
          state.requireLive().rawHandleValue,
          QueryStructs.featureStateSelector(selector, this),
          completion,
        )
      )
    }
  }

  public actual fun loadedStyleJson(): Deferred<ByteArray> =
    CompletionBridge.submit(
      { result -> checkNotNull(bufferCompletion(result)) },
      { completion -> mln_map_loaded_style_json(state.requireLive().rawHandleValue, completion) },
    )

  public actual fun styleUrl(): Deferred<String> =
    CompletionBridge.submit(
      { result -> checkNotNull(bufferCompletion(result)).decodeToString() },
      { completion -> mln_map_style_url(state.requireLive().rawHandleValue, completion) },
    )

  public actual fun addStyleSourceJson(
    sourceId: String,
    sourceJson: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_add_style_source_json(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          ByteStructs.bufferView(sourceJson, this),
          completion,
        )
      )
    }
  }

  public actual fun removeStyleSource(sourceId: String): Deferred<CommandCompletion> =
    command { completion ->
      memScoped {
        Status.check(
          mln_map_remove_style_source(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            completion,
          )
        )
      }
    }

  public actual fun styleSourceInfo(sourceId: String): Deferred<SourceInfo?> = memScoped {
    CompletionBridge.submit(
      { result ->
        if (result.pointed.value_count == 0uL) null
        else {
          val value = result.pointed.value!!.reinterpret<mln_style_source_result>().pointed
          val tileUrls =
            List(value.tile_url_count.toInt()) { index ->
              CoreStructs.stringView(value.tile_urls!![index])
            }
          StyleStructs.sourceInfo(
            value.info,
            if (value.info.has_attribution) CoreStructs.stringView(value.attribution) else null,
            CoreStructs.stringView(value.url),
            tileUrls,
          )
        }
      },
      { completion ->
        mln_map_get_style_source_info(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          completion,
        )
      },
    )
  }

  public actual fun styleSourceIds(): Deferred<List<String>> =
    CompletionBridge.submit(
      ::stringViewsCompletion,
      { completion ->
        mln_map_list_style_source_ids(state.requireLive().rawHandleValue, completion)
      },
    )

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_add_geojson_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.geoJsonSourceOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun addGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> = command { completion ->
    data.withNativeHandle { nativeData ->
      memScoped {
        Status.check(
          mln_map_add_geojson_source_data(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            nativeData.rawHandleValue,
            completion,
          )
        )
      }
    }
  }

  public actual fun setGeoJsonSourceUrl(
    sourceId: String,
    url: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_geojson_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          completion,
        )
      )
    }
  }

  public actual fun setGeoJsonSourceData(
    sourceId: String,
    data: GeoJsonSourceDataHandle,
  ): Deferred<CommandCompletion> = command { completion ->
    data.withNativeHandle { nativeData ->
      memScoped {
        Status.check(
          mln_map_set_geojson_source_data(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            nativeData.rawHandleValue,
            completion,
          )
        )
      }
    }
  }

  public actual fun setGeoJsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_geojson_source_synchronous_tiling(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          enabled,
          completion,
        )
      )
    }
  }

  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion> =
    command { completion
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
              completion,
            )
          )
        }
      }
    }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_custom_geometry_source_tile_data(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.canonicalTileId(tileId),
          ByteStructs.bufferView(data, this),
          completion,
        )
      )
    }
  }

  public actual fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_invalidate_custom_geometry_source_tile(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.canonicalTileId(tileId),
          completion,
        )
      )
    }
  }

  public actual fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_invalidate_custom_geometry_source_region(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngBounds(bounds),
          completion,
        )
      )
    }
  }

  public actual fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
  ): Deferred<CommandCompletion> = command { completion ->
    val registry = customMvtVectorSources
    val sourceState = CustomMvtVectorSourceState(options) { registry.remove(sourceId) }
    registry.install(sourceId, sourceState) {
      memScoped {
        Status.check(
          mln_map_add_custom_mvt_vector_source(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            sourceState.descriptor(),
            completion,
          )
        )
      }
    }
  }

  public actual fun setCustomMvtVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_custom_mvt_vector_source_tile_data(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.canonicalTileId(tileId),
          ByteStructs.bufferView(data, this),
          completion,
        )
      )
    }
  }

  public actual fun setCustomMvtVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_custom_mvt_vector_source_tile_error(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.canonicalTileId(tileId),
          CoreStructs.stringView(message, this),
          completion,
        )
      )
    }
  }

  public actual fun invalidateCustomMvtVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_invalidate_custom_mvt_vector_source_tile(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.canonicalTileId(tileId),
          completion,
        )
      )
    }
  }

  internal fun customGeometrySourceCountForTesting(): Int = customGeometrySources.size

  public actual fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_add_vector_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.tileSourceOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    val tileSnapshot = tiles.toList()
    memScoped {
      Status.check(
        mln_map_add_vector_source_tiles(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.stringViewArray(tileSnapshot, this),
          tileSnapshot.size.toULong(),
          StyleStructs.tileSourceOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_add_raster_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.tileSourceOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    val tileSnapshot = tiles.toList()
    memScoped {
      Status.check(
        mln_map_add_raster_source_tiles(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.stringViewArray(tileSnapshot, this),
          tileSnapshot.size.toULong(),
          StyleStructs.tileSourceOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_add_raster_dem_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(url, this),
          StyleStructs.tileSourceOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ): Deferred<CommandCompletion> = command { completion ->
    val tileSnapshot = tiles.toList()
    memScoped {
      Status.check(
        mln_map_add_raster_dem_source_tiles(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.stringViewArray(tileSnapshot, this),
          tileSnapshot.size.toULong(),
          StyleStructs.tileSourceOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_style_image(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(imageId, this),
          StyleStructs.premultipliedRgba8Image(image, this),
          StyleStructs.styleImageOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun removeStyleImage(imageId: String): Deferred<CommandCompletion> =
    command { completion ->
      memScoped {
        Status.check(
          mln_map_remove_style_image(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(imageId, this),
            completion,
          )
        )
      }
    }

  public actual fun styleImageInfo(imageId: String): Deferred<StyleImageInfo?> = memScoped {
    CompletionBridge.submit(
      { result ->
        if (result.pointed.value_count == 0uL) null
        else
          StyleStructs.styleImageInfo(
            result.pointed.value!!.reinterpret<mln_style_image_result>().pointed.info
          )
      },
      { completion ->
        mln_map_get_style_image_info(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(imageId, this),
          completion,
        )
      },
    )
  }

  public actual fun styleImageStretches(
    imageId: String
  ): Deferred<Pair<List<ImageStretch>, List<ImageStretch>>?> = memScoped {
    CompletionBridge.submit(
      { result ->
        if (result.pointed.value_count == 0uL) null
        else {
          val value = result.pointed.value!!.reinterpret<mln_style_image_stretches_result>().pointed
          val copy = { values: CPointer<mln_image_stretch>?, count: ULong ->
            List(count.toInt()) { index -> ImageStretch(values!![index].from, values[index].to) }
          }
          copy(value.stretch_x, value.stretch_x_count) to
            copy(value.stretch_y, value.stretch_y_count)
        }
      },
      { completion ->
        mln_map_copy_style_image_stretches(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(imageId, this),
          completion,
        )
      },
    )
  }

  public actual fun copyStyleImagePremultipliedRgba8(imageId: String): Deferred<StyleImage?> =
    memScoped {
      CompletionBridge.submit(
        { result ->
          if (result.pointed.value_count == 0uL) null
          else {
            val value = result.pointed.value!!.reinterpret<mln_style_image_result>().pointed
            val info = StyleStructs.styleImageInfo(value.info)
            StyleImage(
              PremultipliedRgba8Image(
                info.width,
                info.height,
                info.stride,
                ByteStructs.copyBufferView(value.pixels),
              ),
              info.pixelRatio,
              info.sdf,
            )
          }
        },
        { completion ->
          mln_map_get_style_image_info(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(imageId, this),
            completion,
          )
        },
      )
    }

  public actual fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion> = command { completion ->
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      Status.check(
        mln_map_add_image_source_url(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
          CoreStructs.stringView(url, this),
          completion,
        )
      )
    }
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> = command { completion ->
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      Status.check(
        mln_map_add_image_source_image(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
          StyleStructs.premultipliedRgba8Image(image, this),
          completion,
        )
      )
    }
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    command { completion ->
      memScoped {
        Status.check(
          mln_map_set_image_source_url(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(sourceId, this),
            CoreStructs.stringView(url, this),
            completion,
          )
        )
      }
    }

  public actual fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_image_source_image(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          StyleStructs.premultipliedRgba8Image(image, this),
          completion,
        )
      )
    }
  }

  public actual fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion> = command { completion ->
    val coordinateSnapshot = coordinates.toList()
    memScoped {
      Status.check(
        mln_map_set_image_source_coordinates(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          CoreStructs.latLngArray(coordinateSnapshot, this),
          coordinateSnapshot.size.toULong(),
          completion,
        )
      )
    }
  }

  public actual fun imageSourceCoordinates(sourceId: String): Deferred<List<LatLng>?> = memScoped {
    CompletionBridge.submit(
      { result ->
        val count = result.pointed.value_count
        if (count == 0uL) null
        else
          CoreStructs.latLngArray(result.pointed.value!!.reinterpret<mln_lat_lng>(), count.toInt())
      },
      { completion ->
        mln_map_get_image_source_coordinates(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(sourceId, this),
          completion,
        )
      },
    )
  }

  public actual fun addStyleLayerJson(
    layerJson: ByteArray,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_add_style_layer_json(
          state.requireLive().rawHandleValue,
          ByteStructs.bufferView(layerJson, this),
          CoreStructs.stringView(beforeLayerId, this),
          completion,
        )
      )
    }
  }

  public actual fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_add_hillshade_layer(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(beforeLayerId, this),
          completion,
        )
      )
    }
  }

  public actual fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_add_color_relief_layer(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(sourceId, this),
          CoreStructs.stringView(beforeLayerId, this),
          completion,
        )
      )
    }
  }

  public actual fun addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_add_location_indicator_layer(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(beforeLayerId, this),
          completion,
        )
      )
    }
  }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_location_indicator_location(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.latLng(coordinate),
          altitude,
          completion,
        )
      )
    }
  }

  public actual fun setLocationIndicatorBearing(
    layerId: String,
    bearing: Double,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_location_indicator_bearing(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          bearing,
          completion,
        )
      )
    }
  }

  public actual fun setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_location_indicator_accuracy_radius(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          radius,
          completion,
        )
      )
    }
  }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_location_indicator_image_name(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          imageKind.nativeValue.toUInt(),
          CoreStructs.stringView(imageId, this),
          completion,
        )
      )
    }
  }

  public actual fun removeStyleLayer(layerId: String): Deferred<CommandCompletion> =
    command { completion ->
      memScoped {
        Status.check(
          mln_map_remove_style_layer(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            completion,
          )
        )
      }
    }

  public actual fun styleLayerInfo(layerId: String): Deferred<LayerInfo?> = memScoped {
    CompletionBridge.submit(
      { result ->
        if (result.pointed.value_count == 0uL) null
        else {
          val raw = result.pointed.value!!.reinterpret<mln_style_layer_result>().pointed
          val info = raw.info
          val fields = info.fields
          LayerInfo(
            ByteStructs.copyBufferView(info.type).decodeToString(),
            info.min_zoom,
            info.max_zoom,
            StyleLayerVisibility.fromNative(info.visibility),
            if ((fields and MLN_STYLE_LAYER_INFO_SOURCE_ID) != 0u)
              ByteStructs.copyBufferView(raw.source_id).decodeToString()
            else null,
            if ((fields and MLN_STYLE_LAYER_INFO_SOURCE_LAYER) != 0u)
              ByteStructs.copyBufferView(raw.source_layer).decodeToString()
            else null,
          )
        }
      },
      { completion ->
        mln_map_get_style_layer_info(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          completion,
        )
      },
    )
  }

  public actual fun styleLayerIds(): Deferred<List<String>> =
    CompletionBridge.submit(
      ::stringViewsCompletion,
      { completion -> mln_map_list_style_layer_ids(state.requireLive().rawHandleValue, completion) },
    )

  public actual fun moveStyleLayer(
    layerId: String,
    beforeLayerId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_move_style_layer(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(beforeLayerId, this),
          completion,
        )
      )
    }
  }

  public actual fun styleLayerJson(layerId: String): Deferred<ByteArray?> = memScoped {
    CompletionBridge.submit(
      ::bufferCompletion,
      { completion ->
        mln_map_get_style_layer_json(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          completion,
        )
      },
    )
  }

  public actual fun setStyleLightJson(lightJson: ByteArray): Deferred<CommandCompletion> =
    command { completion ->
      memScoped {
        Status.check(
          mln_map_set_style_light_json(
            state.requireLive().rawHandleValue,
            ByteStructs.bufferView(lightJson, this),
            completion,
          )
        )
      }
    }

  public actual fun setStyleLightProperty(
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_style_light_property(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(propertyName, this),
          ByteStructs.bufferView(value, this),
          completion,
        )
      )
    }
  }

  public actual fun styleLightProperty(propertyName: String): Deferred<ByteArray?> = memScoped {
    CompletionBridge.submit(
      ::bufferCompletion,
      { completion ->
        mln_map_get_style_light_property(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(propertyName, this),
          completion,
        )
      },
    )
  }

  public actual fun setStyleTransitionOptions(
    options: StyleTransitionOptions
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_style_transition_options(
          state.requireLive().rawHandleValue,
          StyleStructs.styleTransitionOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun styleTransitionOptions(): Deferred<StyleTransitionOptions> =
    CompletionBridge.submit(
      { result ->
        StyleStructs.styleTransitionOptions(
          result.pointed.value!!
            .reinterpret<org.maplibre.nativeffi.internal.c.mln_style_transition_options>()
            .pointed
        )
      },
      { completion ->
        mln_map_get_style_transition_options(state.requireLive().rawHandleValue, completion)
      },
    )

  public actual fun setLayerProperty(
    layerId: String,
    propertyName: String,
    value: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_layer_property(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(propertyName, this),
          ByteStructs.bufferView(value, this),
          completion,
        )
      )
    }
  }

  public actual fun layerProperty(layerId: String, propertyName: String): Deferred<ByteArray?> =
    memScoped {
      CompletionBridge.submit(
        ::bufferCompletion,
        { completion ->
          mln_map_get_layer_property(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            CoreStructs.stringView(propertyName, this),
            completion,
          )
        },
      )
    }

  public actual fun setLayerFilter(
    layerId: String,
    filter: ByteArray,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_layer_filter(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          ByteStructs.bufferViewPointer(filter, this),
          completion,
        )
      )
    }
  }

  public actual fun clearLayerFilter(layerId: String): Deferred<CommandCompletion> =
    command { completion ->
      memScoped {
        Status.check(
          mln_map_set_layer_filter(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            null,
            completion,
          )
        )
      }
    }

  public actual fun layerFilter(layerId: String): Deferred<ByteArray?> = memScoped {
    CompletionBridge.submit(
      ::bufferCompletion,
      { completion ->
        mln_map_get_layer_filter(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          completion,
        )
      },
    )
  }

  public actual fun setLayerSourceLayer(
    layerId: String,
    sourceLayer: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_layer_source_layer(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(sourceLayer, this),
          completion,
        )
      )
    }
  }

  public actual fun layerSourceLayer(layerId: String): Deferred<String> = memScoped {
    CompletionBridge.submit(
      { result -> checkNotNull(bufferCompletion(result)).decodeToString() },
      { completion ->
        mln_map_copy_layer_source_layer(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          completion,
        )
      },
    )
  }

  public actual fun setLayerSourceId(
    layerId: String,
    sourceId: String,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_layer_source_id(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          CoreStructs.stringView(sourceId, this),
          completion,
        )
      )
    }
  }

  public actual fun layerSourceId(layerId: String): Deferred<String> = memScoped {
    CompletionBridge.submit(
      { result -> checkNotNull(bufferCompletion(result)).decodeToString() },
      { completion ->
        mln_map_copy_layer_source_id(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          completion,
        )
      },
    )
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

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double): Deferred<CommandCompletion> =
    command { completion ->
      memScoped {
        Status.check(
          mln_map_set_layer_min_zoom(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            minZoom,
            completion,
          )
        )
      }
    }

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double): Deferred<CommandCompletion> =
    command { completion ->
      memScoped {
        Status.check(
          mln_map_set_layer_max_zoom(
            state.requireLive().rawHandleValue,
            CoreStructs.stringView(layerId, this),
            maxZoom,
            completion,
          )
        )
      }
    }

  public actual fun setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility,
  ): Deferred<CommandCompletion> = command { completion ->
    memScoped {
      Status.check(
        mln_map_set_layer_visibility(
          state.requireLive().rawHandleValue,
          CoreStructs.stringView(layerId, this),
          visibility.nativeValue.toUInt(),
          completion,
        )
      )
    }
  }

  public actual fun requestRepaint(): Deferred<CommandCompletion> = command { completion ->
    Status.check(mln_map_request_repaint(state.requireLive().rawHandleValue, completion))
  }

  public actual fun requestStillImage(): Deferred<CommandCompletion> = command { completion ->
    Status.check(mln_map_request_still_image(state.requireLive().rawHandleValue, completion))
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

  public actual fun setDebugOptions(options: Set<DebugOption>): Deferred<CommandCompletion> =
    command { completion ->
      Status.check(
        mln_map_set_debug_options(
          state.requireLive().rawHandleValue,
          MapStructs.debugOptionMask(options),
          completion,
        )
      )
    }

  public actual fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion> =
    command { completion ->
      Status.check(
        mln_map_set_rendering_stats_view_enabled(
          state.requireLive().rawHandleValue,
          enabled,
          completion,
        )
      )
    }

  public actual fun setViewportOptions(options: ViewportOptions): Deferred<CommandCompletion> =
    memScoped {
      command { completion ->
        Status.check(
          mln_map_set_viewport_options(
            state.requireLive().rawHandleValue,
            MapStructs.viewportOptions(options, this),
            completion,
          )
        )
      }
    }

  public actual fun setTileOptions(options: TileOptions): Deferred<CommandCompletion> = memScoped {
    command { completion ->
      Status.check(
        mln_map_set_tile_options(
          state.requireLive().rawHandleValue,
          MapStructs.tileOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun setBounds(options: BoundOptions): Deferred<CommandCompletion> = memScoped {
    command { completion ->
      Status.check(
        mln_map_set_bounds(
          state.requireLive().rawHandleValue,
          MapStructs.boundOptions(options, this),
          completion,
        )
      )
    }
  }

  public actual fun setFreeCameraOptions(options: FreeCameraOptions): Deferred<CommandCompletion> =
    memScoped {
      command { completion ->
        Status.check(
          mln_map_set_free_camera_options(
            state.requireLive().rawHandleValue,
            MapStructs.freeCameraOptions(options, this),
            completion,
          )
        )
      }
    }

  public actual fun resize(size: MapSize): Deferred<CommandCompletion> = memScoped {
    val extent = alloc<mln_logical_extent>()
    extent.width = size.width.toUInt()
    extent.height = size.height.toUInt()
    extent.scale_factor = size.scaleFactor
    command { completion ->
      Status.check(
        mln_map_resize(state.requireLive().rawHandleValue, extent.readValue(), completion)
      )
    }
  }

  public actual fun cameraSnapshot(): CameraSnapshot = memScoped {
    val outCamera = mln_camera_options_default().getPointer(this)
    val outGeneration = alloc<ULongVar>()
    Status.check(
      mln_map_camera_snapshot_get(state.requireLive().rawHandleValue, outCamera, outGeneration.ptr)
    )
    CameraSnapshot(outGeneration.value.toLong(), MapStructs.cameraOptions(outCamera.pointed))
  }

  public actual fun updateCamera(update: CameraUpdate): Deferred<CommandCompletion> = memScoped {
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
    command { completion ->
      Status.check(
        mln_map_update_camera(state.requireLive().rawHandleValue, nativeUpdate, completion)
      )
    }
  }

  public actual fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion> = memScoped {
    val nativeDelta = mln_camera_delta_default().getPointer(this)
    nativeDelta.pointed.kind = delta.kind.nativeValue.toUInt()
    nativeDelta.pointed.offset.x = delta.offset.x
    nativeDelta.pointed.offset.y = delta.offset.y
    nativeDelta.pointed.amount = delta.amount
    nativeDelta.pointed.has_anchor = delta.anchor != null
    delta.anchor?.let {
      nativeDelta.pointed.anchor.x = it.x
      nativeDelta.pointed.anchor.y = it.y
    }
    val animation = MapStructs.animationOptions(delta.animation, this).pointed
    nativeDelta.pointed.animation.size = animation.size
    nativeDelta.pointed.animation.fields = animation.fields
    nativeDelta.pointed.animation.duration_ms = animation.duration_ms
    nativeDelta.pointed.animation.velocity = animation.velocity
    nativeDelta.pointed.animation.min_zoom = animation.min_zoom
    nativeDelta.pointed.animation.easing.x1 = animation.easing.x1
    nativeDelta.pointed.animation.easing.y1 = animation.easing.y1
    nativeDelta.pointed.animation.easing.x2 = animation.easing.x2
    nativeDelta.pointed.animation.easing.y2 = animation.easing.y2
    nativeDelta.pointed.animation.transition_id = animation.transition_id
    command { completion ->
      Status.check(
        mln_map_apply_camera_delta(state.requireLive().rawHandleValue, nativeDelta, completion)
      )
    }
  }

  public actual fun queryCamera(): Deferred<CameraSnapshot> =
    CompletionBridge.submit(
      { completion ->
        val result = completion.pointed.value!!.reinterpret<mln_camera_query_result>().pointed
        CameraSnapshot(result.generation.toLong(), MapStructs.cameraOptions(result.camera))
      },
      { completion -> mln_map_camera_query(state.requireLive().rawHandleValue, completion) },
    )

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

  public actual fun createProjection(): Deferred<MapProjectionHandle> =
    CompletionBridge.submit(
      { result ->
        MapProjectionHandle(
          result.pointed.value!!
            .reinterpret<ULongVar>()
            .pointed
            .value
            .asHandle("mln_map_projection_create", ::mapProjectionHandle)
        )
      },
      { completion -> mln_map_projection_create(state.requireLive().rawHandleValue, completion) },
    )

  public actual fun close(): Deferred<Unit> {
    if (!state.beginClose()) return kotlinx.coroutines.CompletableDeferred(Unit)
    val completed =
      try {
        CompletionBridge.unitChecked { completion ->
          mln_map_release(state.handleForClose().rawHandleValue, completion)
        }
      } catch (error: Throwable) {
        state.abortClose()
        throw error
      }
    state.completeClose { runtime.unregisterMap(this) }
    return completed
  }

  public actual val isClosed: Boolean
    get() = state.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  internal fun nativeHandle(): NativeMap = state.requireLive()

  internal fun nativeHandleId(): Long = state.handleId()

  private fun checkedInt(value: ULong, name: String): Int {
    require(value <= Int.MAX_VALUE.toULong()) { "$name exceeds Int.MAX_VALUE" }
    return value.toInt()
  }

  public actual companion object {
    public actual fun create(runtime: RuntimeHandle, options: MapOptions): Deferred<MapHandle> =
      memScoped {
        CompletionBridge.submit(
          { result ->
            MapHandle(
                runtime,
                result.pointed.value!!
                  .reinterpret<ULongVar>()
                  .pointed
                  .value
                  .asHandle("mln_map_create", ::mapHandle),
                options.eventMask,
              )
              .also(runtime::registerMap)
          },
          { completion ->
            mln_map_create(
              runtime.nativeHandle().rawHandleValue,
              mapOptions(options, this),
              completion,
            )
          },
        )
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
}

@OptIn(ExperimentalForeignApi::class)
private fun bufferCompletion(result: CPointer<mln_completion_result>): ByteArray? {
  if (result.pointed.value_count == 0uL) return null
  return ByteStructs.copyBufferView(result.pointed.value!!.reinterpret<mln_buffer_view>().pointed)
}

@OptIn(ExperimentalForeignApi::class)
private fun stringViewsCompletion(result: CPointer<mln_completion_result>): List<String> {
  val count = result.pointed.value_count
  if (count == 0uL) return emptyList()
  require(count <= Int.MAX_VALUE.toULong()) { "string count exceeds Int.MAX_VALUE" }
  val values = result.pointed.value!!.reinterpret<mln_buffer_view>()
  return List(count.toInt()) { index -> CoreStructs.stringView(values[index]) }
}

@OptIn(ExperimentalForeignApi::class)
private inline fun command(
  crossinline call: (CPointer<mln_completion>) -> Unit
): Deferred<CommandCompletion> = CompletionBridge.command {
  call(it)
  0
}
