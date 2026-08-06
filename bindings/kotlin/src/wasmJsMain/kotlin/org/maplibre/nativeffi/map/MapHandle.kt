package org.maplibre.nativeffi.map

import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraFitOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException
import org.maplibre.nativeffi.geo.CanonicalTileId
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.Geometry
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.LatLngBounds
import org.maplibre.nativeffi.geo.Quaternion
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.geo.Vec3
import org.maplibre.nativeffi.internal.lifecycle.HandleStateCore
import org.maplibre.nativeffi.internal.lifecycle.NativeMap
import org.maplibre.nativeffi.internal.lifecycle.NativeMapProjection
import org.maplibre.nativeffi.internal.lifecycle.NativeRenderSession
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.CameraMarshal
import org.maplibre.nativeffi.internal.wasm.CustomGeometryBridge
import org.maplibre.nativeffi.internal.wasm.GeoJsonMarshal
import org.maplibre.nativeffi.internal.wasm.GeometryMarshal
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.internal.wasm.JsonMarshal
import org.maplibre.nativeffi.internal.wasm.MapOptionsMarshal
import org.maplibre.nativeffi.internal.wasm.RenderMarshal
import org.maplibre.nativeffi.internal.wasm.SourceMarshal
import org.maplibre.nativeffi.internal.wasm.StyleMarshal
import org.maplibre.nativeffi.internal.wasm.generated.MlnAnimationOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnAnimationOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnCameraFitOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnCameraFitOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnCanonicalTileId
import org.maplibre.nativeffi.internal.wasm.generated.MlnFreeCameraOptionField
import org.maplibre.nativeffi.internal.wasm.generated.MlnFreeCameraOptions
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLng
import org.maplibre.nativeffi.internal.wasm.generated.MlnLatLngBounds
import org.maplibre.nativeffi.internal.wasm.generated.MlnOpenglOwnedTextureDescriptor
import org.maplibre.nativeffi.internal.wasm.generated.MlnProjectionMode
import org.maplibre.nativeffi.internal.wasm.generated.MlnProjectionModeField
import org.maplibre.nativeffi.internal.wasm.generated.MlnQuaternion
import org.maplibre.nativeffi.internal.wasm.generated.MlnScreenPoint
import org.maplibre.nativeffi.internal.wasm.generated.MlnStringView
import org.maplibre.nativeffi.internal.wasm.generated.MlnStyleSourceInfo
import org.maplibre.nativeffi.internal.wasm.generated.MlnUnitBezier
import org.maplibre.nativeffi.internal.wasm.generated.MlnVec3
import org.maplibre.nativeffi.internal.wasm.generated.mln_json_snapshot_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_json_snapshot_get
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_color_relief_layer
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_custom_geometry_source
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_geojson_source_data
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_geojson_source_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_hillshade_layer
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_image_source_image
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_image_source_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_location_indicator_layer
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_raster_dem_source_tiles
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_raster_dem_source_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_raster_source_tiles
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_raster_source_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_style_layer_json
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_style_source_json
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_vector_source_tiles
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_add_vector_source_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_camera_for_geometry
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_camera_for_lat_lng_bounds
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_camera_for_lat_lngs
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_cancel_transitions
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_copy_layer_source_id
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_copy_layer_source_layer
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_copy_loaded_style_json
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_copy_style_image_premultiplied_rgba8
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_copy_style_image_stretches
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_copy_style_source_attribution
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_copy_style_source_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_copy_style_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_create
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_dump_debug_logs
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_ease_to
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_fly_to
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_bounds
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_camera
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_debug_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_free_camera_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_image_source_coordinates
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_layer_filter
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_layer_max_zoom
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_layer_min_zoom
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_layer_property
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_layer_visibility
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_projection_mode
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_rendering_stats_view_enabled
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_size
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_style_image_info
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_style_layer_json
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_style_layer_type
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_style_light_property
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_style_source_info
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_style_source_tile_urls
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_style_source_type
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_style_transition_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_tile_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_get_viewport_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_invalidate_custom_geometry_source_region
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_invalidate_custom_geometry_source_tile
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_is_fully_loaded
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_is_gesture_in_progress
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_jump_to
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_lat_lng_bounds_for_camera
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_lat_lng_bounds_for_camera_unwrapped
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_lat_lng_for_pixel
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_lat_lngs_for_pixels
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_list_style_layer_ids
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_list_style_source_ids
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_move_by
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_move_by_animated
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_move_style_layer
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_pitch_by
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_pitch_by_animated
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_pixel_for_lat_lng
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_pixels_for_lat_lngs
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_projection_create
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_remove_style_image
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_remove_style_layer
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_remove_style_source
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_request_repaint
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_request_still_image
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_rotate_by
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_rotate_by_animated
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_scale_by
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_scale_by_animated
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_bounds
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_custom_geometry_source_tile_data
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_debug_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_free_camera_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_geojson_source_data
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_geojson_source_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_gesture_in_progress
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_image_source_coordinates
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_image_source_image
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_image_source_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_layer_filter
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_layer_max_zoom
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_layer_min_zoom
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_layer_property
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_layer_source_id
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_layer_source_layer
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_layer_visibility
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_location_indicator_accuracy_radius
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_location_indicator_bearing
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_location_indicator_image_name
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_location_indicator_location
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_projection_mode
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_rendering_stats_view_enabled
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_style_image
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_style_json
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_style_light_json
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_style_light_property
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_style_transition_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_style_url
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_tile_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_set_viewport_options
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_style_image_exists
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_style_layer_exists
import org.maplibre.nativeffi.internal.wasm.generated.mln_map_style_source_exists
import org.maplibre.nativeffi.internal.wasm.generated.mln_opengl_borrowed_texture_attach
import org.maplibre.nativeffi.internal.wasm.generated.mln_opengl_owned_texture_attach
import org.maplibre.nativeffi.internal.wasm.generated.mln_opengl_surface_attach
import org.maplibre.nativeffi.internal.wasm.generated.mln_style_id_list_count
import org.maplibre.nativeffi.internal.wasm.generated.mln_style_id_list_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_style_id_list_get
import org.maplibre.nativeffi.internal.wasm.generated.mln_style_string_list_count
import org.maplibre.nativeffi.internal.wasm.generated.mln_style_string_list_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_style_string_list_get
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.render.MetalBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.MetalOwnedTextureDescriptor
import org.maplibre.nativeffi.render.MetalSurfaceDescriptor
import org.maplibre.nativeffi.render.OpenGLBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLContextDescriptor
import org.maplibre.nativeffi.render.OpenGLOwnedTextureDescriptor
import org.maplibre.nativeffi.render.OpenGLSurfaceDescriptor
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.render.RenderSessionHandle
import org.maplibre.nativeffi.render.VulkanBorrowedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanOwnedTextureDescriptor
import org.maplibre.nativeffi.render.VulkanSurfaceDescriptor
import org.maplibre.nativeffi.render.WebglContext
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.CustomGeometrySourceOptions
import org.maplibre.nativeffi.style.GeoJsonSourceOptions
import org.maplibre.nativeffi.style.ImageStretch
import org.maplibre.nativeffi.style.LocationIndicatorImageKind
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType
import org.maplibre.nativeffi.style.StyleImage
import org.maplibre.nativeffi.style.StyleImageInfo
import org.maplibre.nativeffi.style.StyleImageOptions
import org.maplibre.nativeffi.style.StyleLayerVisibility
import org.maplibre.nativeffi.style.StyleTransitionOptions
import org.maplibre.nativeffi.style.TileSourceOptions

/** Bytes one C API handle occupies. Handles are 64-bit whatever a pointer is on this target. */
private const val HANDLE_BYTES = 8

/** Bytes a `size_t`, a `uint32_t`, a pointer, a `bool`, and a `double` occupy on wasm32. */
private const val SIZE_BYTES = 4
private const val POINTER_BYTES = 4
private const val BOOL_BYTES = 1
private const val DOUBLE_BYTES = 8

/** Coordinates an image source carries, which the C API fixes at the corners of a quad. */
private const val IMAGE_SOURCE_COORDINATE_COUNT = 4

/** The terminator a C string ends at, which is why one may not appear inside the text. */
private const val NUL = '\u0000'

/**
 * An owned map, on the thread the module gave this binding.
 *
 * That thread created the runtime this map belongs to, which is what makes it the map's owner
 * thread as far as the C API is concerned, so every call here is an ordinary synchronous call as on
 * every other platform.
 *
 * The browser build compiles one render backend, OpenGL against WebGL, and every render target that
 * backend has: a native surface, which here is the canvas the context is bound to, and both the
 * owned and the borrowed texture target. See the Metal and Vulkan members below for what compiling
 * one backend leaves unreachable.
 */
