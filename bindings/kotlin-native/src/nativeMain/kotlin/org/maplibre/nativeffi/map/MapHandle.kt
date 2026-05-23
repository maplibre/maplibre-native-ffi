package org.maplibre.nativeffi.map

import cnames.structs.mln_map
import cnames.structs.mln_style_id_list
import kotlinx.cinterop.BooleanVar
import kotlinx.cinterop.ByteVar
import kotlinx.cinterop.CPointer
import kotlinx.cinterop.CPointerVarOf
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UIntVar
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.allocArray
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.pointed
import kotlinx.cinterop.ptr
import kotlinx.cinterop.readBytes
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.value
import org.maplibre.nativeffi.camera.AnimationOptions
import org.maplibre.nativeffi.camera.BoundOptions
import org.maplibre.nativeffi.camera.CameraOptions
import org.maplibre.nativeffi.camera.FreeCameraOptions
import org.maplibre.nativeffi.geo.GeoJson
import org.maplibre.nativeffi.geo.LatLng
import org.maplibre.nativeffi.geo.ScreenPoint
import org.maplibre.nativeffi.internal.c.mln_bound_options_default
import org.maplibre.nativeffi.internal.c.mln_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_free_camera_options_default
import org.maplibre.nativeffi.internal.c.mln_lat_lng
import org.maplibre.nativeffi.internal.c.mln_map_add_geojson_source_data
import org.maplibre.nativeffi.internal.c.mln_map_add_geojson_source_url
import org.maplibre.nativeffi.internal.c.mln_map_add_style_layer_json
import org.maplibre.nativeffi.internal.c.mln_map_add_style_source_json
import org.maplibre.nativeffi.internal.c.mln_map_cancel_transitions
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
import org.maplibre.nativeffi.internal.c.mln_map_get_projection_mode
import org.maplibre.nativeffi.internal.c.mln_map_get_rendering_stats_view_enabled
import org.maplibre.nativeffi.internal.c.mln_map_get_style_source_info
import org.maplibre.nativeffi.internal.c.mln_map_get_style_source_type
import org.maplibre.nativeffi.internal.c.mln_map_get_tile_options
import org.maplibre.nativeffi.internal.c.mln_map_get_viewport_options
import org.maplibre.nativeffi.internal.c.mln_map_is_fully_loaded
import org.maplibre.nativeffi.internal.c.mln_map_jump_to
import org.maplibre.nativeffi.internal.c.mln_map_lat_lng_for_pixel
import org.maplibre.nativeffi.internal.c.mln_map_lat_lngs_for_pixels
import org.maplibre.nativeffi.internal.c.mln_map_list_style_source_ids
import org.maplibre.nativeffi.internal.c.mln_map_move_by
import org.maplibre.nativeffi.internal.c.mln_map_move_by_animated
import org.maplibre.nativeffi.internal.c.mln_map_options
import org.maplibre.nativeffi.internal.c.mln_map_options_default
import org.maplibre.nativeffi.internal.c.mln_map_pitch_by
import org.maplibre.nativeffi.internal.c.mln_map_pitch_by_animated
import org.maplibre.nativeffi.internal.c.mln_map_pixel_for_lat_lng
import org.maplibre.nativeffi.internal.c.mln_map_pixels_for_lat_lngs
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_layer
import org.maplibre.nativeffi.internal.c.mln_map_remove_style_source
import org.maplibre.nativeffi.internal.c.mln_map_request_repaint
import org.maplibre.nativeffi.internal.c.mln_map_request_still_image
import org.maplibre.nativeffi.internal.c.mln_map_rotate_by
import org.maplibre.nativeffi.internal.c.mln_map_rotate_by_animated
import org.maplibre.nativeffi.internal.c.mln_map_scale_by
import org.maplibre.nativeffi.internal.c.mln_map_scale_by_animated
import org.maplibre.nativeffi.internal.c.mln_map_set_bounds
import org.maplibre.nativeffi.internal.c.mln_map_set_debug_options
import org.maplibre.nativeffi.internal.c.mln_map_set_free_camera_options
import org.maplibre.nativeffi.internal.c.mln_map_set_geojson_source_data
import org.maplibre.nativeffi.internal.c.mln_map_set_geojson_source_url
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_filter
import org.maplibre.nativeffi.internal.c.mln_map_set_layer_property
import org.maplibre.nativeffi.internal.c.mln_map_set_projection_mode
import org.maplibre.nativeffi.internal.c.mln_map_set_rendering_stats_view_enabled
import org.maplibre.nativeffi.internal.c.mln_map_set_style_json
import org.maplibre.nativeffi.internal.c.mln_map_set_style_light_json
import org.maplibre.nativeffi.internal.c.mln_map_set_style_light_property
import org.maplibre.nativeffi.internal.c.mln_map_set_style_url
import org.maplibre.nativeffi.internal.c.mln_map_set_tile_options
import org.maplibre.nativeffi.internal.c.mln_map_set_viewport_options
import org.maplibre.nativeffi.internal.c.mln_map_style_layer_exists
import org.maplibre.nativeffi.internal.c.mln_map_style_source_exists
import org.maplibre.nativeffi.internal.c.mln_map_tile_options_default
import org.maplibre.nativeffi.internal.c.mln_map_viewport_options_default
import org.maplibre.nativeffi.internal.c.mln_projection_mode_default
import org.maplibre.nativeffi.internal.c.mln_screen_point
import org.maplibre.nativeffi.internal.c.mln_style_source_info
import org.maplibre.nativeffi.internal.lifecycle.HandleState
import org.maplibre.nativeffi.internal.memory.MemoryUtil
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.struct.CoreStructs
import org.maplibre.nativeffi.internal.struct.MapStructs
import org.maplibre.nativeffi.internal.struct.StyleStructs
import org.maplibre.nativeffi.internal.struct.ValueStructs
import org.maplibre.nativeffi.json.JsonValue
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.style.SourceInfo
import org.maplibre.nativeffi.style.SourceType

