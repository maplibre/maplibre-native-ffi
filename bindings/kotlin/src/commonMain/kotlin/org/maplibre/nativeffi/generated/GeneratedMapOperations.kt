// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.internal.async.CompletionBridge
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*
import org.maplibre.nativeffi.runtime.CommandCompletion

public abstract class GeneratedMapOperations internal constructor() {
  internal abstract val binding: HandleStateCore
  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  public fun addColorReliefLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_color_relief_layer") {
      check(
        C.mln_map_add_color_relief_layer(
          handle,
          view(layerId),
          view(sourceId),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  public fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_custom_geometry_source", bindingCallbacks) {
      check(
        C.mln_map_add_custom_geometry_source(
          handle,
          view(sourceId),
          writeCustomGeometrySourceOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  public fun addCustomMvtVectorSource(
    sourceId: String,
    options: CustomMvtVectorSourceOptions,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_custom_mvt_vector_source", bindingCallbacks) {
      check(
        C.mln_map_add_custom_mvt_vector_source(
          handle,
          view(sourceId),
          writeCustomMvtVectorSourceOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  public fun addGeojsonSourceData(
    sourceId: String,
    data: GeojsonSourceDataHandle,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_geojson_source_data") {
      check(
        C.mln_map_add_geojson_source_data(
          handle,
          view(sourceId),
          data.binding.handle(),
          completion,
          diagnostic,
        )
      )
    }

  public fun addGeojsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeojsonSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_geojson_source_url") {
      check(
        C.mln_map_add_geojson_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeGeojsonSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun addHillshadeLayer(
    layerId: String,
    sourceId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_hillshade_layer") {
      check(
        C.mln_map_add_hillshade_layer(
          handle,
          view(layerId),
          view(sourceId),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  public fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_image_source_image") {
      check(
        C.mln_map_add_image_source_image(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          writePremultipliedRgba8Image(image),
          completion,
          diagnostic,
        )
      )
    }

  public fun addImageSourceUrl(
    sourceId: String,
    coordinates: List<LatLng>,
    url: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_image_source_url") {
      check(
        C.mln_map_add_image_source_url(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          view(url),
          completion,
          diagnostic,
        )
      )
    }

  public fun addLocationIndicatorLayer(
    layerId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_location_indicator_layer") {
      check(
        C.mln_map_add_location_indicator_layer(
          handle,
          view(layerId),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  public fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_raster_dem_source_tiles") {
      check(
        C.mln_map_add_raster_dem_source_tiles(
          handle,
          view(sourceId),
          array(tiles, 2 * NativeMemory.addressSize, NativeMemory.addressSize) { at, item ->
            putView(at, item)
          },
          tiles.size.toLong(),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_raster_dem_source_url") {
      check(
        C.mln_map_add_raster_dem_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_raster_source_tiles") {
      check(
        C.mln_map_add_raster_source_tiles(
          handle,
          view(sourceId),
          array(tiles, 2 * NativeMemory.addressSize, NativeMemory.addressSize) { at, item ->
            putView(at, item)
          },
          tiles.size.toLong(),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun addRasterSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_raster_source_url") {
      check(
        C.mln_map_add_raster_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun addStyleLayerJson(
    layerJson: ByteArray,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_style_layer_json") {
      check(
        C.mln_map_add_style_layer_json(
          handle,
          view(layerJson),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  public fun addStyleSourceJson(
    sourceId: String,
    sourceJson: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_style_source_json") {
      check(
        C.mln_map_add_style_source_json(
          handle,
          view(sourceId),
          view(sourceJson),
          completion,
          diagnostic,
        )
      )
    }

  public fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_vector_source_tiles") {
      check(
        C.mln_map_add_vector_source_tiles(
          handle,
          view(sourceId),
          array(tiles, 2 * NativeMemory.addressSize, NativeMemory.addressSize) { at, item ->
            putView(at, item)
          },
          tiles.size.toLong(),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun addVectorSourceUrl(
    sourceId: String,
    url: String,
    options: StyleTileSourceOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_add_vector_source_url") {
      check(
        C.mln_map_add_vector_source_url(
          handle,
          view(sourceId),
          view(url),
          options?.let { writeStyleTileSourceOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun applyCameraDelta(delta: CameraDelta): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_apply_camera_delta") {
      check(C.mln_map_apply_camera_delta(handle, writeCameraDelta(delta), completion, diagnostic))
    }

  public fun cameraForGeometry(
    geometry: ByteArray,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(
      this,
      binding,
      "mln_map_camera_for_geometry",
      { result -> readCameraOptions(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_camera_for_geometry(
          handle,
          view(geometry),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(
      this,
      binding,
      "mln_map_camera_for_lat_lng_bounds",
      { result -> readCameraOptions(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_camera_for_lat_lng_bounds(
          handle,
          writeLatLngBounds(bounds),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions? = null,
  ): Deferred<CameraOptions> =
    nativeSubmit(
      this,
      binding,
      "mln_map_camera_for_lat_lngs",
      { result -> readCameraOptions(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_camera_for_lat_lngs(
          handle,
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          fitOptions?.let { writeCameraFitOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun cameraQuery(): Deferred<CameraQueryResult> =
    nativeSubmit(
      this,
      binding,
      "mln_map_camera_query",
      { result -> readCameraQueryResult(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_camera_query(handle, completion, diagnostic))
    }

  public fun cameraSnapshotGet(): MapCameraSnapshotGetResult =
    nativeCall(this, binding, "mln_map_camera_snapshot_get") {
      val out0 = allocate(120, 8).also { writeU32(it, 120.toUInt()) }
      val out1 = allocate(8)
      check(C.mln_map_camera_snapshot_get(handle, out0, out1, diagnostic))
      MapCameraSnapshotGetResult(camera = readCameraOptions(out0), generation = readU64(out1))
    }

  public fun cancelTransitions(): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_cancel_transitions") {
      check(C.mln_map_cancel_transitions(handle, completion, diagnostic))
    }

  public fun copyLayerSourceId(layerId: String): Deferred<String?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_copy_layer_source_id",
      { result -> readViewString(CompletionBridge.value(result)).takeIf { it.isNotEmpty() } },
    ) {
      check(C.mln_map_copy_layer_source_id(handle, view(layerId), completion, diagnostic))
    }

  public fun copyLayerSourceLayer(layerId: String): Deferred<String?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_copy_layer_source_layer",
      { result -> readViewString(CompletionBridge.value(result)).takeIf { it.isNotEmpty() } },
    ) {
      check(C.mln_map_copy_layer_source_layer(handle, view(layerId), completion, diagnostic))
    }

  public fun copyStyleImagePremultipliedRgba8(imageId: String): Deferred<ByteArray?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_copy_style_image_premultiplied_rgba8",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readView(CompletionBridge.value(result))
      },
    ) {
      check(
        C.mln_map_copy_style_image_premultiplied_rgba8(
          handle,
          view(imageId),
          completion,
          diagnostic,
        )
      )
    }

  public fun copyStyleImageStretches(imageId: String): Deferred<StyleImageStretchesResult?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_copy_style_image_stretches",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleImageStretchesResult(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_copy_style_image_stretches(handle, view(imageId), completion, diagnostic))
    }

  public fun copyStyleSourceAttribution(sourceId: String): Deferred<String?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_copy_style_source_attribution",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readViewString(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_copy_style_source_attribution(handle, view(sourceId), completion, diagnostic))
    }

  public fun copyStyleSourceUrl(sourceId: String): Deferred<String?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_copy_style_source_url",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readViewString(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_copy_style_source_url(handle, view(sourceId), completion, diagnostic))
    }

  public fun dispose(): Unit =
    nativeClose(this, binding, "mln_map_dispose") { check(C.mln_map_dispose(handle, diagnostic)) }

  public fun dumpDebugLogs(): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_dump_debug_logs") {
      check(C.mln_map_dump_debug_logs(handle, completion, diagnostic))
    }

  public fun getFeatureState(selector: FeatureStateSelector): Deferred<ByteArray> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_feature_state",
      { result -> readView(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_get_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          completion,
          diagnostic,
        )
      )
    }

  public fun getGlobalState(): Deferred<ByteArray> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_global_state",
      { result -> readView(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_get_global_state(handle, completion, diagnostic))
    }

  public fun getImageSourceCoordinates(sourceId: String): Deferred<List<LatLng>?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_image_source_coordinates",
      { result ->
        if (CompletionBridge.valuePointer(result) == 0L) null
        else
          readArray(
            CompletionBridge.valuePointer(result),
            CompletionBridge.valueCount(result),
            16.toLong(),
          ) {
            readLatLng(it)
          }
      },
    ) {
      check(C.mln_map_get_image_source_coordinates(handle, view(sourceId), completion, diagnostic))
    }

  public fun getLayerFilter(layerId: String): Deferred<ByteArray?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_layer_filter",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readView(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_layer_filter(handle, view(layerId), completion, diagnostic))
    }

  public fun getLayerProperty(layerId: String, propertyName: String): Deferred<ByteArray?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_layer_property",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readView(CompletionBridge.value(result))
      },
    ) {
      check(
        C.mln_map_get_layer_property(
          handle,
          view(layerId),
          view(propertyName),
          completion,
          diagnostic,
        )
      )
    }

  public fun getStyleImageInfo(imageId: String): Deferred<StyleImageResult?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_image_info",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleImageResult(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_image_info(handle, view(imageId), completion, diagnostic))
    }

  public fun getStyleLayerInfo(layerId: String): Deferred<StyleLayerResult?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_layer_info",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleLayerResult(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_layer_info(handle, view(layerId), completion, diagnostic))
    }

  public fun getStyleLayerJson(layerId: String): Deferred<ByteArray?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_layer_json",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readView(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_layer_json(handle, view(layerId), completion, diagnostic))
    }

  public fun getStyleLightProperty(propertyName: String): Deferred<ByteArray?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_light_property",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readView(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_light_property(handle, view(propertyName), completion, diagnostic))
    }

  public fun getStyleSourceInfo(sourceId: String): Deferred<StyleSourceResult?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_source_info",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleSourceResult(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_source_info(handle, view(sourceId), completion, diagnostic))
    }

  public fun getStyleSourceTileUrls(sourceId: String): Deferred<StyleSourceTileUrlsResult?> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_source_tile_urls",
      { result ->
        if (CompletionBridge.valueCount(result) == 0uL) null
        else readStyleSourceTileUrlsResult(CompletionBridge.value(result))
      },
    ) {
      check(C.mln_map_get_style_source_tile_urls(handle, view(sourceId), completion, diagnostic))
    }

  public fun getStyleTransitionOptions(): Deferred<StyleTransitionOptions> =
    nativeSubmit(
      this,
      binding,
      "mln_map_get_style_transition_options",
      { result -> readStyleTransitionOptions(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_get_style_transition_options(handle, completion, diagnostic))
    }

  public fun invalidateCustomGeometrySourceRegion(
    sourceId: String,
    bounds: LatLngBounds,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_invalidate_custom_geometry_source_region") {
      check(
        C.mln_map_invalidate_custom_geometry_source_region(
          handle,
          view(sourceId),
          writeLatLngBounds(bounds),
          completion,
          diagnostic,
        )
      )
    }

  public fun invalidateCustomGeometrySourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_invalidate_custom_geometry_source_tile") {
      check(
        C.mln_map_invalidate_custom_geometry_source_tile(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          completion,
          diagnostic,
        )
      )
    }

  public fun invalidateCustomMvtVectorSourceTile(
    sourceId: String,
    tileId: CanonicalTileId,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_invalidate_custom_mvt_vector_source_tile") {
      check(
        C.mln_map_invalidate_custom_mvt_vector_source_tile(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          completion,
          diagnostic,
        )
      )
    }

  public fun latLngBoundsForCamera(camera: CameraOptions): Deferred<LatLngBounds> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lng_bounds_for_camera",
      { result -> readLatLngBounds(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_lat_lng_bounds_for_camera(
          handle,
          writeCameraOptions(camera),
          completion,
          diagnostic,
        )
      )
    }

  public fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): Deferred<LatLngBounds> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lng_bounds_for_camera_unwrapped",
      { result -> readLatLngBounds(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_lat_lng_bounds_for_camera_unwrapped(
          handle,
          writeCameraOptions(camera),
          completion,
          diagnostic,
        )
      )
    }

  public fun latLngForPixel(point: ScreenPoint): Deferred<LatLng> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lng_for_pixel",
      { result -> readLatLng(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_lat_lng_for_pixel(handle, writeScreenPoint(point), completion, diagnostic))
    }

  public fun latLngForPixelUnwrapped(point: ScreenPoint): Deferred<LatLng> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lng_for_pixel_unwrapped",
      { result -> readLatLng(CompletionBridge.value(result)) },
    ) {
      check(
        C.mln_map_lat_lng_for_pixel_unwrapped(
          handle,
          writeScreenPoint(point),
          completion,
          diagnostic,
        )
      )
    }

  public fun latLngsForPixels(points: List<ScreenPoint>): Deferred<List<LatLng>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lngs_for_pixels",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          16.toLong(),
        ) {
          readLatLng(it)
        }
      },
    ) {
      check(
        C.mln_map_lat_lngs_for_pixels(
          handle,
          array(points, 16, 8) { at, item -> putScreenPoint(at, item) },
          points.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  public fun latLngsForPixelsUnwrapped(points: List<ScreenPoint>): Deferred<List<LatLng>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_lat_lngs_for_pixels_unwrapped",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          16.toLong(),
        ) {
          readLatLng(it)
        }
      },
    ) {
      check(
        C.mln_map_lat_lngs_for_pixels_unwrapped(
          handle,
          array(points, 16, 8) { at, item -> putScreenPoint(at, item) },
          points.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  public fun listStyleLayerIds(): Deferred<List<String>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_list_style_layer_ids",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          2 * NativeMemory.addressSize.toLong(),
        ) {
          readViewString(it)
        }
      },
    ) {
      check(C.mln_map_list_style_layer_ids(handle, completion, diagnostic))
    }

  public fun listStyleLayers(): Deferred<List<StyleLayerEntry>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_list_style_layers",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          w(36, 72).toLong(),
        ) {
          readStyleLayerEntry(it)
        }
      },
    ) {
      check(C.mln_map_list_style_layers(handle, completion, diagnostic))
    }

  public fun listStyleSourceIds(): Deferred<List<String>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_list_style_source_ids",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          2 * NativeMemory.addressSize.toLong(),
        ) {
          readViewString(it)
        }
      },
    ) {
      check(C.mln_map_list_style_source_ids(handle, completion, diagnostic))
    }

  public fun loadedStyleJson(): Deferred<ByteArray> =
    nativeSubmit(
      this,
      binding,
      "mln_map_loaded_style_json",
      { result -> readView(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_loaded_style_json(handle, completion, diagnostic))
    }

  public fun metersPerPixelAtLatitude(latitude: Double): Deferred<Double> =
    nativeSubmit(
      this,
      binding,
      "mln_map_meters_per_pixel_at_latitude",
      { result -> readF64(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_meters_per_pixel_at_latitude(handle, latitude, completion, diagnostic))
    }

  public fun moveStyleLayer(
    layerId: String,
    beforeLayerId: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_move_style_layer") {
      check(
        C.mln_map_move_style_layer(
          handle,
          view(layerId),
          view(beforeLayerId ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  public fun pixelForLatLng(coordinate: LatLng): Deferred<ScreenPoint> =
    nativeSubmit(
      this,
      binding,
      "mln_map_pixel_for_lat_lng",
      { result -> readScreenPoint(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_pixel_for_lat_lng(handle, writeLatLng(coordinate), completion, diagnostic))
    }

  public fun pixelsForLatLngs(coordinates: List<LatLng>): Deferred<List<ScreenPoint>> =
    nativeSubmit(
      this,
      binding,
      "mln_map_pixels_for_lat_lngs",
      { result ->
        readArray(
          CompletionBridge.valuePointer(result),
          CompletionBridge.valueCount(result),
          16.toLong(),
        ) {
          readScreenPoint(it)
        }
      },
    ) {
      check(
        C.mln_map_pixels_for_lat_lngs(
          handle,
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  public fun projectionCreate(): Deferred<MapProjectionHandle> =
    nativeSubmitOwned(
      this,
      binding,
      "mln_map_projection_create",
      { MapProjectionHandle(it) },
      GeneratedOwnerDisposal::mapProjection,
      { it.close() },
    ) {
      check(C.mln_map_projection_create(handle, completion, diagnostic))
    }

  public fun release(): Deferred<Unit> =
    nativeRetire(this, binding, "mln_map_release") {
      check(C.mln_map_release(handle, completion, diagnostic))
    }

  public fun removeFeatureState(selector: FeatureStateSelector): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_remove_feature_state") {
      check(
        C.mln_map_remove_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          completion,
          diagnostic,
        )
      )
    }

  public fun removeStyleImage(imageId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_remove_style_image") {
      check(C.mln_map_remove_style_image(handle, view(imageId), completion, diagnostic))
    }

  public fun removeStyleLayer(layerId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_remove_style_layer") {
      check(C.mln_map_remove_style_layer(handle, view(layerId), completion, diagnostic))
    }

  public fun removeStyleSource(sourceId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_remove_style_source") {
      check(C.mln_map_remove_style_source(handle, view(sourceId), completion, diagnostic))
    }

  public fun requestRepaint(): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_request_repaint") {
      check(C.mln_map_request_repaint(handle, completion, diagnostic))
    }

  public fun requestStillImage(): Deferred<Unit> =
    nativeUnit(this, binding, "mln_map_request_still_image") {
      check(C.mln_map_request_still_image(handle, completion, diagnostic))
    }

  public fun resize(extent: LogicalExtent): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_resize") {
      check(C.mln_map_resize(handle, writeLogicalExtent(extent), completion, diagnostic))
    }

  public fun setBounds(options: BoundOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_bounds") {
      check(C.mln_map_set_bounds(handle, writeBoundOptions(options), completion, diagnostic))
    }

  public fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_custom_geometry_source_tile_data") {
      check(
        C.mln_map_set_custom_geometry_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(data),
          completion,
          diagnostic,
        )
      )
    }

  public fun setCustomMvtVectorSourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_custom_mvt_vector_source_tile_data") {
      check(
        C.mln_map_set_custom_mvt_vector_source_tile_data(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(data),
          completion,
          diagnostic,
        )
      )
    }

  public fun setCustomMvtVectorSourceTileError(
    sourceId: String,
    tileId: CanonicalTileId,
    message: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_custom_mvt_vector_source_tile_error") {
      check(
        C.mln_map_set_custom_mvt_vector_source_tile_error(
          handle,
          view(sourceId),
          writeCanonicalTileId(tileId),
          view(message),
          completion,
          diagnostic,
        )
      )
    }

  public fun setDebugOptions(options: MapDebugOption): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_debug_options") {
      check(C.mln_map_set_debug_options(handle, options.rawValue.toInt(), completion, diagnostic))
    }

  public fun setEventMask(mask: RuntimeEventMask): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_event_mask") {
      check(C.mln_map_set_event_mask(handle, mask.rawValue.toLong(), completion, diagnostic))
    }

  public fun setFeatureState(
    selector: FeatureStateSelector,
    state: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_feature_state") {
      check(
        C.mln_map_set_feature_state(
          handle,
          writeFeatureStateSelector(selector),
          view(state),
          completion,
          diagnostic,
        )
      )
    }

  public fun setFreeCameraOptions(options: FreeCameraOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_free_camera_options") {
      check(
        C.mln_map_set_free_camera_options(
          handle,
          writeFreeCameraOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  public fun setGeojsonSourceData(
    sourceId: String,
    data: GeojsonSourceDataHandle,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_geojson_source_data") {
      check(
        C.mln_map_set_geojson_source_data(
          handle,
          view(sourceId),
          data.binding.handle(),
          completion,
          diagnostic,
        )
      )
    }

  public fun setGeojsonSourceSynchronousTiling(
    sourceId: String,
    enabled: Boolean,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_geojson_source_synchronous_tiling") {
      check(
        C.mln_map_set_geojson_source_synchronous_tiling(
          handle,
          view(sourceId),
          enabled,
          completion,
          diagnostic,
        )
      )
    }

  public fun setGeojsonSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_geojson_source_url") {
      check(
        C.mln_map_set_geojson_source_url(handle, view(sourceId), view(url), completion, diagnostic)
      )
    }

  public fun setGlobalStateProperty(
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_global_state_property") {
      check(
        C.mln_map_set_global_state_property(
          handle,
          view(propertyName),
          view(valueValue),
          completion,
          diagnostic,
        )
      )
    }

  public fun setImageSourceCoordinates(
    sourceId: String,
    coordinates: List<LatLng>,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_image_source_coordinates") {
      check(
        C.mln_map_set_image_source_coordinates(
          handle,
          view(sourceId),
          array(coordinates, 16, 8) { at, item -> putLatLng(at, item) },
          coordinates.size.toLong(),
          completion,
          diagnostic,
        )
      )
    }

  public fun setImageSourceImage(
    sourceId: String,
    image: PremultipliedRgba8Image,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_image_source_image") {
      check(
        C.mln_map_set_image_source_image(
          handle,
          view(sourceId),
          writePremultipliedRgba8Image(image),
          completion,
          diagnostic,
        )
      )
    }

  public fun setImageSourceUrl(sourceId: String, url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_image_source_url") {
      check(
        C.mln_map_set_image_source_url(handle, view(sourceId), view(url), completion, diagnostic)
      )
    }

  public fun setLayerFilter(
    layerId: String,
    filter: ByteArray? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_layer_filter") {
      check(
        C.mln_map_set_layer_filter(
          handle,
          view(layerId),
          filter?.let { view(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun setLayerMaxZoom(layerId: String, maxZoom: Double): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_layer_max_zoom") {
      check(C.mln_map_set_layer_max_zoom(handle, view(layerId), maxZoom, completion, diagnostic))
    }

  public fun setLayerMinZoom(layerId: String, minZoom: Double): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_layer_min_zoom") {
      check(C.mln_map_set_layer_min_zoom(handle, view(layerId), minZoom, completion, diagnostic))
    }

  public fun setLayerProperty(
    layerId: String,
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_layer_property") {
      check(
        C.mln_map_set_layer_property(
          handle,
          view(layerId),
          view(propertyName),
          view(valueValue),
          completion,
          diagnostic,
        )
      )
    }

  public fun setLayerSourceId(layerId: String, sourceId: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_layer_source_id") {
      check(
        C.mln_map_set_layer_source_id(handle, view(layerId), view(sourceId), completion, diagnostic)
      )
    }

  public fun setLayerSourceLayer(
    layerId: String,
    sourceLayer: String? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_layer_source_layer") {
      check(
        C.mln_map_set_layer_source_layer(
          handle,
          view(layerId),
          view(sourceLayer ?: ""),
          completion,
          diagnostic,
        )
      )
    }

  public fun setLayerVisibility(
    layerId: String,
    visibility: StyleLayerVisibility,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_layer_visibility") {
      check(
        C.mln_map_set_layer_visibility(
          handle,
          view(layerId),
          visibility.rawValue.toInt(),
          completion,
          diagnostic,
        )
      )
    }

  public fun setLocationIndicatorAccuracyRadius(
    layerId: String,
    radius: Double,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_location_indicator_accuracy_radius") {
      check(
        C.mln_map_set_location_indicator_accuracy_radius(
          handle,
          view(layerId),
          radius,
          completion,
          diagnostic,
        )
      )
    }

  public fun setLocationIndicatorBearing(
    layerId: String,
    bearing: Double,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_location_indicator_bearing") {
      check(
        C.mln_map_set_location_indicator_bearing(
          handle,
          view(layerId),
          bearing,
          completion,
          diagnostic,
        )
      )
    }

  public fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_location_indicator_image_name") {
      check(
        C.mln_map_set_location_indicator_image_name(
          handle,
          view(layerId),
          imageKind.rawValue.toInt(),
          view(imageId),
          completion,
          diagnostic,
        )
      )
    }

  public fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_location_indicator_location") {
      check(
        C.mln_map_set_location_indicator_location(
          handle,
          view(layerId),
          writeLatLng(coordinate),
          altitude,
          completion,
          diagnostic,
        )
      )
    }

  public fun setProjectionMode(mode: ProjectionMode): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_projection_mode") {
      check(
        C.mln_map_set_projection_mode(handle, writeProjectionMode(mode), completion, diagnostic)
      )
    }

  public fun setRenderingStatsViewEnabled(enabled: Boolean): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_rendering_stats_view_enabled") {
      check(C.mln_map_set_rendering_stats_view_enabled(handle, enabled, completion, diagnostic))
    }

  public fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions? = null,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_image") {
      check(
        C.mln_map_set_style_image(
          handle,
          view(imageId),
          writePremultipliedRgba8Image(image),
          options?.let { writeStyleImageOptions(it) } ?: 0L,
          completion,
          diagnostic,
        )
      )
    }

  public fun setStyleJson(json: ByteArray): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_json") {
      check(C.mln_map_set_style_json(handle, view(json), completion, diagnostic))
    }

  public fun setStyleLightJson(lightJson: ByteArray): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_light_json") {
      check(C.mln_map_set_style_light_json(handle, view(lightJson), completion, diagnostic))
    }

  public fun setStyleLightProperty(
    propertyName: String,
    valueValue: ByteArray,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_light_property") {
      check(
        C.mln_map_set_style_light_property(
          handle,
          view(propertyName),
          view(valueValue),
          completion,
          diagnostic,
        )
      )
    }

  public fun setStyleSourceVolatile(
    sourceId: String,
    isVolatile: Boolean,
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_source_volatile") {
      check(
        C.mln_map_set_style_source_volatile(
          handle,
          view(sourceId),
          isVolatile,
          completion,
          diagnostic,
        )
      )
    }

  public fun setStyleTransitionOptions(
    options: StyleTransitionOptions
  ): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_transition_options") {
      check(
        C.mln_map_set_style_transition_options(
          handle,
          writeStyleTransitionOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  public fun setStyleUrl(url: String): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_style_url") {
      check(C.mln_map_set_style_url(handle, cString(url), completion, diagnostic))
    }

  public fun setTileOptions(options: MapTileOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_tile_options") {
      check(
        C.mln_map_set_tile_options(handle, writeMapTileOptions(options), completion, diagnostic)
      )
    }

  public fun setViewportOptions(options: MapViewportOptions): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_set_viewport_options") {
      check(
        C.mln_map_set_viewport_options(
          handle,
          writeMapViewportOptions(options),
          completion,
          diagnostic,
        )
      )
    }

  public fun snapshotGet(): MapSnapshot =
    nativeCall(this, binding, "mln_map_snapshot_get") {
      val out = allocate(456, 8).also { writeU32(it, 456.toUInt()) }
      check(C.mln_map_snapshot_get(handle, out, diagnostic))
      readMapSnapshot(out)
    }

  public fun styleUrl(): Deferred<String> =
    nativeSubmit(
      this,
      binding,
      "mln_map_style_url",
      { result -> readViewString(CompletionBridge.value(result)) },
    ) {
      check(C.mln_map_style_url(handle, completion, diagnostic))
    }

  public fun updateCamera(update: CameraUpdate): Deferred<CommandCompletion> =
    nativeCommand(this, binding, "mln_map_update_camera") {
      check(C.mln_map_update_camera(handle, writeCameraUpdate(update), completion, diagnostic))
    }

  public fun metalBorrowedTextureAttach(
    descriptor: MetalBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_metal_borrowed_texture_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_metal_borrowed_texture_attach(
            handle,
            writeMetalBorrowedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun metalOwnedTextureAttach(
    descriptor: MetalOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_metal_owned_texture_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_metal_owned_texture_attach(
            handle,
            writeMetalOwnedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun metalSurfaceAttach(
    descriptor: MetalSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_metal_surface_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_metal_surface_attach(
            handle,
            writeMetalSurfaceDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun openglBorrowedTextureAttach(
    descriptor: OpenglBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_opengl_borrowed_texture_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_opengl_borrowed_texture_attach(
            handle,
            writeOpenglBorrowedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun openglOwnedTextureAttach(
    descriptor: OpenglOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_opengl_owned_texture_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_opengl_owned_texture_attach(
            handle,
            writeOpenglOwnedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun openglSurfaceAttach(
    descriptor: OpenglSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_opengl_surface_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_opengl_surface_attach(
            handle,
            writeOpenglSurfaceDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun vulkanBorrowedTextureAttach(
    descriptor: VulkanBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_vulkan_borrowed_texture_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_vulkan_borrowed_texture_attach(
            handle,
            writeVulkanBorrowedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun vulkanOwnedTextureAttach(
    descriptor: VulkanOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_vulkan_owned_texture_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_vulkan_owned_texture_attach(
            handle,
            writeVulkanOwnedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun vulkanSurfaceAttach(
    descriptor: VulkanSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_vulkan_surface_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_vulkan_surface_attach(
            handle,
            writeVulkanSurfaceDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun webgpuBorrowedTextureAttach(
    descriptor: WebgpuBorrowedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_webgpu_borrowed_texture_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_webgpu_borrowed_texture_attach(
            handle,
            writeWebgpuBorrowedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun webgpuOwnedTextureAttach(
    descriptor: WebgpuOwnedTextureDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_webgpu_owned_texture_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_webgpu_owned_texture_attach(
            handle,
            writeWebgpuOwnedTextureDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }

  public fun webgpuSurfaceAttach(
    descriptor: WebgpuSurfaceDescriptor,
    options: RenderSessionAttachOptions,
  ): RenderSessionAttachment =
    nativeCall(this, binding, "mln_webgpu_surface_attach") {
      val out = allocate(8)
      val ready = CompletionBridge.unitChecked { completion ->
        check(
          C.mln_webgpu_surface_attach(
            handle,
            writeWebgpuSurfaceDescriptor(descriptor),
            writeRenderSessionAttachOptions(options),
            out,
            completion,
            diagnostic,
          )
        )
      }
      RenderSessionAttachment(
        adopt(readI64(out), GeneratedOwnerDisposal::renderSession) {
            RenderSessionHandle(it, this@GeneratedMapOperations as MapHandle)
          }
          .let { accept(it, it.bindingCallbacks) { it.dispose() } },
        ready,
      )
    }
}