public actual class MapHandle
private constructor(private val runtime: RuntimeHandle, private val handle: NativeMap) :
  AutoCloseable {
  private val runtimeRetention = runtime.retainChild("MapHandle")
  private val core = HandleStateCore("MapHandle", handle.raw, runtime)

  /**
   * The tile callback registration behind each custom geometry source this map holds, by source id.
   *
   * A registration outlives the call that made it, because native keeps asking for tiles for as
   * long as the source is in the style. What ends it is the source ending: removal, a new style
   * that does not carry it, or this map closing.
   */
  private val customGeometrySources = mutableMapOf<String, CustomGeometryBridge>()

  private inline fun <T> live(body: () -> T): T {
    core.requireLive()
    return body()
  }

  public actual val isClosed: Boolean
    get() = core.isReleased()

  public actual fun runtime(): RuntimeHandle = runtime

  public actual fun setStyleUrl(url: String) {
    // One of the two calls here that takes a null-terminated C string rather than a string view, so
    // an embedded NUL would truncate the URL instead of being carried as a byte.
    requireValidCString(url, "url")
    live {
      Heap.withScratch(Heap.utf8Size(url)) { text ->
        Heap.storeUtf8(text, url)
        Status.check(mln_map_set_style_url(handle.raw, text.address))
      }
    }
  }

  public actual fun setStyleJson(json: String) {
    requireValidCString(json, "json")
    live {
      Heap.withScratch(Heap.utf8Size(json)) { text ->
        Heap.storeUtf8(text, json)
        Status.check(mln_map_set_style_json(handle.raw, text.address))
      }
      // The style this parsed replaces the one the sources were added to, so none of them exists
      // any more. A style set by URL loads later instead, and its registrations are released when
      // the load is reported; see `RuntimeHandle.pollEvent`.
      clearCustomGeometrySources()
    }
  }

  public actual fun loadedStyleJson(): String = copyMapText(::mln_map_copy_loaded_style_json)

  public actual fun styleUrl(): String = copyMapText(::mln_map_copy_style_url)

  public actual fun addStyleSourceJson(sourceId: String, sourceJson: JsonValue) {
    live { callWithIdAndJson(::mln_map_add_style_source_json, sourceId, sourceJson) }
  }

  public actual fun removeStyleSource(sourceId: String): Boolean {
    val removed = flagForId(::mln_map_remove_style_source, sourceId)
    // Only a source native really removed, because a refused removal leaves the source in the style
    // and it goes on asking for tiles.
    if (removed) customGeometrySources.remove(sourceId)?.close()
    return removed
  }

  public actual fun styleSourceExists(sourceId: String): Boolean =
    flagForId(::mln_map_style_source_exists, sourceId)

  public actual fun styleSourceType(sourceId: String): SourceType? = live {
    withArena(bytes(stringViewBytes(sourceId), blockBytes(SIZE_BYTES), blockBytes(BOOL_BYTES))) {
      arena ->
      val view = writeStringView(arena, sourceId)
      val type = allocate(arena, SIZE_BYTES)
      val found = allocate(arena, BOOL_BYTES)
      Status.check(
        mln_map_get_style_source_type(handle.raw, view.address, type.address, found.address)
      )
      if (isSet(found)) SourceMarshal.readSourceType(type) else null
    }
  }

  public actual fun styleSourceInfo(sourceId: String): SourceInfo? = live {
    withArena(
      bytes(
        stringViewBytes(sourceId),
        blockBytes(SourceMarshal.SOURCE_INFO_SIZEOF),
        blockBytes(BOOL_BYTES),
      )
    ) { arena ->
      val view = writeStringView(arena, sourceId)
      val info = allocate(arena, SourceMarshal.SOURCE_INFO_SIZEOF)
      val found = allocate(arena, BOOL_BYTES)
      // An output descriptor states its own size too: native reads it to decide which fields it may
      // write, and a zeroed block would ask for a zero-sized descriptor.
      SourceMarshal.writeSourceInfoHeader(info)
      Status.check(
        mln_map_get_style_source_info(handle.raw, view.address, info.address, found.address)
      )
      if (!isSet(found)) {
        return@withArena null
      }
      // The descriptor carries lengths and counts rather than the strings themselves, so the three
      // copies below run while it is still alive and are folded into the value it returns.
      SourceMarshal.readSourceInfo(
        info,
        copyStyleSourceAttribution(sourceId, info),
        copyStyleSourceUrl(sourceId, info),
        styleSourceTileUrls(sourceId, info),
      )
    }
  }

  /** Copies the attribution the metadata at [info] reports a length for. */
  private fun copyStyleSourceAttribution(sourceId: String, info: HeapPointer): String? {
    // A source with no attribution reports no size, which is distinct from reporting an empty one.
    if (!SourceMarshal.sourceInfoHasAttribution(info)) return null
    val capacity =
      readCount(MlnStyleSourceInfo.attributionSize(info), "style source attribution size")
    if (capacity == 0) return ""
    return copyStyleSourceText(
      ::mln_map_copy_style_source_attribution,
      sourceId,
      capacity,
      "style source attribution size",
    )
  }

  /** Copies the URL the metadata at [info] reports a length for. */
  private fun copyStyleSourceUrl(sourceId: String, info: HeapPointer): String? {
    if (!SourceMarshal.sourceInfoHasUrl(info)) return null
    val capacity = readCount(MlnStyleSourceInfo.urlSize(info), "style source URL size")
    if (capacity == 0) return ""
    return copyStyleSourceText(
      ::mln_map_copy_style_source_url,
      sourceId,
      capacity,
      "style source URL size",
    )
  }

  /**
   * Copies one of the texts a source metadata length sizes.
   *
   * Null when the source went missing between the metadata call and this one, which is a race the
   * caller loses rather than an error.
   */
  private fun copyStyleSourceText(
    entry: (Long, Int, Int, Int, Int, Int) -> Int,
    sourceId: String,
    capacity: Int,
    subject: String,
  ): String? = live {
    withArena(
      bytes(
        stringViewBytes(sourceId),
        blockBytes(capacity),
        blockBytes(SIZE_BYTES),
        blockBytes(BOOL_BYTES),
      )
    ) { arena ->
      val view = writeStringView(arena, sourceId)
      val text = allocate(arena, capacity)
      val copied = allocate(arena, SIZE_BYTES)
      val found = allocate(arena, BOOL_BYTES)
      Status.check(
        entry(handle.raw, view.address, text.address, capacity, copied.address, found.address)
      )
      if (!isSet(found)) {
        null
      } else {
        Heap.loadBytes(text, readCount(Heap.loadInt(copied), subject)).decodeToString()
      }
    }
  }

  /** Copies the inline TileJSON tile URLs of the source the metadata at [info] describes. */
  private fun styleSourceTileUrls(sourceId: String, info: HeapPointer): List<String>? {
    if (!SourceMarshal.sourceInfoHasTileJson(info)) return null
    val list = live {
      withArena(
        bytes(stringViewBytes(sourceId), blockBytes(HANDLE_BYTES), blockBytes(BOOL_BYTES))
      ) { arena ->
        val view = writeStringView(arena, sourceId)
        // Native refuses an out-parameter that is not the null handle, which the zeroed arena
        // already satisfies.
        val out = allocate(arena, HANDLE_BYTES)
        val found = allocate(arena, BOOL_BYTES)
        Status.check(
          mln_map_get_style_source_tile_urls(handle.raw, view.address, out.address, found.address)
        )
        if (isSet(found)) Heap.loadLong(out) else 0L
      }
    }
    return readStyleStringList(list)
  }

  public actual fun styleSourceIds(): List<String> = listStyleIds(::mln_map_list_style_source_ids)

  public actual fun addGeoJsonSourceUrl(
    sourceId: String,
    url: String,
    options: GeoJsonSourceOptions?,
  ) {
    live {
      withArena(
        bytes(stringViewBytes(sourceId), stringViewBytes(url), geoJsonSourceOptionsBytes(options))
      ) { arena ->
        val sourceView = writeStringView(arena, sourceId)
        val urlView = writeStringView(arena, url)
        val descriptor = writeGeoJsonSourceOptions(arena, options)
        Status.check(
          mln_map_add_geojson_source_url(
            handle.raw,
            sourceView.address,
            urlView.address,
            descriptor.address,
          )
        )
      }
    }
  }

  public actual fun addGeoJsonSourceData(
    sourceId: String,
    data: GeoJson,
    options: GeoJsonSourceOptions?,
  ) {
    live {
      withArena(
        bytes(
          stringViewBytes(sourceId),
          GeoJsonMarshal.measure(data).toLong(),
          geoJsonSourceOptionsBytes(options),
        )
      ) { arena ->
        val sourceView = writeStringView(arena, sourceId)
        val root = GeoJsonMarshal.write(arena, data)
        val descriptor = writeGeoJsonSourceOptions(arena, options)
        Status.check(
          mln_map_add_geojson_source_data(
            handle.raw,
            sourceView.address,
            root.address,
            descriptor.address,
          )
        )
      }
    }
  }

  public actual fun setGeoJsonSourceUrl(sourceId: String, url: String) {
    live { callWithTwoIds(::mln_map_set_geojson_source_url, sourceId, url) }
  }

  public actual fun setGeoJsonSourceData(sourceId: String, data: GeoJson) {
    live {
      withArena(bytes(stringViewBytes(sourceId), GeoJsonMarshal.measure(data).toLong())) { arena ->
        val view = writeStringView(arena, sourceId)
        val root = GeoJsonMarshal.write(arena, data)
        Status.check(mln_map_set_geojson_source_data(handle.raw, view.address, root.address))
      }
    }
  }

  /**
   * Adds a custom geometry source whose tiles the host supplies.
   *
   * MapLibre asks for a tile on the worker the source's tile loader runs on, which is not the
   * thread this binding runs on, so the module queues the request and the runtime delivers it from
   * [org.maplibre.nativeffi.runtime.RuntimeHandle.pump]. The callback may answer from inside itself
   * with [setCustomGeometrySourceTileData], exactly as it may on every other platform.
   *
   * The callback registration lives as long as the source does. It is released when the source is
   * removed with [removeStyleSource], when a new style drops it, and when this map is closed, and a
   * request that arrives after any of those is dropped rather than delivered.
   */
  public actual fun addCustomGeometrySource(
    sourceId: String,
    options: CustomGeometrySourceOptions,
  ) {
    live {
      // Installed before native is told about it, because the module names the callback by the
      // pointer this places: a source added first could ask for a tile that had nowhere to go.
      val bridge = CustomGeometryBridge.install(options.callback)
      var added = false
      try {
        withArena(
          bytes(
            stringViewBytes(sourceId),
            blockBytes(SourceMarshal.CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZEOF),
          )
        ) { arena ->
          val view = writeStringView(arena, sourceId)
          val descriptor = allocate(arena, SourceMarshal.CUSTOM_GEOMETRY_SOURCE_OPTIONS_SIZEOF)
          SourceMarshal.writeCustomGeometrySourceOptions(
            descriptor,
            options,
            CustomGeometryBridge.fetchCallback(),
            CustomGeometryBridge.cancelCallback(),
            bridge.userData,
          )
          Status.check(
            mln_map_add_custom_geometry_source(handle.raw, view.address, descriptor.address)
          )
        }
        added = true
      } finally {
        // A refused source has no callbacks to serve, so the registration goes back rather than
        // standing for a source that does not exist.
        if (!added) bridge.close()
      }
      // Replacing an id native accepted means the previous source is gone, so its registration is
      // released here rather than left to be found by a request it can no longer answer.
      customGeometrySources.put(sourceId, bridge)?.close()
    }
  }

  public actual fun setCustomGeometrySourceTileData(
    sourceId: String,
    tileId: CanonicalTileId,
    data: GeoJson,
  ) {
    live {
      withArena(
        bytes(
          stringViewBytes(sourceId),
          blockBytes(MlnCanonicalTileId.SIZEOF),
          GeoJsonMarshal.measure(data).toLong(),
        )
      ) { arena ->
        val view = writeStringView(arena, sourceId)
        val tile = writeCanonicalTileId(arena, tileId)
        val root = GeoJsonMarshal.write(arena, data)
        Status.check(
          mln_map_set_custom_geometry_source_tile_data(
            handle.raw,
            view.address,
            tile.address,
            root.address,
          )
        )
      }
    }
  }

  public actual fun invalidateCustomGeometrySourceTile(sourceId: String, tileId: CanonicalTileId) {
    live {
      withArena(bytes(stringViewBytes(sourceId), blockBytes(MlnCanonicalTileId.SIZEOF))) { arena ->
        val view = writeStringView(arena, sourceId)
        val tile = writeCanonicalTileId(arena, tileId)
        Status.check(
          mln_map_invalidate_custom_geometry_source_tile(handle.raw, view.address, tile.address)
        )
      }
    }
  }

  public actual fun invalidateCustomGeometrySourceRegion(sourceId: String, bounds: LatLngBounds) {
    live {
      withArena(bytes(stringViewBytes(sourceId), blockBytes(MlnLatLngBounds.SIZEOF))) { arena ->
        val view = writeStringView(arena, sourceId)
        val region = allocate(arena, MlnLatLngBounds.SIZEOF)
        MapOptionsMarshal.writeLatLngBounds(region, bounds)
        Status.check(
          mln_map_invalidate_custom_geometry_source_region(handle.raw, view.address, region.address)
        )
      }
    }
  }

  public actual fun addVectorSourceUrl(sourceId: String, url: String, options: TileSourceOptions?) {
    addTileSourceUrl(::mln_map_add_vector_source_url, sourceId, url, options)
  }

  public actual fun addVectorSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    addTileSourceTiles(::mln_map_add_vector_source_tiles, sourceId, tiles, options)
  }

  public actual fun addRasterSourceUrl(sourceId: String, url: String, options: TileSourceOptions?) {
    addTileSourceUrl(::mln_map_add_raster_source_url, sourceId, url, options)
  }

  public actual fun addRasterSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    addTileSourceTiles(::mln_map_add_raster_source_tiles, sourceId, tiles, options)
  }

  public actual fun addRasterDemSourceUrl(
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    addTileSourceUrl(::mln_map_add_raster_dem_source_url, sourceId, url, options)
  }

  public actual fun addRasterDemSourceTiles(
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    addTileSourceTiles(::mln_map_add_raster_dem_source_tiles, sourceId, tiles, options)
  }

  public actual fun setStyleImage(
    imageId: String,
    image: PremultipliedRgba8Image,
    options: StyleImageOptions,
  ) {
    live {
      // Two blocks rather than one: the image marshaller owns the descriptor-plus-pixels pairing
      // and
      // takes its own scratch, and folding that into this arena would mean measuring the pixels
      // here as well as there.
      StyleMarshal.withImage(image) { imageDescriptor ->
        withArena(
          bytes(stringViewBytes(imageId), StyleMarshal.measureImageOptions(options).toLong())
        ) { arena ->
          val view = writeStringView(arena, imageId)
          val optionsDescriptor = StyleMarshal.writeImageOptions(arena, options)
          Status.check(
            mln_map_set_style_image(
              handle.raw,
              view.address,
              imageDescriptor.address,
              optionsDescriptor.address,
            )
          )
        }
      }
    }
  }

  public actual fun removeStyleImage(imageId: String): Boolean =
    flagForId(::mln_map_remove_style_image, imageId)

  public actual fun styleImageExists(imageId: String): Boolean =
    flagForId(::mln_map_style_image_exists, imageId)

  public actual fun styleImageInfo(imageId: String): StyleImageInfo? = live {
    withArena(
      bytes(
        stringViewBytes(imageId),
        blockBytes(StyleMarshal.IMAGE_INFO_SIZEOF),
        blockBytes(BOOL_BYTES),
      )
    ) { arena ->
      val view = writeStringView(arena, imageId)
      val info = allocate(arena, StyleMarshal.IMAGE_INFO_SIZEOF)
      val found = allocate(arena, BOOL_BYTES)
      StyleMarshal.writeImageInfoHeader(info)
      Status.check(
        mln_map_get_style_image_info(handle.raw, view.address, info.address, found.address)
      )
      if (isSet(found)) StyleMarshal.readImageInfo(info) else null
    }
  }

  public actual fun styleImageStretches(
    imageId: String
  ): Pair<List<ImageStretch>, List<ImageStretch>>? {
    // A null destination with zero capacity is the C API's size probe: it fills both counts and
    // succeeds without copying, which is how the arrays below are sized.
    val counts =
      live {
        withArena(
          bytes(
            stringViewBytes(imageId),
            blockBytes(SIZE_BYTES),
            blockBytes(SIZE_BYTES),
            blockBytes(BOOL_BYTES),
          )
        ) { arena ->
          val view = writeStringView(arena, imageId)
          val xCount = allocate(arena, SIZE_BYTES)
          val yCount = allocate(arena, SIZE_BYTES)
          val found = allocate(arena, BOOL_BYTES)
          copyStyleImageStretches(view, HeapPointer(0), 0, xCount, HeapPointer(0), 0, yCount, found)
          if (!isSet(found)) {
            null
          } else {
            readCount(Heap.loadInt(xCount), "style image stretch x count") to
              readCount(Heap.loadInt(yCount), "style image stretch y count")
          }
        }
      } ?: return null

    val (xCapacity, yCapacity) = counts
    val xBytes = Heap.sizeOf(StyleMarshal.IMAGE_STRETCH_SIZEOF, xCapacity)
    val yBytes = Heap.sizeOf(StyleMarshal.IMAGE_STRETCH_SIZEOF, yCapacity)
    return live {
      withArena(
        bytes(
          stringViewBytes(imageId),
          blockBytes(xBytes),
          blockBytes(yBytes),
          blockBytes(SIZE_BYTES),
          blockBytes(SIZE_BYTES),
          blockBytes(BOOL_BYTES),
        )
      ) { arena ->
        val view = writeStringView(arena, imageId)
        val stretchX = allocate(arena, xBytes)
        val stretchY = allocate(arena, yBytes)
        val xCount = allocate(arena, SIZE_BYTES)
        val yCount = allocate(arena, SIZE_BYTES)
        val found = allocate(arena, BOOL_BYTES)
        copyStyleImageStretches(
          view,
          stretchX,
          xCapacity,
          xCount,
          stretchY,
          yCapacity,
          yCount,
          found,
        )
        // The image was there a moment ago, so it going missing between the two calls is a race the
        // caller loses rather than an error; report it the way the first call would have.
        if (!isSet(found)) {
          null
        } else {
          readStretches(stretchX, readCount(Heap.loadInt(xCount), "style image stretch x count")) to
            readStretches(stretchY, readCount(Heap.loadInt(yCount), "style image stretch y count"))
        }
      }
    }
  }

  public actual fun copyStyleImagePremultipliedRgba8(imageId: String): StyleImage? {
    // The metadata says how large the pixel buffer has to be, and reports absence, so it is read
    // first rather than probed for a second time here.
    val info = styleImageInfo(imageId) ?: return null
    val capacity = readLength(info.byteLength, "style image byte length")
    return live {
      withArena(
        bytes(
          stringViewBytes(imageId),
          blockBytes(capacity),
          blockBytes(SIZE_BYTES),
          blockBytes(BOOL_BYTES),
        )
      ) { arena ->
        val view = writeStringView(arena, imageId)
        val pixels = allocate(arena, capacity)
        val copied = allocate(arena, SIZE_BYTES)
        val found = allocate(arena, BOOL_BYTES)
        Status.check(
          mln_map_copy_style_image_premultiplied_rgba8(
            handle.raw,
            view.address,
            pixels.address,
            capacity,
            copied.address,
            found.address,
          )
        )
        if (!isSet(found)) {
          null
        } else {
          StyleImage(
            PremultipliedRgba8Image(
              info.width,
              info.height,
              info.stride,
              Heap.loadBytes(
                pixels,
                readCount(Heap.loadInt(copied), "style image copied byte length"),
              ),
            ),
            info.pixelRatio,
            info.sdf,
          )
        }
      }
    }
  }

  public actual fun addImageSourceUrl(sourceId: String, coordinates: List<LatLng>, url: String) {
    live {
      val coordinateBytes = Heap.sizeOf(MlnLatLng.SIZEOF, coordinates.size)
      withArena(
        bytes(stringViewBytes(sourceId), blockBytes(coordinateBytes), stringViewBytes(url))
      ) { arena ->
        val sourceView = writeStringView(arena, sourceId)
        val quad = writeLatLngs(arena, coordinates)
        val urlView = writeStringView(arena, url)
        Status.check(
          mln_map_add_image_source_url(
            handle.raw,
            sourceView.address,
            quad.address,
            coordinates.size,
            urlView.address,
          )
        )
      }
    }
  }

  public actual fun addImageSourceImage(
    sourceId: String,
    coordinates: List<LatLng>,
    image: PremultipliedRgba8Image,
  ) {
    live {
      StyleMarshal.withImage(image) { imageDescriptor ->
        val coordinateBytes = Heap.sizeOf(MlnLatLng.SIZEOF, coordinates.size)
        withArena(bytes(stringViewBytes(sourceId), blockBytes(coordinateBytes))) { arena ->
          val view = writeStringView(arena, sourceId)
          val quad = writeLatLngs(arena, coordinates)
          Status.check(
            mln_map_add_image_source_image(
              handle.raw,
              view.address,
              quad.address,
              coordinates.size,
              imageDescriptor.address,
            )
          )
        }
      }
    }
  }

  public actual fun setImageSourceUrl(sourceId: String, url: String) {
    live { callWithTwoIds(::mln_map_set_image_source_url, sourceId, url) }
  }

  public actual fun setImageSourceImage(sourceId: String, image: PremultipliedRgba8Image) {
    live {
      StyleMarshal.withImage(image) { imageDescriptor ->
        withArena(stringViewBytes(sourceId)) { arena ->
          val view = writeStringView(arena, sourceId)
          Status.check(
            mln_map_set_image_source_image(handle.raw, view.address, imageDescriptor.address)
          )
        }
      }
    }
  }

  public actual fun setImageSourceCoordinates(sourceId: String, coordinates: List<LatLng>) {
    live {
      val coordinateBytes = Heap.sizeOf(MlnLatLng.SIZEOF, coordinates.size)
      withArena(bytes(stringViewBytes(sourceId), blockBytes(coordinateBytes))) { arena ->
        val view = writeStringView(arena, sourceId)
        val quad = writeLatLngs(arena, coordinates)
        Status.check(
          mln_map_set_image_source_coordinates(
            handle.raw,
            view.address,
            quad.address,
            coordinates.size,
          )
        )
      }
    }
  }

  public actual fun imageSourceCoordinates(sourceId: String): List<LatLng>? = live {
    // An image source's coordinates are the four corners of a quad, so the buffer is fixed rather
    // than probed for.
    val coordinateBytes = Heap.sizeOf(MlnLatLng.SIZEOF, IMAGE_SOURCE_COORDINATE_COUNT)
    withArena(
      bytes(
        stringViewBytes(sourceId),
        blockBytes(coordinateBytes),
        blockBytes(SIZE_BYTES),
        blockBytes(BOOL_BYTES),
      )
    ) { arena ->
      val view = writeStringView(arena, sourceId)
      val quad = allocate(arena, coordinateBytes)
      val count = allocate(arena, SIZE_BYTES)
      val found = allocate(arena, BOOL_BYTES)
      Status.check(
        mln_map_get_image_source_coordinates(
          handle.raw,
          view.address,
          quad.address,
          IMAGE_SOURCE_COORDINATE_COUNT,
          count.address,
          found.address,
        )
      )
      if (!isSet(found)) {
        null
      } else {
        readLatLngs(quad, readCount(Heap.loadInt(count), "image source coordinate count"))
      }
    }
  }

  public actual fun addStyleLayerJson(layerJson: JsonValue, beforeLayerId: String) {
    live {
      withArena(bytes(JsonMarshal.measure(layerJson).toLong(), stringViewBytes(beforeLayerId))) {
        arena ->
        val root = JsonMarshal.write(arena, layerJson)
        val view = writeStringView(arena, beforeLayerId)
        Status.check(mln_map_add_style_layer_json(handle.raw, root.address, view.address))
      }
    }
  }

  public actual fun addHillshadeLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    live { callWithThreeIds(::mln_map_add_hillshade_layer, layerId, sourceId, beforeLayerId) }
  }

  public actual fun addColorReliefLayer(layerId: String, sourceId: String, beforeLayerId: String) {
    live { callWithThreeIds(::mln_map_add_color_relief_layer, layerId, sourceId, beforeLayerId) }
  }

  public actual fun addLocationIndicatorLayer(layerId: String, beforeLayerId: String) {
    live { callWithTwoIds(::mln_map_add_location_indicator_layer, layerId, beforeLayerId) }
  }

  public actual fun setLocationIndicatorLocation(
    layerId: String,
    coordinate: LatLng,
    altitude: Double,
  ) {
    live {
      withArena(bytes(stringViewBytes(layerId), blockBytes(MlnLatLng.SIZEOF))) { arena ->
        val view = writeStringView(arena, layerId)
        val position = allocate(arena, MlnLatLng.SIZEOF)
        CameraMarshal.writeLatLng(position, coordinate)
        Status.check(
          mln_map_set_location_indicator_location(
            handle.raw,
            view.address,
            position.address,
            altitude,
          )
        )
      }
    }
  }

  public actual fun setLocationIndicatorBearing(layerId: String, bearing: Double) {
    live { setDoubleForId(::mln_map_set_location_indicator_bearing, layerId, bearing) }
  }

  public actual fun setLocationIndicatorAccuracyRadius(layerId: String, radius: Double) {
    live { setDoubleForId(::mln_map_set_location_indicator_accuracy_radius, layerId, radius) }
  }

  public actual fun setLocationIndicatorImageName(
    layerId: String,
    imageKind: LocationIndicatorImageKind,
    imageId: String,
  ) {
    live {
      withArena(bytes(stringViewBytes(layerId), stringViewBytes(imageId))) { arena ->
        val layerView = writeStringView(arena, layerId)
        val imageView = writeStringView(arena, imageId)
        Status.check(
          mln_map_set_location_indicator_image_name(
            handle.raw,
            layerView.address,
            imageKind.nativeValue,
            imageView.address,
          )
        )
      }
    }
  }

  public actual fun removeStyleLayer(layerId: String): Boolean =
    flagForId(::mln_map_remove_style_layer, layerId)

  public actual fun styleLayerExists(layerId: String): Boolean =
    flagForId(::mln_map_style_layer_exists, layerId)

  public actual fun styleLayerType(layerId: String): String? = live {
    withArena(
      bytes(stringViewBytes(layerId), blockBytes(MlnStringView.SIZEOF), blockBytes(BOOL_BYTES))
    ) { arena ->
      val view = writeStringView(arena, layerId)
      val type = allocate(arena, MlnStringView.SIZEOF)
      val found = allocate(arena, BOOL_BYTES)
      Status.check(
        mln_map_get_style_layer_type(handle.raw, view.address, type.address, found.address)
      )
      // Copied here rather than handed back as a view: the bytes belong to the style, which the
      // next call on this map may replace.
      if (isSet(found)) JsonMarshal.readText(type) else null
    }
  }

  public actual fun styleLayerIds(): List<String> = listStyleIds(::mln_map_list_style_layer_ids)

  public actual fun moveStyleLayer(layerId: String, beforeLayerId: String) {
    live { callWithTwoIds(::mln_map_move_style_layer, layerId, beforeLayerId) }
  }

  public actual fun styleLayerJson(layerId: String): JsonValue? {
    val snapshot = live {
      withArena(
        bytes(stringViewBytes(layerId), blockBytes(HANDLE_BYTES), blockBytes(BOOL_BYTES))
      ) { arena ->
        val view = writeStringView(arena, layerId)
        val out = allocate(arena, HANDLE_BYTES)
        val found = allocate(arena, BOOL_BYTES)
        Status.check(
          mln_map_get_style_layer_json(handle.raw, view.address, out.address, found.address)
        )
        if (isSet(found)) Heap.loadLong(out) else 0L
      }
    }
    return readJsonSnapshot(snapshot)
  }

  public actual fun setStyleLightJson(lightJson: JsonValue) {
    live {
      withArena(JsonMarshal.measure(lightJson).toLong()) { arena ->
        val root = JsonMarshal.write(arena, lightJson)
        Status.check(mln_map_set_style_light_json(handle.raw, root.address))
      }
    }
  }

  public actual fun setStyleLightProperty(propertyName: String, value: JsonValue) {
    live { callWithIdAndJson(::mln_map_set_style_light_property, propertyName, value) }
  }

  public actual fun styleLightProperty(propertyName: String): JsonValue? =
    jsonSnapshotForId(::mln_map_get_style_light_property, propertyName)

  public actual fun setStyleTransitionOptions(options: StyleTransitionOptions) {
    live {
      Heap.withScratch(StyleMarshal.TRANSITION_OPTIONS_SIZEOF) { descriptor ->
        StyleMarshal.writeTransitionOptions(descriptor, options)
        Status.check(mln_map_set_style_transition_options(handle.raw, descriptor.address))
      }
    }
  }

  public actual fun styleTransitionOptions(): StyleTransitionOptions = live {
    Heap.withScratch(StyleMarshal.TRANSITION_OPTIONS_SIZEOF) { out ->
      StyleMarshal.writeTransitionOptionsHeader(out)
      Status.check(mln_map_get_style_transition_options(handle.raw, out.address))
      StyleMarshal.readTransitionOptions(out)
    }
  }

  public actual fun setLayerProperty(layerId: String, propertyName: String, value: JsonValue) {
    live {
      withArena(
        bytes(
          stringViewBytes(layerId),
          stringViewBytes(propertyName),
          JsonMarshal.measure(value).toLong(),
        )
      ) { arena ->
        val layerView = writeStringView(arena, layerId)
        val propertyView = writeStringView(arena, propertyName)
        val root = JsonMarshal.write(arena, value)
        Status.check(
          mln_map_set_layer_property(
            handle.raw,
            layerView.address,
            propertyView.address,
            root.address,
          )
        )
      }
    }
  }

  public actual fun layerProperty(layerId: String, propertyName: String): JsonValue? {
    val snapshot = live {
      withArena(
        bytes(stringViewBytes(layerId), stringViewBytes(propertyName), blockBytes(HANDLE_BYTES))
      ) { arena ->
        val layerView = writeStringView(arena, layerId)
        val propertyView = writeStringView(arena, propertyName)
        val out = allocate(arena, HANDLE_BYTES)
        Status.check(
          mln_map_get_layer_property(
            handle.raw,
            layerView.address,
            propertyView.address,
            out.address,
          )
        )
        Heap.loadLong(out)
      }
    }
    return readJsonSnapshot(snapshot)
  }

  public actual fun setLayerFilter(layerId: String, filter: JsonValue) {
    live { callWithIdAndJson(::mln_map_set_layer_filter, layerId, filter) }
  }

  public actual fun clearLayerFilter(layerId: String) {
    // The same entry point with a null filter, which is how the C API spells "no filter" rather
    // than an empty expression, which would mean something else.
    live { callWithIdAndJson(::mln_map_set_layer_filter, layerId, null) }
  }

  public actual fun layerFilter(layerId: String): JsonValue? =
    jsonSnapshotForId(::mln_map_get_layer_filter, layerId)

  public actual fun setLayerSourceLayer(layerId: String, sourceLayer: String) {
    live { callWithTwoIds(::mln_map_set_layer_source_layer, layerId, sourceLayer) }
  }

  public actual fun layerSourceLayer(layerId: String): String =
    copyLayerText(::mln_map_copy_layer_source_layer, layerId)

  public actual fun setLayerSourceId(layerId: String, sourceId: String) {
    live { callWithTwoIds(::mln_map_set_layer_source_id, layerId, sourceId) }
  }

  public actual fun layerSourceId(layerId: String): String =
    copyLayerText(::mln_map_copy_layer_source_id, layerId)

  public actual fun setLayerMinZoom(layerId: String, minZoom: Double) {
    live { setDoubleForId(::mln_map_set_layer_min_zoom, layerId, minZoom) }
  }

  public actual fun layerMinZoom(layerId: String): Double =
    doubleForId(::mln_map_get_layer_min_zoom, layerId)

  public actual fun setLayerMaxZoom(layerId: String, maxZoom: Double) {
    live { setDoubleForId(::mln_map_set_layer_max_zoom, layerId, maxZoom) }
  }

  public actual fun layerMaxZoom(layerId: String): Double =
    doubleForId(::mln_map_get_layer_max_zoom, layerId)

  public actual fun setLayerVisibility(layerId: String, visibility: StyleLayerVisibility) {
    live {
      withArena(stringViewBytes(layerId)) { arena ->
        val view = writeStringView(arena, layerId)
        Status.check(mln_map_set_layer_visibility(handle.raw, view.address, visibility.nativeValue))
      }
    }
  }

  public actual fun layerVisibility(layerId: String): StyleLayerVisibility = live {
    withArena(bytes(stringViewBytes(layerId), blockBytes(SIZE_BYTES))) { arena ->
      val view = writeStringView(arena, layerId)
      val out = allocate(arena, SIZE_BYTES)
      Status.check(mln_map_get_layer_visibility(handle.raw, view.address, out.address))
      StyleLayerVisibility.fromNative(Heap.loadInt(out))
    }
  }

  public actual fun requestRepaint() {
    live { Status.check(mln_map_request_repaint(handle.raw)) }
  }

  public actual fun requestStillImage() {
    live { Status.check(mln_map_request_still_image(handle.raw)) }
  }

  public actual var debugOptions: Set<DebugOption>
    get() = live {
      Heap.withScratch(SIZE_BYTES) { out ->
        Status.check(mln_map_get_debug_options(handle.raw, out.address))
        val mask = Heap.loadInt(out)
        DebugOption.entries.filterTo(mutableSetOf()) { (mask and it.nativeMask) != 0 }
      }
    }
    set(options) {
      val mask = options.fold(0) { accumulated, option -> accumulated or option.nativeMask }
      live { Status.check(mln_map_set_debug_options(handle.raw, mask)) }
    }

  public actual var isRenderingStatsViewEnabled: Boolean
    get() = flagForMap(::mln_map_get_rendering_stats_view_enabled)
    set(enabled) {
      live { setFlagForMap(::mln_map_set_rendering_stats_view_enabled, enabled) }
    }

  public actual val isFullyLoaded: Boolean
    get() = flagForMap(::mln_map_is_fully_loaded)

  public actual fun dumpDebugLogs() {
    live { Status.check(mln_map_dump_debug_logs(handle.raw)) }
  }

  public actual val size: MapSize
    get() = live {
      // The scale factor goes first because it is the only member here that needs eight-byte
      // alignment: the heap views these reads go through index by width rather than by byte, so a
      // double placed at an address the allocator did not align would be read somewhere else
      // entirely rather than merely slowly.
      Heap.withScratch(DOUBLE_BYTES + SIZE_BYTES + SIZE_BYTES) { scaleFactor ->
        val width = scaleFactor + DOUBLE_BYTES
        val height = width + SIZE_BYTES
        Status.check(
          mln_map_get_size(handle.raw, width.address, height.address, scaleFactor.address)
        )
        MapSize(Heap.loadInt(width), Heap.loadInt(height), Heap.loadDouble(scaleFactor))
      }
    }

  public actual var viewportOptions: ViewportOptions
    get() = live {
      Heap.withScratch(MapOptionsMarshal.VIEWPORT_OPTIONS_SIZEOF) { out ->
        MapOptionsMarshal.writeViewportOptionsHeader(out)
        Status.check(mln_map_get_viewport_options(handle.raw, out.address))
        MapOptionsMarshal.readViewportOptions(out)
      }
    }
    set(options) {
      live {
        Heap.withScratch(MapOptionsMarshal.VIEWPORT_OPTIONS_SIZEOF) { descriptor ->
          MapOptionsMarshal.writeViewportOptions(descriptor, options)
          Status.check(mln_map_set_viewport_options(handle.raw, descriptor.address))
        }
      }
    }

  public actual var tileOptions: TileOptions
    get() = live {
      Heap.withScratch(MapOptionsMarshal.TILE_OPTIONS_SIZEOF) { out ->
        MapOptionsMarshal.writeTileOptionsHeader(out)
        Status.check(mln_map_get_tile_options(handle.raw, out.address))
        MapOptionsMarshal.readTileOptions(out)
      }
    }
    set(options) {
      live {
        Heap.withScratch(MapOptionsMarshal.TILE_OPTIONS_SIZEOF) { descriptor ->
          MapOptionsMarshal.writeTileOptions(descriptor, options)
          Status.check(mln_map_set_tile_options(handle.raw, descriptor.address))
        }
      }
    }

  public actual val camera: CameraOptions
    get() = live {
      Heap.withScratch(CameraMarshal.SIZEOF) { out ->
        CameraMarshal.writeHeader(out)
        Status.check(mln_map_get_camera(handle.raw, out.address))
        CameraMarshal.read(out)
      }
    }

  public actual fun jumpTo(camera: CameraOptions) {
    live {
      Heap.withScratch(CameraMarshal.SIZEOF) { descriptor ->
        CameraMarshal.write(descriptor, camera)
        Status.check(mln_map_jump_to(handle.raw, descriptor.address))
      }
    }
  }

  public actual fun easeTo(camera: CameraOptions, animation: AnimationOptions?) {
    live { transitionTo(::mln_map_ease_to, camera, animation) }
  }

  public actual fun flyTo(camera: CameraOptions, animation: AnimationOptions?) {
    live { transitionTo(::mln_map_fly_to, camera, animation) }
  }

  public actual fun moveBy(deltaX: Double, deltaY: Double) {
    live { Status.check(mln_map_move_by(handle.raw, deltaX, deltaY)) }
  }

  public actual fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions?) {
    live {
      withArena(animationBytes(animation)) { arena ->
        val descriptor = writeAnimationOptions(arena, animation)
        Status.check(mln_map_move_by_animated(handle.raw, deltaX, deltaY, descriptor.address))
      }
    }
  }

  public actual fun scaleBy(scale: Double, anchor: ScreenPoint?) {
    live {
      withArena(anchorBytes(anchor)) { arena ->
        val point = writeScreenPointOrNull(arena, anchor)
        Status.check(mln_map_scale_by(handle.raw, scale, point.address))
      }
    }
  }

  public actual fun scaleByAnimated(
    scale: Double,
    anchor: ScreenPoint?,
    animation: AnimationOptions?,
  ) {
    live {
      withArena(bytes(anchorBytes(anchor), animationBytes(animation))) { arena ->
        val point = writeScreenPointOrNull(arena, anchor)
        val descriptor = writeAnimationOptions(arena, animation)
        Status.check(
          mln_map_scale_by_animated(handle.raw, scale, point.address, descriptor.address)
        )
      }
    }
  }

  public actual fun rotateBy(first: ScreenPoint, second: ScreenPoint) {
    live {
      withArena(bytes(blockBytes(MlnScreenPoint.SIZEOF), blockBytes(MlnScreenPoint.SIZEOF))) { arena
        ->
        val start = writeScreenPoint(arena, first)
        val end = writeScreenPoint(arena, second)
        Status.check(mln_map_rotate_by(handle.raw, start.address, end.address))
      }
    }
  }

  public actual fun rotateByAnimated(
    first: ScreenPoint,
    second: ScreenPoint,
    animation: AnimationOptions?,
  ) {
    live {
      withArena(
        bytes(
          blockBytes(MlnScreenPoint.SIZEOF),
          blockBytes(MlnScreenPoint.SIZEOF),
          animationBytes(animation),
        )
      ) { arena ->
        val start = writeScreenPoint(arena, first)
        val end = writeScreenPoint(arena, second)
        val descriptor = writeAnimationOptions(arena, animation)
        Status.check(
          mln_map_rotate_by_animated(handle.raw, start.address, end.address, descriptor.address)
        )
      }
    }
  }

  public actual fun pitchBy(pitch: Double) {
    live { Status.check(mln_map_pitch_by(handle.raw, pitch)) }
  }

  public actual fun pitchByAnimated(pitch: Double, animation: AnimationOptions?) {
    live {
      withArena(animationBytes(animation)) { arena ->
        val descriptor = writeAnimationOptions(arena, animation)
        Status.check(mln_map_pitch_by_animated(handle.raw, pitch, descriptor.address))
      }
    }
  }

  public actual fun cancelTransitions() {
    live { Status.check(mln_map_cancel_transitions(handle.raw)) }
  }

  public actual var isGestureInProgress: Boolean
    get() = flagForMap(::mln_map_is_gesture_in_progress)
    set(inProgress) {
      live { setFlagForMap(::mln_map_set_gesture_in_progress, inProgress) }
    }

  public actual fun cameraForLatLngBounds(
    bounds: LatLngBounds,
    fitOptions: CameraFitOptions?,
  ): CameraOptions = live {
    withArena(
      bytes(
        blockBytes(MlnLatLngBounds.SIZEOF),
        fitOptionsBytes(fitOptions),
        blockBytes(CameraMarshal.SIZEOF),
      )
    ) { arena ->
      val region = allocate(arena, MlnLatLngBounds.SIZEOF)
      MapOptionsMarshal.writeLatLngBounds(region, bounds)
      val fit = writeCameraFitOptions(arena, fitOptions)
      val out = allocate(arena, CameraMarshal.SIZEOF)
      CameraMarshal.writeHeader(out)
      Status.check(
        mln_map_camera_for_lat_lng_bounds(handle.raw, region.address, fit.address, out.address)
      )
      CameraMarshal.read(out)
    }
  }

  public actual fun cameraForLatLngs(
    coordinates: List<LatLng>,
    fitOptions: CameraFitOptions?,
  ): CameraOptions = live {
    val coordinateBytes = Heap.sizeOf(MlnLatLng.SIZEOF, coordinates.size)
    withArena(
      bytes(
        blockBytes(coordinateBytes),
        fitOptionsBytes(fitOptions),
        blockBytes(CameraMarshal.SIZEOF),
      )
    ) { arena ->
      val points = writeLatLngs(arena, coordinates)
      val fit = writeCameraFitOptions(arena, fitOptions)
      val out = allocate(arena, CameraMarshal.SIZEOF)
      CameraMarshal.writeHeader(out)
      Status.check(
        mln_map_camera_for_lat_lngs(
          handle.raw,
          points.address,
          coordinates.size,
          fit.address,
          out.address,
        )
      )
      CameraMarshal.read(out)
    }
  }

  public actual fun cameraForGeometry(
    geometry: Geometry,
    fitOptions: CameraFitOptions?,
  ): CameraOptions = live {
    // Measured before the block is taken. A geometry tree is many nested spans, and the arena
    // carves them out of one allocation rather than taking one per node.
    withArena(
      bytes(
        GeometryMarshal.measure(geometry).toLong(),
        fitOptionsBytes(fitOptions),
        blockBytes(CameraMarshal.SIZEOF),
      )
    ) { arena ->
      val root = GeometryMarshal.write(arena, geometry)
      val fit = writeCameraFitOptions(arena, fitOptions)
      val out = allocate(arena, CameraMarshal.SIZEOF)
      CameraMarshal.writeHeader(out)
      Status.check(mln_map_camera_for_geometry(handle.raw, root.address, fit.address, out.address))
      CameraMarshal.read(out)
    }
  }

  public actual fun latLngBoundsForCamera(camera: CameraOptions): LatLngBounds =
    boundsForCamera(::mln_map_lat_lng_bounds_for_camera, camera)

  public actual fun latLngBoundsForCameraUnwrapped(camera: CameraOptions): LatLngBounds =
    boundsForCamera(::mln_map_lat_lng_bounds_for_camera_unwrapped, camera)

  public actual var bounds: BoundOptions
    get() = live {
      Heap.withScratch(MapOptionsMarshal.BOUND_OPTIONS_SIZEOF) { out ->
        MapOptionsMarshal.writeBoundOptionsHeader(out)
        Status.check(mln_map_get_bounds(handle.raw, out.address))
        MapOptionsMarshal.readBoundOptions(out)
      }
    }
    set(options) {
      live {
        Heap.withScratch(MapOptionsMarshal.BOUND_OPTIONS_SIZEOF) { descriptor ->
          MapOptionsMarshal.writeBoundOptions(descriptor, options)
          Status.check(mln_map_set_bounds(handle.raw, descriptor.address))
        }
      }
    }

  public actual var freeCameraOptions: FreeCameraOptions
    get() = live {
      Heap.withScratch(MlnFreeCameraOptions.SIZEOF) { out ->
        MlnFreeCameraOptions.setSize(out, MlnFreeCameraOptions.SIZEOF)
        Status.check(mln_map_get_free_camera_options(handle.raw, out.address))
        readFreeCameraOptions(out)
      }
    }
    set(options) {
      live {
        Heap.withScratch(MlnFreeCameraOptions.SIZEOF) { descriptor ->
          writeFreeCameraOptions(descriptor, options)
          Status.check(mln_map_set_free_camera_options(handle.raw, descriptor.address))
        }
      }
    }

  public actual var projectionMode: ProjectionModeOptions
    get() = live {
      Heap.withScratch(MlnProjectionMode.SIZEOF) { out ->
        MlnProjectionMode.setSize(out, MlnProjectionMode.SIZEOF)
        Status.check(mln_map_get_projection_mode(handle.raw, out.address))
        readProjectionMode(out)
      }
    }
    set(mode) {
      live {
        Heap.withScratch(MlnProjectionMode.SIZEOF) { descriptor ->
          writeProjectionMode(descriptor, mode)
          Status.check(mln_map_set_projection_mode(handle.raw, descriptor.address))
        }
      }
    }

  public actual fun pixelForLatLng(coordinate: LatLng): ScreenPoint = live {
    Heap.withScratch(MlnLatLng.SIZEOF + MlnScreenPoint.SIZEOF) { scratch ->
      val out = scratch + MlnLatLng.SIZEOF
      CameraMarshal.writeLatLng(scratch, coordinate)
      Status.check(mln_map_pixel_for_lat_lng(handle.raw, scratch.address, out.address))
      ScreenPoint(MlnScreenPoint.x(out), MlnScreenPoint.y(out))
    }
  }

  public actual fun latLngForPixel(point: ScreenPoint): LatLng = live {
    Heap.withScratch(MlnScreenPoint.SIZEOF + MlnLatLng.SIZEOF) { scratch ->
      val out = scratch + MlnScreenPoint.SIZEOF
      MlnScreenPoint.setX(scratch, point.x)
      MlnScreenPoint.setY(scratch, point.y)
      Status.check(mln_map_lat_lng_for_pixel(handle.raw, scratch.address, out.address))
      CameraMarshal.readLatLng(out)
    }
  }

  public actual fun pixelsForLatLngs(coordinates: List<LatLng>): List<ScreenPoint> {
    // An empty run would ask for a zero-byte block, which cannot be acquired, and there is nothing
    // for native to project either way.
    if (coordinates.isEmpty()) return emptyList()
    return live {
      val inputBytes = Heap.sizeOf(MlnLatLng.SIZEOF, coordinates.size)
      val outputBytes = Heap.sizeOf(MlnScreenPoint.SIZEOF, coordinates.size)
      withArena(bytes(blockBytes(inputBytes), blockBytes(outputBytes))) { arena ->
        val points = writeLatLngs(arena, coordinates)
        val out = allocate(arena, outputBytes)
        Status.check(
          mln_map_pixels_for_lat_lngs(handle.raw, points.address, coordinates.size, out.address)
        )
        List(coordinates.size) { index ->
          val entry = out + index * MlnScreenPoint.SIZEOF
          ScreenPoint(MlnScreenPoint.x(entry), MlnScreenPoint.y(entry))
        }
      }
    }
  }

  public actual fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng> {
    if (points.isEmpty()) return emptyList()
    return live {
      val inputBytes = Heap.sizeOf(MlnScreenPoint.SIZEOF, points.size)
      val outputBytes = Heap.sizeOf(MlnLatLng.SIZEOF, points.size)
      withArena(bytes(blockBytes(inputBytes), blockBytes(outputBytes))) { arena ->
        val block = allocate(arena, inputBytes)
        points.forEachIndexed { index, point ->
          val entry = block + index * MlnScreenPoint.SIZEOF
          MlnScreenPoint.setX(entry, point.x)
          MlnScreenPoint.setY(entry, point.y)
        }
        val out = allocate(arena, outputBytes)
        Status.check(
          mln_map_lat_lngs_for_pixels(handle.raw, block.address, points.size, out.address)
        )
        readLatLngs(out, points.size)
      }
    }
  }

  public actual fun attachMetalOwnedTexture(
    descriptor: MetalOwnedTextureDescriptor
  ): RenderSessionHandle = throw unsupportedBackend("Metal")

  public actual fun attachMetalBorrowedTexture(
    descriptor: MetalBorrowedTextureDescriptor
  ): RenderSessionHandle = throw unsupportedBackend("Metal")

  public actual fun attachVulkanOwnedTexture(
    descriptor: VulkanOwnedTextureDescriptor
  ): RenderSessionHandle = throw unsupportedBackend("Vulkan")

  public actual fun attachVulkanBorrowedTexture(
    descriptor: VulkanBorrowedTextureDescriptor
  ): RenderSessionHandle = throw unsupportedBackend("Vulkan")

  public actual fun attachOpenGLOwnedTexture(
    descriptor: OpenGLOwnedTextureDescriptor
  ): RenderSessionHandle = live {
    withWebglContext(descriptor.context) { retention ->
      // The out-handle goes first because it is the only member here that needs eight-byte
      // alignment.
      Heap.withScratch(HANDLE_BYTES + MlnOpenglOwnedTextureDescriptor.SIZEOF) { out ->
        val block = out + HANDLE_BYTES
        // The render marshaller owns the descriptors a session sets on itself, and an owned texture
        // is only ever attached, so its two nested descriptors are placed here.
        MlnOpenglOwnedTextureDescriptor.setSize(block, MlnOpenglOwnedTextureDescriptor.SIZEOF)
        RenderMarshal.writeExtent(
          block + MlnOpenglOwnedTextureDescriptor.OFFSET_EXTENT,
          descriptor.extent,
        )
        RenderMarshal.writeOpenGLContext(
          block + MlnOpenglOwnedTextureDescriptor.OFFSET_CONTEXT,
          descriptor.context,
        )
        attach(::mln_opengl_owned_texture_attach, block, out, retention)
      }
    }
  }

  public actual fun attachOpenGLBorrowedTexture(
    descriptor: OpenGLBorrowedTextureDescriptor
  ): RenderSessionHandle = live {
    withWebglContext(descriptor.context) { retention ->
      Heap.withScratch(HANDLE_BYTES + RenderMarshal.OPENGL_BORROWED_TEXTURE_SIZEOF) { out ->
        val block = out + HANDLE_BYTES
        RenderMarshal.writeOpenGLBorrowedTexture(block, descriptor)
        attach(::mln_opengl_borrowed_texture_attach, block, out, retention)
      }
    }
  }

  public actual fun attachMetalSurface(descriptor: MetalSurfaceDescriptor): RenderSessionHandle =
    throw unsupportedBackend("Metal")

  public actual fun attachVulkanSurface(descriptor: VulkanSurfaceDescriptor): RenderSessionHandle =
    throw unsupportedBackend("Vulkan")

  /**
   * Attaches a surface target that presents through the canvas its WebGL context is bound to.
   *
   * `descriptor.surface` must be the null pointer. Every other OpenGL provider names a drawable
   * there — an HDC, an EGLSurface — and a browser has none to name: the context already selects a
   * canvas, and the session renders into that canvas's default framebuffer. Presenting is the
   * browser compositing that canvas, so a canvas the page displays shows the frame with no copy.
   *
   * The context names an entry in the browser module's own table rather than anything a host could
   * produce, so it comes from [org.maplibre.nativeffi.render.WebglContext], created on this thread.
   */
  public actual fun attachOpenGLSurface(descriptor: OpenGLSurfaceDescriptor): RenderSessionHandle =
    live {
      withWebglContext(descriptor.context) { retention ->
        Heap.withScratch(HANDLE_BYTES + RenderMarshal.OPENGL_SURFACE_SIZEOF) { out ->
          val block = out + HANDLE_BYTES
          RenderMarshal.writeOpenGLSurface(block, descriptor)
          attach(::mln_opengl_surface_attach, block, out, retention)
        }
      }
    }

  public actual fun createProjection(): MapProjectionHandle = live {
    Heap.withScratch(HANDLE_BYTES) { out ->
      Status.check(mln_map_projection_create(handle.raw, out.address))
      MapProjectionHandle.fromNative(NativeMapProjection(Heap.loadLong(out)))
    }
  }

  public actual override fun close() {
    core.closeOnce(
      destroy = { mln_map_destroy(handle.raw) },
      // The registry holds a strong reference, because this target has neither finalization nor a
      // weak reference to hold one with, so the entry goes when the map closes.
      afterSuccess = {
        clearCustomGeometrySources()
        runtime.unregisterMap(this)
        runtimeRetention.close()
      },
    )
  }

  /**
   * Releases the registrations whose sources the newly loaded style dropped.
   *
   * Called when a `MAP_STYLE_LOADED` event is polled, which is the only moment a style set by URL
   * announces that it has replaced the previous one. What decides is whether the id still names a
   * custom vector source, rather than whether the style changed: a style document cannot declare
   * one, so an id that still names one names a source this binding added, and the entry under it is
   * the registration that source was added with.
   */
  internal fun releaseDetachedCustomGeometrySources() {
    if (customGeometrySources.isEmpty()) return
    val detached =
      customGeometrySources.keys.filter { sourceId ->
        styleSourceType(sourceId) != SourceType.CUSTOM_VECTOR
      }
    for (sourceId in detached) customGeometrySources.remove(sourceId)?.close()
  }

  private fun clearCustomGeometrySources() {
    // Emptied before anything is closed, so a close that failed part way through could not leave a
    // registration reachable under an id whose source has already gone.
    val bridges = customGeometrySources.values.toList()
    customGeometrySources.clear()
    for (bridge in bridges) bridge.close()
  }

  /** The native map, for the wrappers this map owns. */
  internal fun nativeHandle(): NativeMap = live { handle }

  internal fun nativeHandleId(): Long = core.handleId()

  internal fun retainChild(childTypeName: String): HandleStateCore.ChildRetention =
    core.retainChild(childTypeName)

  // ---------------------------------------------------------------- shared call shapes
  //
  // Each of these takes the entry point it calls, because the C API spells one shape many times:
  // six existence queries over a string view and a boolean, four property setters over a string
  // view and a JSON tree. The parameters are in C order, so a helper's body reads as the call it
  // makes.

  /** One map argument and one boolean output, which several of these queries share. */
  private fun flagForMap(entry: (Long, Int) -> Int): Boolean = live {
    Heap.withScratch(BOOL_BYTES) { out ->
      Status.check(entry(handle.raw, out.address))
      isSet(out)
    }
  }

  private fun setFlagForMap(entry: (Long, Int) -> Int, value: Boolean) {
    Status.check(entry(handle.raw, if (value) 1 else 0))
  }

  /** One string-view argument and one boolean output, which the existence queries share. */
  private fun flagForId(entry: (Long, Int, Int) -> Int, id: String): Boolean = live {
    withArena(bytes(stringViewBytes(id), blockBytes(BOOL_BYTES))) { arena ->
      val view = writeStringView(arena, id)
      val out = allocate(arena, BOOL_BYTES)
      Status.check(entry(handle.raw, view.address, out.address))
      isSet(out)
    }
  }

  /** One string-view argument and one double, which the layer and indicator setters share. */
  private fun setDoubleForId(entry: (Long, Int, Double) -> Int, id: String, value: Double) {
    withArena(stringViewBytes(id)) { arena ->
      val view = writeStringView(arena, id)
      Status.check(entry(handle.raw, view.address, value))
    }
  }

  /** One string-view argument and one double output, which the zoom-bound getters share. */
  private fun doubleForId(entry: (Long, Int, Int) -> Int, id: String): Double = live {
    withArena(bytes(stringViewBytes(id), blockBytes(DOUBLE_BYTES))) { arena ->
      val view = writeStringView(arena, id)
      val out = allocate(arena, DOUBLE_BYTES)
      Status.check(entry(handle.raw, view.address, out.address))
      Heap.loadDouble(out)
    }
  }

  /** Two string-view arguments, which most of the style mutators take. */
  private fun callWithTwoIds(entry: (Long, Int, Int) -> Int, first: String, second: String) {
    withArena(bytes(stringViewBytes(first), stringViewBytes(second))) { arena ->
      val firstView = writeStringView(arena, first)
      val secondView = writeStringView(arena, second)
      Status.check(entry(handle.raw, firstView.address, secondView.address))
    }
  }

  /** Three string-view arguments, which the typed layer additions take. */
  private fun callWithThreeIds(
    entry: (Long, Int, Int, Int) -> Int,
    first: String,
    second: String,
    third: String,
  ) {
    withArena(bytes(stringViewBytes(first), stringViewBytes(second), stringViewBytes(third))) {
      arena ->
      val firstView = writeStringView(arena, first)
      val secondView = writeStringView(arena, second)
      val thirdView = writeStringView(arena, third)
      Status.check(entry(handle.raw, firstView.address, secondView.address, thirdView.address))
    }
  }

  /**
   * One string-view argument and one JSON tree, which the property setters take.
   *
   * A null [value] reaches native as the null pointer the C API documents, which is how clearing a
   * layer filter is spelled.
   */
  private fun callWithIdAndJson(entry: (Long, Int, Int) -> Int, id: String, value: JsonValue?) {
    withArena(bytes(stringViewBytes(id), value?.let { JsonMarshal.measure(it).toLong() } ?: 0L)) {
      arena ->
      val view = writeStringView(arena, id)
      val root = value?.let { JsonMarshal.write(arena, it) } ?: HeapPointer(0)
      Status.check(entry(handle.raw, view.address, root.address))
    }
  }

  /** One string-view argument and one snapshot output, which the property getters take. */
  private fun jsonSnapshotForId(entry: (Long, Int, Int) -> Int, id: String): JsonValue? {
    val snapshot = live {
      withArena(bytes(stringViewBytes(id), blockBytes(HANDLE_BYTES))) { arena ->
        val view = writeStringView(arena, id)
        val out = allocate(arena, HANDLE_BYTES)
        Status.check(entry(handle.raw, view.address, out.address))
        Heap.loadLong(out)
      }
    }
    return readJsonSnapshot(snapshot)
  }

  private fun boundsForCamera(entry: (Long, Int, Int) -> Int, camera: CameraOptions): LatLngBounds =
    live {
      Heap.withScratch(CameraMarshal.SIZEOF + MlnLatLngBounds.SIZEOF) { descriptor ->
        val out = descriptor + CameraMarshal.SIZEOF
        CameraMarshal.write(descriptor, camera)
        Status.check(entry(handle.raw, descriptor.address, out.address))
        MapOptionsMarshal.readLatLngBounds(out)
      }
    }

  private fun transitionTo(
    entry: (Long, Int, Int) -> Int,
    camera: CameraOptions,
    animation: AnimationOptions?,
  ) {
    withArena(bytes(blockBytes(CameraMarshal.SIZEOF), animationBytes(animation))) { arena ->
      val cameraDescriptor = allocate(arena, CameraMarshal.SIZEOF)
      CameraMarshal.write(cameraDescriptor, camera)
      val animationDescriptor = writeAnimationOptions(arena, animation)
      Status.check(entry(handle.raw, cameraDescriptor.address, animationDescriptor.address))
    }
  }

  /**
   * Holds the WebGL context an OpenGL target names open for as long as [body] and its session need
   * it.
   *
   * Released again when the attach fails, so a refused target leaves nothing holding the context.
   * The retention is idempotent, so releasing it here and in the session is the same release.
   */
  private fun <T> withWebglContext(
    context: OpenGLContextDescriptor,
    body: (HandleStateCore.ChildRetention?) -> T,
  ): T {
    val retention = WebglContext.retainForTarget(context)
    try {
      return body(retention)
    } catch (error: Throwable) {
      retention?.close()
      throw error
    }
  }

  private fun attach(
    entry: (Long, Int, Int) -> Int,
    descriptor: HeapPointer,
    out: HeapPointer,
    contextRetention: HandleStateCore.ChildRetention?,
  ): RenderSessionHandle {
    Status.check(entry(handle.raw, descriptor.address, out.address))
    return RenderSessionHandle.fromNative(
      this,
      NativeRenderSession(Heap.loadLong(out)),
      contextRetention,
    )
  }

  private fun addTileSourceUrl(
    entry: (Long, Int, Int, Int) -> Int,
    sourceId: String,
    url: String,
    options: TileSourceOptions?,
  ) {
    live {
      withArena(
        bytes(stringViewBytes(sourceId), stringViewBytes(url), tileSourceOptionsBytes(options))
      ) { arena ->
        val sourceView = writeStringView(arena, sourceId)
        val urlView = writeStringView(arena, url)
        val descriptor = writeTileSourceOptions(arena, options)
        Status.check(entry(handle.raw, sourceView.address, urlView.address, descriptor.address))
      }
    }
  }

  private fun addTileSourceTiles(
    entry: (Long, Int, Int, Int, Int) -> Int,
    sourceId: String,
    tiles: List<String>,
    options: TileSourceOptions?,
  ) {
    live {
      withArena(
        bytes(
          stringViewBytes(sourceId),
          stringViewArrayBytes(tiles),
          tileSourceOptionsBytes(options),
        )
      ) { arena ->
        val sourceView = writeStringView(arena, sourceId)
        val templates = writeStringViewArray(arena, tiles)
        val descriptor = writeTileSourceOptions(arena, options)
        Status.check(
          entry(handle.raw, sourceView.address, templates.address, tiles.size, descriptor.address)
        )
      }
    }
  }

  private fun copyStyleImageStretches(
    view: HeapPointer,
    stretchX: HeapPointer,
    xCapacity: Int,
    xCount: HeapPointer,
    stretchY: HeapPointer,
    yCapacity: Int,
    yCount: HeapPointer,
    found: HeapPointer,
  ) {
    Status.check(
      mln_map_copy_style_image_stretches(
        handle.raw,
        view.address,
        stretchX.address,
        xCapacity,
        xCount.address,
        stretchY.address,
        yCapacity,
        yCount.address,
        found.address,
      )
    )
  }

  /**
   * Probes the required length and then copies, for the texts a map answers about itself.
   *
   * A null buffer with zero capacity is a size probe the C API answers with OK, so the two calls
   * below are how a caller learns a length it has no descriptor field for.
   */
  private fun copyMapText(entry: (Long, Int, Int, Int) -> Int): String = live {
    val required =
      Heap.withScratch(SIZE_BYTES) { out ->
        Status.check(entry(handle.raw, 0, 0, out.address))
        readCount(Heap.loadInt(out), "map text size")
      }
    if (required == 0) {
      return@live ""
    }
    withArena(bytes(blockBytes(required), blockBytes(SIZE_BYTES))) { arena ->
      val text = allocate(arena, required)
      val copied = allocate(arena, SIZE_BYTES)
      Status.check(entry(handle.raw, text.address, required, copied.address))
      Heap.loadBytes(text, readCount(Heap.loadInt(copied), "map copied text size")).decodeToString()
    }
  }

  /** The same probe-then-copy shape, for the texts a map answers about one layer. */
  private fun copyLayerText(entry: (Long, Int, Int, Int, Int) -> Int, layerId: String): String =
    live {
      val required =
        withArena(bytes(stringViewBytes(layerId), blockBytes(SIZE_BYTES))) { arena ->
          val view = writeStringView(arena, layerId)
          val out = allocate(arena, SIZE_BYTES)
          Status.check(entry(handle.raw, view.address, 0, 0, out.address))
          readCount(Heap.loadInt(out), "layer text size")
        }
      if (required == 0) {
        return@live ""
      }
      withArena(bytes(stringViewBytes(layerId), blockBytes(required), blockBytes(SIZE_BYTES))) {
        arena ->
        val view = writeStringView(arena, layerId)
        val text = allocate(arena, required)
        val copied = allocate(arena, SIZE_BYTES)
        Status.check(entry(handle.raw, view.address, text.address, required, copied.address))
        Heap.loadBytes(text, readCount(Heap.loadInt(copied), "layer copied text size"))
          .decodeToString()
      }
    }

  private fun listStyleIds(entry: (Long, Int) -> Int): List<String> {
    val list = live {
      Heap.withScratch(HANDLE_BYTES) { out ->
        Status.check(entry(handle.raw, out.address))
        Heap.loadLong(out)
      }
    }
    return readStyleIdList(list)
  }

  // ---------------------------------------------------------------- owned results

  /** Copies every ID out of a list and destroys it. */
  private fun readStyleIdList(list: Long): List<String> {
    if (list == 0L) return emptyList()
    try {
      InjectedFaults.beginResultCopy(list, SIZE_BYTES + MlnStringView.SIZEOF)
      return Heap.withScratch(SIZE_BYTES + MlnStringView.SIZEOF) { count ->
        val id = count + SIZE_BYTES
        Status.check(mln_style_id_list_count(list, count.address))
        List(readCount(Heap.loadInt(count), "style ID count")) { index ->
          Status.check(mln_style_id_list_get(list, index, id.address))
          // Copied before the list is destroyed below: the view points into storage the destroy
          // frees.
          JsonMarshal.readText(id)
        }
      }
    } finally {
      mln_style_id_list_destroy(list)
    }
  }

  /** Copies every string out of a list and destroys it. */
  private fun readStyleStringList(list: Long): List<String> {
    if (list == 0L) return emptyList()
    try {
      InjectedFaults.beginResultCopy(list, SIZE_BYTES + MlnStringView.SIZEOF)
      return Heap.withScratch(SIZE_BYTES + MlnStringView.SIZEOF) { count ->
        val value = count + SIZE_BYTES
        Status.check(mln_style_string_list_count(list, count.address))
        List(readCount(Heap.loadInt(count), "style string count")) { index ->
          Status.check(mln_style_string_list_get(list, index, value.address))
          JsonMarshal.readText(value)
        }
      }
    } finally {
      mln_style_string_list_destroy(list)
    }
  }

  /** Copies a JSON snapshot's tree and destroys it. */
  private fun readJsonSnapshot(snapshot: Long): JsonValue? {
    // The C API reports an absent value as the null snapshot rather than as a failure.
    if (snapshot == 0L) return null
    try {
      InjectedFaults.beginResultCopy(snapshot, POINTER_BYTES)
      return Heap.withScratch(POINTER_BYTES) { out ->
        Status.check(mln_json_snapshot_get(snapshot, out.address))
        val root = HeapPointer(Heap.loadInt(out))
        if (root.address == 0) null else JsonMarshal.read(root)
      }
    } finally {
      mln_json_snapshot_destroy(snapshot)
    }
  }

  // ---------------------------------------------------------------- descriptors written here
  //
  // The animation, camera-fit, free-camera, and projection descriptors have no marshaller of their
  // own yet, and this is their only call site. They follow the same rule every descriptor here
  // does: the leading size field carries the size this binding was generated against, and an absent
  // Kotlin value is a field bit left clear rather than a sentinel written into the value.

  private fun animationBytes(animation: AnimationOptions?): Long =
    if (animation == null) 0L else blockBytes(MlnAnimationOptions.SIZEOF)

  /** Returns the null pointer for absent animation, which the C API reads as its own defaults. */
  private fun writeAnimationOptions(arena: HeapArena, animation: AnimationOptions?): HeapPointer {
    if (animation == null) return HeapPointer(0)
    val base = allocate(arena, MlnAnimationOptions.SIZEOF)
    MlnAnimationOptions.setSize(base, MlnAnimationOptions.SIZEOF)
    var fields = 0
    animation.durationMs?.let {
      fields = fields or MlnAnimationOptionField.MLN_ANIMATION_OPTION_DURATION
      MlnAnimationOptions.setDurationMs(base, it)
    }
    animation.velocity?.let {
      fields = fields or MlnAnimationOptionField.MLN_ANIMATION_OPTION_VELOCITY
      MlnAnimationOptions.setVelocity(base, it)
    }
    animation.minZoom?.let {
      fields = fields or MlnAnimationOptionField.MLN_ANIMATION_OPTION_MIN_ZOOM
      MlnAnimationOptions.setMinZoom(base, it)
    }
    animation.easing?.let {
      fields = fields or MlnAnimationOptionField.MLN_ANIMATION_OPTION_EASING
      val easing = base + MlnAnimationOptions.OFFSET_EASING
      MlnUnitBezier.setX1(easing, it.x1)
      MlnUnitBezier.setY1(easing, it.y1)
      MlnUnitBezier.setX2(easing, it.x2)
      MlnUnitBezier.setY2(easing, it.y2)
    }
    animation.transitionId?.let {
      fields = fields or MlnAnimationOptionField.MLN_ANIMATION_OPTION_TRANSITION_ID
      MlnAnimationOptions.setTransitionId(base, it)
    }
    MlnAnimationOptions.setFields(base, fields)
    return base
  }

  private fun fitOptionsBytes(fitOptions: CameraFitOptions?): Long =
    if (fitOptions == null) 0L else blockBytes(MlnCameraFitOptions.SIZEOF)

  /** Returns the null pointer for absent fit options, which the C API reads as its own defaults. */
  private fun writeCameraFitOptions(arena: HeapArena, fitOptions: CameraFitOptions?): HeapPointer {
    if (fitOptions == null) return HeapPointer(0)
    val base = allocate(arena, MlnCameraFitOptions.SIZEOF)
    MlnCameraFitOptions.setSize(base, MlnCameraFitOptions.SIZEOF)
    var fields = 0
    fitOptions.padding?.let {
      fields = fields or MlnCameraFitOptionField.MLN_CAMERA_FIT_OPTION_PADDING
      CameraMarshal.writeEdgeInsets(base + MlnCameraFitOptions.OFFSET_PADDING, it)
    }
    fitOptions.bearing?.let {
      fields = fields or MlnCameraFitOptionField.MLN_CAMERA_FIT_OPTION_BEARING
      MlnCameraFitOptions.setBearing(base, it)
    }
    fitOptions.pitch?.let {
      fields = fields or MlnCameraFitOptionField.MLN_CAMERA_FIT_OPTION_PITCH
      MlnCameraFitOptions.setPitch(base, it)
    }
    MlnCameraFitOptions.setFields(base, fields)
    return base
  }

  private fun writeFreeCameraOptions(base: HeapPointer, options: FreeCameraOptions) {
    MlnFreeCameraOptions.setSize(base, MlnFreeCameraOptions.SIZEOF)
    var fields = 0
    options.position?.let {
      fields = fields or MlnFreeCameraOptionField.MLN_FREE_CAMERA_OPTION_POSITION
      val position = base + MlnFreeCameraOptions.OFFSET_POSITION
      MlnVec3.setX(position, it.x)
      MlnVec3.setY(position, it.y)
      MlnVec3.setZ(position, it.z)
    }
    options.orientation?.let {
      fields = fields or MlnFreeCameraOptionField.MLN_FREE_CAMERA_OPTION_ORIENTATION
      val orientation = base + MlnFreeCameraOptions.OFFSET_ORIENTATION
      MlnQuaternion.setX(orientation, it.x)
      MlnQuaternion.setY(orientation, it.y)
      MlnQuaternion.setZ(orientation, it.z)
      MlnQuaternion.setW(orientation, it.w)
    }
    MlnFreeCameraOptions.setFields(base, fields)
  }

  private fun readFreeCameraOptions(base: HeapPointer): FreeCameraOptions {
    val fields = MlnFreeCameraOptions.fields(base)
    return FreeCameraOptions().also {
      if ((fields and MlnFreeCameraOptionField.MLN_FREE_CAMERA_OPTION_POSITION) != 0) {
        val position = base + MlnFreeCameraOptions.OFFSET_POSITION
        it.position = Vec3(MlnVec3.x(position), MlnVec3.y(position), MlnVec3.z(position))
      }
      if ((fields and MlnFreeCameraOptionField.MLN_FREE_CAMERA_OPTION_ORIENTATION) != 0) {
        val orientation = base + MlnFreeCameraOptions.OFFSET_ORIENTATION
        it.orientation =
          Quaternion(
            MlnQuaternion.x(orientation),
            MlnQuaternion.y(orientation),
            MlnQuaternion.z(orientation),
            MlnQuaternion.w(orientation),
          )
      }
    }
  }

  private fun writeProjectionMode(base: HeapPointer, mode: ProjectionModeOptions) {
    MlnProjectionMode.setSize(base, MlnProjectionMode.SIZEOF)
    var fields = 0
    mode.axonometric?.let {
      fields = fields or MlnProjectionModeField.MLN_PROJECTION_MODE_AXONOMETRIC
      MlnProjectionMode.setAxonometric(base, it)
    }
    mode.xSkew?.let {
      fields = fields or MlnProjectionModeField.MLN_PROJECTION_MODE_X_SKEW
      MlnProjectionMode.setXSkew(base, it)
    }
    mode.ySkew?.let {
      fields = fields or MlnProjectionModeField.MLN_PROJECTION_MODE_Y_SKEW
      MlnProjectionMode.setYSkew(base, it)
    }
    MlnProjectionMode.setFields(base, fields)
  }

  private fun readProjectionMode(base: HeapPointer): ProjectionModeOptions {
    val fields = MlnProjectionMode.fields(base)
    return ProjectionModeOptions().also {
      if ((fields and MlnProjectionModeField.MLN_PROJECTION_MODE_AXONOMETRIC) != 0) {
        it.axonometric = MlnProjectionMode.axonometric(base)
      }
      if ((fields and MlnProjectionModeField.MLN_PROJECTION_MODE_X_SKEW) != 0) {
        it.xSkew = MlnProjectionMode.xSkew(base)
      }
      if ((fields and MlnProjectionModeField.MLN_PROJECTION_MODE_Y_SKEW) != 0) {
        it.ySkew = MlnProjectionMode.ySkew(base)
      }
    }
  }

  private fun writeCanonicalTileId(arena: HeapArena, tileId: CanonicalTileId): HeapPointer {
    val base = allocate(arena, MlnCanonicalTileId.SIZEOF)
    MlnCanonicalTileId.setZ(base, tileId.z)
    // The C fields are unsigned 32-bit and the Kotlin ones are Long so that the whole domain fits,
    // so these carry the bit pattern rather than a converted value. The public type already
    // refuses anything outside that domain.
    MlnCanonicalTileId.setX(base, tileId.x.toInt())
    MlnCanonicalTileId.setY(base, tileId.y.toInt())
    return base
  }

  /**
   * Bytes [options] needs, including the cluster properties the descriptor borrows.
   *
   * The source marshaller writes the descriptor but does not place the graph, because the graph is
   * a JSON tree and that file owns none of the JSON arithmetic. It is measured and placed here, in
   * the same block, so it lives exactly as long as the call that points at it.
   */
  private fun geoJsonSourceOptionsBytes(options: GeoJsonSourceOptions?): Long {
    if (options == null) return 0L
    val clusterProperties =
      options.clusterProperties?.let { JsonMarshal.measure(it).toLong() } ?: 0L
    return JsonMarshal.plus(
      blockBytes(SourceMarshal.GEOJSON_SOURCE_OPTIONS_SIZEOF),
      clusterProperties,
    )
  }

  /** Returns the null pointer for absent options, which the C API reads as its own defaults. */
  private fun writeGeoJsonSourceOptions(
    arena: HeapArena,
    options: GeoJsonSourceOptions?,
  ): HeapPointer {
    if (options == null) return HeapPointer(0)
    val base = allocate(arena, SourceMarshal.GEOJSON_SOURCE_OPTIONS_SIZEOF)
    val clusterProperties = options.clusterProperties?.let { JsonMarshal.write(arena, it) }
    SourceMarshal.writeGeoJsonSourceOptions(base, options, clusterProperties)
    return base
  }

  private fun tileSourceOptionsBytes(options: TileSourceOptions?): Long =
    if (options == null) 0L else SourceMarshal.measureTileSourceOptions(options).toLong()

  private fun writeTileSourceOptions(arena: HeapArena, options: TileSourceOptions?): HeapPointer =
    if (options == null) HeapPointer(0) else SourceMarshal.writeTileSourceOptions(arena, options)

  // ---------------------------------------------------------------- arrays and scalars

  private fun writeLatLngs(arena: HeapArena, coordinates: List<LatLng>): HeapPointer {
    val block = allocate(arena, Heap.sizeOf(MlnLatLng.SIZEOF, coordinates.size))
    coordinates.forEachIndexed { index, coordinate ->
      CameraMarshal.writeLatLng(block + index * MlnLatLng.SIZEOF, coordinate)
    }
    return block
  }

  private fun readLatLngs(base: HeapPointer, count: Int): List<LatLng> =
    List(count) { index -> CameraMarshal.readLatLng(base + index * MlnLatLng.SIZEOF) }

  private fun readStretches(base: HeapPointer, count: Int): List<ImageStretch> =
    List(count) { index ->
      StyleMarshal.readImageStretch(base + index * StyleMarshal.IMAGE_STRETCH_SIZEOF)
    }

  private fun anchorBytes(anchor: ScreenPoint?): Long =
    if (anchor == null) 0L else blockBytes(MlnScreenPoint.SIZEOF)

  /** Returns the null pointer for an absent anchor, which the C API reads as the screen centre. */
  private fun writeScreenPointOrNull(arena: HeapArena, point: ScreenPoint?): HeapPointer =
    if (point == null) HeapPointer(0) else writeScreenPoint(arena, point)

  private fun writeScreenPoint(arena: HeapArena, point: ScreenPoint): HeapPointer {
    val base = allocate(arena, MlnScreenPoint.SIZEOF)
    MlnScreenPoint.setX(base, point.x)
    MlnScreenPoint.setY(base, point.y)
    return base
  }

  private fun stringViewBytes(text: String): Long =
    JsonMarshal.plus(blockBytes(MlnStringView.SIZEOF), JsonMarshal.measureText(text))

  /**
   * Writes a string view the caller passes by address.
   *
   * A C argument of type `mln_string_view` is passed by value, which this target lowers to a
   * pointer to the view, so the view itself needs storage of its own alongside its bytes.
   */
  private fun writeStringView(arena: HeapArena, text: String): HeapPointer {
    val view = allocate(arena, MlnStringView.SIZEOF)
    JsonMarshal.writeText(arena, view, text)
    return view
  }

  private fun stringViewArrayBytes(values: List<String>): Long =
    values.fold(JsonMarshal.measureArray(MlnStringView.SIZEOF, values.size)) { total, value ->
      JsonMarshal.plus(total, JsonMarshal.measureText(value))
    }

  private fun writeStringViewArray(arena: HeapArena, values: List<String>): HeapPointer {
    val block = JsonMarshal.allocateArray(arena, MlnStringView.SIZEOF, values.size)
    values.forEachIndexed { index, value ->
      JsonMarshal.writeText(arena, block + index * MlnStringView.SIZEOF, value)
    }
    return block
  }

  private fun isSet(flag: HeapPointer): Boolean = Heap.loadByte(flag) != 0.toByte()

  // ---------------------------------------------------------------- arena arithmetic

  /**
   * Takes one measured block and carves [body]'s descriptors out of it.
   *
   * The block starts zeroed, so an output slot inside it is already the null handle the C API
   * requires callers to pass.
   */
  private fun <T> withArena(bytes: Long, body: (HeapArena) -> T): T {
    Status.requireArgument(bytes in 0..Int.MAX_VALUE.toLong()) {
      "a descriptor block must be non-negative and addressable on this target"
    }
    // Zero is the ordinary case for a call whose descriptors are all optional and all absent:
    // `scaleBy(scale, null)` passes a null anchor, which the C API reads as the screen centre, so
    // there is nothing to place. An empty arena rather than an empty allocation, because the
    // module's allocator has no zero-sized block to give and none is wanted. Any allocation
    // against this fails the arena's own bounds check, which is what a measure of zero followed by
    // a write should do.
    if (bytes == 0L) return body(HeapArena(HeapPointer(0), 0))
    val size = bytes.toInt()
    return Heap.withScratch(size) { block -> body(HeapArena(block, size)) }
  }

  /**
   * Adds measured sizes through the one checked addition this binding uses.
   *
   * The arithmetic lives in [JsonMarshal] rather than being repeated here, because a second copy is
   * a second place for an unchecked subtotal to appear.
   */
  private fun bytes(vararg sizes: Long): Long =
    sizes.fold(0L) { total, size -> JsonMarshal.plus(total, size) }

  /** Bytes one block occupies in these arenas, including the padding that follows it. */
  private fun blockBytes(size: Int): Long = JsonMarshal.measureBlock(size)

  /** Reserves the block [blockBytes] accounted for. */
  private fun allocate(arena: HeapArena, size: Int): HeapPointer =
    JsonMarshal.allocateBlock(arena, size)

  /** Rejects a string C would truncate when it is passed as null-terminated text. */
  private fun requireValidCString(value: String, subject: String) {
    Heap.requireCString(value, subject)
  }

  /**
   * Refuses a count native reported that no real result could carry.
   *
   * `size_t` is 32 bits on this target, so a value past [Int.MAX_VALUE] arrives negative. The heap
   * could not hold a result that large, so a negative one means the address being read is not the
   * out-parameter it was taken for, and continuing would size a buffer from a number that is not a
   * length.
   */
  private fun readCount(count: Int, subject: String): Int {
    if (count < 0) {
      throw Status.invalidState("The MapLibre Native browser module reported a $subject of $count")
    }
    return count
  }

  /** The same refusal for a length this binding already widened out of a descriptor field. */
  private fun readLength(length: Long, subject: String): Int {
    if (length < 0 || length > Int.MAX_VALUE) {
      throw Status.invalidState("The MapLibre Native browser module reported a $subject of $length")
    }
    return length.toInt()
  }

  private fun unsupportedBackend(backend: String): UnsupportedFeatureException =
    UnsupportedFeatureException(
      MaplibreStatus.UNSUPPORTED.nativeCode,
      "$backend render targets are not supported by the browser build of MapLibre Native, " +
        "which compiles OpenGL against WebGL",
    )

  public actual companion object {
    public actual fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle =
      Heap.withScratch(HANDLE_BYTES + MapOptionsMarshal.MAP_OPTIONS_SIZEOF) { out ->
        val descriptor = out + HANDLE_BYTES
        MapOptionsMarshal.writeMapOptions(descriptor, options)
        Status.check(mln_map_create(runtime.nativeHandle().raw, descriptor.address, out.address))
        MapHandle(runtime, NativeMap(Heap.loadLong(out))).also(runtime::registerMap)
      }
  }
}