/** Owned native map handle. Close it on the map owner thread. */
@OptIn(ExperimentalForeignApi::class)
public class MapHandle
private constructor(private val runtime: RuntimeHandle, handle: CPointer<mln_map>) : AutoCloseable {
  private val state = HandleState("MapHandle", handle, runtime)

  public fun setStyleUrl(url: String) {
    MemoryUtil.requireValidCString(url)
    Status.check(mln_map_set_style_url(state.requireLive(), url))
  }

  public fun setStyleJson(json: String) {
    MemoryUtil.requireValidCString(json)
    Status.check(mln_map_set_style_json(state.requireLive(), json))
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
    outRemoved.value
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

  private fun copyStyleSourceAttribution(sourceId: String, info: mln_style_source_info): String? {
    if (!info.has_attribution) return null
    if (info.attribution_size == 0UL) return ""
    return memScoped {
      val outAttribution = allocArray<ByteVar>(info.attribution_size.toInt())
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
        outAttribution.readBytes(outAttributionSize.value.toInt()).decodeToString()
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

  public fun requestRepaint() {
    Status.check(mln_map_request_repaint(state.requireLive()))
  }

  public fun requestStillImage() {
    Status.check(mln_map_request_still_image(state.requireLive()))
  }

  public fun setDebugOptions(options: Set<DebugOption>) {
    val mask = options.fold(0U) { acc, option -> acc or option.nativeMask }
    Status.check(mln_map_set_debug_options(state.requireLive(), mask))
  }

  public fun debugOptions(): Set<DebugOption> = memScoped {
    val outOptions = alloc<UIntVar>()
    Status.check(mln_map_get_debug_options(state.requireLive(), outOptions.ptr))
    DebugOption.entries.filterTo(mutableSetOf()) { (outOptions.value and it.nativeMask) != 0U }
  }

  public fun setRenderingStatsViewEnabled(enabled: Boolean) {
    Status.check(mln_map_set_rendering_stats_view_enabled(state.requireLive(), enabled))
  }

  public fun isRenderingStatsViewEnabled(): Boolean = memScoped {
    val outEnabled = alloc<BooleanVar>()
    Status.check(mln_map_get_rendering_stats_view_enabled(state.requireLive(), outEnabled.ptr))
    outEnabled.value
  }

  public fun isFullyLoaded(): Boolean = memScoped {
    val outLoaded = alloc<BooleanVar>()
    Status.check(mln_map_is_fully_loaded(state.requireLive(), outLoaded.ptr))
    outLoaded.value
  }

  public fun dumpDebugLogs() {
    Status.check(mln_map_dump_debug_logs(state.requireLive()))
  }

  public fun viewportOptions(): ViewportOptions = memScoped {
    val outOptions = mln_map_viewport_options_default().getPointer(this)
    Status.check(mln_map_get_viewport_options(state.requireLive(), outOptions))
    MapStructs.viewportOptions(outOptions.pointed)
  }

  public fun setViewportOptions(options: ViewportOptions) {
    memScoped {
      Status.check(
        mln_map_set_viewport_options(state.requireLive(), MapStructs.viewportOptions(options, this))
      )
    }
  }

  public fun tileOptions(): TileOptions = memScoped {
    val outOptions = mln_map_tile_options_default().getPointer(this)
    Status.check(mln_map_get_tile_options(state.requireLive(), outOptions))
    MapStructs.tileOptions(outOptions.pointed)
  }

  public fun setTileOptions(options: TileOptions) {
    memScoped {
      Status.check(
        mln_map_set_tile_options(state.requireLive(), MapStructs.tileOptions(options, this))
      )
    }
  }

  public fun camera(): CameraOptions = memScoped {
    val outCamera = mln_camera_options_default().getPointer(this)
    Status.check(mln_map_get_camera(state.requireLive(), outCamera))
    MapStructs.cameraOptions(outCamera.pointed)
  }

  public fun jumpTo(camera: CameraOptions) {
    memScoped {
      Status.check(mln_map_jump_to(state.requireLive(), MapStructs.cameraOptions(camera, this)))
    }
  }

  public fun easeTo(camera: CameraOptions) {
    easeTo(camera, null)
  }

  public fun easeTo(camera: CameraOptions, animation: AnimationOptions?) {
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

  public fun flyTo(camera: CameraOptions) {
    flyTo(camera, null)
  }

  public fun flyTo(camera: CameraOptions, animation: AnimationOptions?) {
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

  public fun moveByAnimated(deltaX: Double, deltaY: Double) {
    moveByAnimated(deltaX, deltaY, null)
  }

  public fun moveByAnimated(deltaX: Double, deltaY: Double, animation: AnimationOptions?) {
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

  public fun scaleBy(scale: Double) {
    scaleBy(scale, null)
  }

  public fun scaleBy(scale: Double, anchor: ScreenPoint?) {
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

  public fun rotateByAnimated(first: ScreenPoint, second: ScreenPoint) {
    rotateByAnimated(first, second, null)
  }

  public fun rotateByAnimated(
    first: ScreenPoint,
    second: ScreenPoint,
    animation: AnimationOptions?,
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

  public fun pitchByAnimated(pitch: Double) {
    pitchByAnimated(pitch, null)
  }

  public fun pitchByAnimated(pitch: Double, animation: AnimationOptions?) {
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

  public fun bounds(): BoundOptions = memScoped {
    val outOptions = mln_bound_options_default().getPointer(this)
    Status.check(mln_map_get_bounds(state.requireLive(), outOptions))
    MapStructs.boundOptions(outOptions.pointed)
  }

  public fun setBounds(options: BoundOptions) {
    memScoped {
      Status.check(mln_map_set_bounds(state.requireLive(), MapStructs.boundOptions(options, this)))
    }
  }

  public fun freeCameraOptions(): FreeCameraOptions = memScoped {
    val outOptions = mln_free_camera_options_default().getPointer(this)
    Status.check(mln_map_get_free_camera_options(state.requireLive(), outOptions))
    MapStructs.freeCameraOptions(outOptions.pointed)
  }

  public fun setFreeCameraOptions(options: FreeCameraOptions) {
    memScoped {
      Status.check(
        mln_map_set_free_camera_options(
          state.requireLive(),
          MapStructs.freeCameraOptions(options, this),
        )
      )
    }
  }

  public fun projectionMode(): ProjectionModeOptions = memScoped {
    val outMode = mln_projection_mode_default().getPointer(this)
    Status.check(mln_map_get_projection_mode(state.requireLive(), outMode))
    MapStructs.projectionModeOptions(outMode.pointed)
  }

  public fun setProjectionMode(mode: ProjectionModeOptions) {
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
    if (coordinates.isEmpty()) return@memScoped emptyList()
    val outPoints = allocArray<mln_screen_point>(coordinates.size)
    Status.check(
      mln_map_pixels_for_lat_lngs(
        state.requireLive(),
        CoreStructs.latLngArray(coordinates, this),
        coordinates.size.toULong(),
        outPoints,
      )
    )
    CoreStructs.screenPointArray(outPoints, coordinates.size)
  }

  public fun latLngsForPixels(points: List<ScreenPoint>): List<LatLng> = memScoped {
    if (points.isEmpty()) return@memScoped emptyList()
    val outCoordinates = allocArray<mln_lat_lng>(points.size)
    Status.check(
      mln_map_lat_lngs_for_pixels(
        state.requireLive(),
        CoreStructs.screenPointArray(points, this),
        points.size.toULong(),
        outCoordinates,
      )
    )
    CoreStructs.latLngArray(outCoordinates, points.size)
  }

  public fun createProjection(): MapProjectionHandle = MapProjectionHandle.create(this)

  override fun close() {
    state.closeOnce(::mln_map_destroy) { runtime.unregisterMap(this) }
  }

  public fun isClosed(): Boolean = state.isReleased()

  public fun runtime(): RuntimeHandle = runtime

  internal fun nativeHandle(): CPointer<mln_map> = state.requireLive()

  internal fun nativeAddress(): Long = state.address()

  public companion object {
    public fun create(runtime: RuntimeHandle, options: MapOptions): MapHandle = memScoped {
      val nativeOptions = alloc<mln_map_options>()
      mln_map_options_default().place(nativeOptions.ptr)
      options.width?.let { nativeOptions.width = it }
      options.height?.let { nativeOptions.height = it }
      options.scaleFactor?.let { nativeOptions.scale_factor = it }
      options.mapMode?.let { nativeOptions.map_mode = it.nativeValue }

      val outMap = alloc<CPointerVarOf<CPointer<mln_map>>>()
      outMap.value = null
      Status.check(mln_map_create(runtime.nativeHandle(), nativeOptions.ptr, outMap.ptr))
      val map = MapHandle(runtime, requireNotNull(outMap.value) { "mln_map_create returned null" })
      runtime.registerMap(map)
      map
    }
  }
}
