// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*
import org.maplibre.nativeffi.render.NativePointer

public object GeneratedApi {
  /**
   * Initializes Android platform services.
   *
   * See `mln_android_init` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
   */
  public fun androidInit(
    jniEnv: NativePointer,
    jniClass: NativePointer,
    context: NativePointer,
  ): Unit =
    nativeCall(null, null, "mln_android_init") {
      check(C.mln_android_init(jniEnv.address, jniClass.address, context.address, diagnostic))
    }

  /**
   * Returns empty animation options initialized for this C API version.
   *
   * See `mln_animation_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun animationOptionsDefault(): AnimationOptions =
    nativeCall(null, null, "mln_animation_options_default") {
      val out = sized(72, 8)
      C.mln_animation_options_default(out)
      readAnimationOptions(out)
    }

  /**
   * Returns empty map bound options initialized for this C API version.
   *
   * See `mln_bound_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun boundOptionsDefault(): BoundOptions =
    nativeCall(null, null, "mln_bound_options_default") {
      val out = sized(72, 8)
      C.mln_bound_options_default(out)
      readBoundOptions(out)
    }

  /**
   * Reports the C ABI contract version. The value is 0 while the ABI is unstable, and will
   * increment on each SemVer major release.
   *
   * See `mln_c_version` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
   */
  public fun cVersion(): UInt =
    nativeCall(null, null, "mln_c_version") {
      val raw = C.mln_c_version()
      raw.toUInt()
    }

  /**
   * Returns an empty relative camera update initialized for this API version.
   *
   * See `mln_camera_delta_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraDeltaDefault(): CameraDelta =
    nativeCall(null, null, "mln_camera_delta_default") {
      val out = sized(128, 8)
      C.mln_camera_delta_default(out)
      readCameraDelta(out)
    }

  /**
   * Returns empty camera fitting options initialized for this C API version.
   *
   * See `mln_camera_fit_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraFitOptionsDefault(): CameraFitOptions =
    nativeCall(null, null, "mln_camera_fit_options_default") {
      val out = sized(56, 8)
      C.mln_camera_fit_options_default(out)
      readCameraFitOptions(out)
    }

  /**
   * Returns empty camera options initialized for this C API version.
   *
   * See `mln_camera_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraOptionsDefault(): CameraOptions =
    nativeCall(null, null, "mln_camera_options_default") {
      val out = sized(120, 8)
      C.mln_camera_options_default(out)
      readCameraOptions(out)
    }

  /**
   * Returns an empty atomic camera update initialized for this API version.
   *
   * See `mln_camera_update_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun cameraUpdateDefault(): CameraUpdate =
    nativeCall(null, null, "mln_camera_update_default") {
      val out = sized(208, 8)
      C.mln_camera_update_default(out)
      readCameraUpdate(out)
    }

  /**
   * Returns default custom geometry source options.
   *
   * See `mln_custom_geometry_source_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun customGeometrySourceOptionsDefault(): CustomGeometrySourceOptions =
    nativeCall(null, null, "mln_custom_geometry_source_options_default") {
      val out = sized(w(64, 80), 8)
      C.mln_custom_geometry_source_options_default(out)
      readCustomGeometrySourceOptions(out)
    }

  /**
   * Returns default custom MVT vector source options.
   *
   * See `mln_custom_mvt_vector_source_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun customMvtVectorSourceOptionsDefault(): CustomMvtVectorSourceOptions =
    nativeCall(null, null, "mln_custom_mvt_vector_source_options_default") {
      val out = sized(w(48, 56), 8)
      C.mln_custom_mvt_vector_source_options_default(out)
      readCustomMvtVectorSourceOptions(out)
    }

  /**
   * Returns a zero-token, render-if-needed, nonpresenting frame demand.
   *
   * See `mln_frame_demand_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
   */
  public fun frameDemandDefault(): FrameDemand =
    nativeCall(null, null, "mln_frame_demand_default") {
      val out = sized(32, 8)
      C.mln_frame_demand_default(out)
      readFrameDemand(out)
    }

  /**
   * Returns empty free camera options initialized for this C API version.
   *
   * See `mln_free_camera_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun freeCameraOptionsDefault(): FreeCameraOptions =
    nativeCall(null, null, "mln_free_camera_options_default") {
      val out = sized(64, 8)
      C.mln_free_camera_options_default(out)
      readFreeCameraOptions(out)
    }

  /**
   * Prepares GeoJSON source data for installation on a map.
   *
   * See `mln_geojson_source_data_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun geojsonSourceDataCreate(
    data: ByteArray,
    options: GeojsonSourceOptions? = null,
  ): GeojsonSourceDataHandle =
    nativeCall(null, null, "mln_geojson_source_data_create") {
      val out = allocate(8, 8)
      check(
        C.mln_geojson_source_data_create(
          view(data),
          options?.let { writeGeojsonSourceOptions(it) } ?: 0L,
          out,
          diagnostic,
        )
      )
      adopt(out, GeneratedOwnerDisposal::geojsonSourceData) { GeojsonSourceDataHandle(it) }
    }

  /**
   * Returns default GeoJSON source options.
   *
   * See `mln_geojson_source_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun geojsonSourceOptionsDefault(): GeojsonSourceOptions =
    nativeCall(null, null, "mln_geojson_source_options_default") {
      val out = sized(w(72, 80), 8)
      C.mln_geojson_source_options_default(out)
      readGeojsonSourceOptions(out)
    }

  /**
   * Returns CPU-complete synchronization for this C API version.
   *
   * See `mln_gpu_sync_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
   */
  public fun gpuSyncDefault(): GpuSync =
    nativeCall(null, null, "mln_gpu_sync_default") {
      val out = sized(24, 8)
      C.mln_gpu_sync_default(out)
      readGpuSync(out)
    }

  /**
   * Sets one outgoing HTTP request header for the current transform invocation.
   *
   * See `mln_http_header_transform_response_set` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun httpHeaderTransformResponseSet(
    response: HttpHeaderTransformResponse,
    name: String,
    valueValue: String,
  ): Unit =
    nativeRespond(
      response.bindingScope,
      response.bindingAddress,
      "mln_http_header_transform_response_set",
    ) {
      val nameUtf8 = name.encodeToByteArray()
      val valueValueUtf8 = valueValue.encodeToByteArray()
      check(
        C.mln_http_header_transform_response_set(
          handle,
          bytes(nameUtf8),
          nameUtf8.size.toLong(),
          bytes(valueValueUtf8),
          valueValueUtf8.size.toLong(),
          diagnostic,
        )
      )
    }

  /**
   * Converts spherical Mercator projected meters to a geographic coordinate.
   *
   * See `mln_lat_lng_for_projected_meters` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng =
    nativeCall(null, null, "mln_lat_lng_for_projected_meters") {
      val out = allocate(16, 8)
      check(C.mln_lat_lng_for_projected_meters(writeProjectedMeters(meters), out, diagnostic))
      readLatLng(out)
    }

  /**
   * Clears the process-global log callback.
   *
   * See `mln_log_clear_callback` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
   */
  public fun logClearCallback(): Unit =
    nativeCall(null, null, "mln_log_clear_callback") { check(C.mln_log_clear_callback(diagnostic)) }

  /**
   * Controls which log severities MapLibre Native may dispatch asynchronously.
   *
   * See `mln_log_set_async_severity_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
   */
  public fun logSetAsyncSeverityMask(mask: LogSeverityMask): Unit =
    nativeCall(null, null, "mln_log_set_async_severity_mask") {
      check(C.mln_log_set_async_severity_mask(mask.rawValue.toInt(), diagnostic))
    }

  /**
   * Installs a process-global MapLibre Native log callback.
   *
   * See `mln_log_set_callback` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
   */
  public fun logSetCallback(callback: LogCallback): Unit =
    nativeCall(null, null, "mln_log_set_callback") {
      val token = registrations.register(GeneratedLogCallbackRegistration(callback))
      check(
        C.mln_log_set_callback(UpcallStubs.logCallback, token, UpcallStubs.releaseRoot, diagnostic)
      )
      accept(CallbackOwner.global)
    }

  /**
   * Returns map options initialized for this C API version.
   *
   * See `mln_map_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun mapOptionsDefault(): MapOptions =
    nativeCall(null, null, "mln_map_options_default") {
      val out = sized(40, 8)
      C.mln_map_options_default(out)
      readMapOptions(out)
    }

  /**
   * Returns empty tile tuning options initialized for this C API version.
   *
   * See `mln_map_tile_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun mapTileOptionsDefault(): MapTileOptions =
    nativeCall(null, null, "mln_map_tile_options_default") {
      val out = sized(56, 8)
      C.mln_map_tile_options_default(out)
      readMapTileOptions(out)
    }

  /**
   * Returns empty viewport options initialized for this C API version.
   *
   * See `mln_map_viewport_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun mapViewportOptionsDefault(): MapViewportOptions =
    nativeCall(null, null, "mln_map_viewport_options_default") {
      val out = sized(56, 8)
      C.mln_map_viewport_options_default(out)
      readMapViewportOptions(out)
    }

  /**
   * Returns Metal borrowed-texture descriptor defaults for this C API version.
   *
   * See `mln_metal_borrowed_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun metalBorrowedTextureDescriptorDefault(): MetalBorrowedTextureDescriptor =
    nativeCall(null, null, "mln_metal_borrowed_texture_descriptor_default") {
      val out = sized(48, 8)
      C.mln_metal_borrowed_texture_descriptor_default(out)
      readMetalBorrowedTextureDescriptor(out)
    }

  /**
   * Returns Metal owned-texture descriptor defaults for this C API version.
   *
   * See `mln_metal_owned_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun metalOwnedTextureDescriptorDefault(): MetalOwnedTextureDescriptor =
    nativeCall(null, null, "mln_metal_owned_texture_descriptor_default") {
      val out = sized(w(40, 48), 8)
      C.mln_metal_owned_texture_descriptor_default(out)
      readMetalOwnedTextureDescriptor(out)
    }

  /**
   * Returns Metal surface descriptor defaults for this C API version.
   *
   * See `mln_metal_surface_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun metalSurfaceDescriptorDefault(): MetalSurfaceDescriptor =
    nativeCall(null, null, "mln_metal_surface_descriptor_default") {
      val out = sized(w(48, 56), 8)
      C.mln_metal_surface_descriptor_default(out)
      readMetalSurfaceDescriptor(out)
    }

  /**
   * Reads MapLibre Native's process-global network status.
   *
   * See `mln_network_status_get` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun networkStatusGet(): NetworkStatus =
    nativeCall(null, null, "mln_network_status_get") {
      val out = allocate(4, 4)
      check(C.mln_network_status_get(out, diagnostic))
      NetworkStatus(readU32(out))
    }

  /**
   * Sets MapLibre Native's process-global network status.
   *
   * See `mln_network_status_set` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun networkStatusSet(status: NetworkStatus): Unit =
    nativeCall(null, null, "mln_network_status_set") {
      check(C.mln_network_status_set(status.rawValue.toInt(), diagnostic))
    }

  /**
   * Returns OpenGL borrowed-texture descriptor defaults for this C API version.
   *
   * See `mln_opengl_borrowed_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun openglBorrowedTextureDescriptorDefault(): OpenglBorrowedTextureDescriptor =
    nativeCall(null, null, "mln_opengl_borrowed_texture_descriptor_default") {
      val out = sized(w(88, 112), 8)
      C.mln_opengl_borrowed_texture_descriptor_default(out)
      readOpenglBorrowedTextureDescriptor(out)
    }

  /**
   * Returns OpenGL owned-texture descriptor defaults for this C API version.
   *
   * See `mln_opengl_owned_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun openglOwnedTextureDescriptorDefault(): OpenglOwnedTextureDescriptor =
    nativeCall(null, null, "mln_opengl_owned_texture_descriptor_default") {
      val out = sized(w(72, 96), 8)
      C.mln_opengl_owned_texture_descriptor_default(out)
      readOpenglOwnedTextureDescriptor(out)
    }

  /**
   * Returns OpenGL context providers supported by this build.
   *
   * See `mln_opengl_supported_context_provider_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
   */
  public fun openglSupportedContextProviderMask(): OpenglContextProviderFlag =
    nativeCall(null, null, "mln_opengl_supported_context_provider_mask") {
      val raw = C.mln_opengl_supported_context_provider_mask()
      OpenglContextProviderFlag(raw.toUInt())
    }

  /**
   * Returns OpenGL surface descriptor defaults for this C API version.
   *
   * See `mln_opengl_surface_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun openglSurfaceDescriptorDefault(): OpenglSurfaceDescriptor =
    nativeCall(null, null, "mln_opengl_surface_descriptor_default") {
      val out = sized(w(72, 104), 8)
      C.mln_opengl_surface_descriptor_default(out)
      readOpenglSurfaceDescriptor(out)
    }

  /**
   * Returns the process-wide `mln_plugin_register_v1` entry point; never null.
   *
   * See `mln_plugin_get_register_function_v1` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
   */
  public fun pluginGetRegisterFunctionV1(): NativePointer =
    nativeCall(null, null, "mln_plugin_get_register_function_v1") {
      val raw = C.mln_plugin_get_register_function_v1()
      NativePointer.ofAddress(raw)
    }

  /**
   * Returns a default premultiplied RGBA8 image descriptor.
   *
   * See `mln_premultiplied_rgba8_image_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun premultipliedRgba8ImageDefault(): PremultipliedRgba8Image =
    nativeCall(null, null, "mln_premultiplied_rgba8_image_default") {
      val out = sized(w(24, 32), w(4, 8))
      C.mln_premultiplied_rgba8_image_default(out)
      readPremultipliedRgba8Image(out)
    }

  /**
   * Converts a geographic coordinate to spherical Mercator projected meters.
   *
   * See `mln_projected_meters_for_lat_lng` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters =
    nativeCall(null, null, "mln_projected_meters_for_lat_lng") {
      val out = allocate(16, 8)
      check(C.mln_projected_meters_for_lat_lng(writeLatLng(coordinate), out, diagnostic))
      readProjectedMeters(out)
    }

  /**
   * Returns empty axonometric rendering options initialized for this C API version.
   *
   * See `mln_projection_mode_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun projectionModeDefault(): ProjectionMode =
    nativeCall(null, null, "mln_projection_mode_default") {
      val out = sized(32, 8)
      C.mln_projection_mode_default(out)
      readProjectionMode(out)
    }

  /**
   * Returns default caller-graphics-thread attachment policy with no wakes and a one-slot texture
   * ring.
   *
   * See `mln_render_session_attach_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
   */
  public fun renderSessionAttachOptionsDefault(): RenderSessionAttachOptions =
    nativeCall(null, null, "mln_render_session_attach_options_default") {
      val out = sized(w(68, 120), w(4, 8))
      C.mln_render_session_attach_options_default(out)
      readRenderSessionAttachOptions(out)
    }

  /**
   * Computes the physical device-pixel size of a logical render target extent.
   *
   * See `mln_render_target_extent_physical_size` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
   */
  public fun renderTargetExtentPhysicalSize(
    extent: RenderTargetExtent
  ): RenderTargetExtentPhysicalSizeResult =
    nativeCall(null, null, "mln_render_target_extent_physical_size") {
      val out0 = allocate(4, 4)
      val out1 = allocate(4, 4)
      check(
        C.mln_render_target_extent_physical_size(
          writeRenderTargetExtent(extent),
          out0,
          out1,
          diagnostic,
        )
      )
      RenderTargetExtentPhysicalSizeResult(width = readU32(out0), height = readU32(out1))
    }

  /**
   * Returns default rendered feature query options.
   *
   * See `mln_rendered_feature_query_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun renderedFeatureQueryOptionsDefault(): RenderedFeatureQueryOptions =
    nativeCall(null, null, "mln_rendered_feature_query_options_default") {
      val out = sized(w(24, 40), w(4, 8))
      C.mln_rendered_feature_query_options_default(out)
      readRenderedFeatureQueryOptions(out)
    }

  /**
   * Returns a rendered box query geometry descriptor.
   *
   * See `mln_rendered_query_geometry_box` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun renderedQueryGeometryBox(box: ScreenBox): RenderedQueryGeometry =
    nativeCall(null, null, "mln_rendered_query_geometry_box") {
      val out = sized(40, 8)
      C.mln_rendered_query_geometry_box(writeScreenBox(box), out)
      readRenderedQueryGeometry(out)
    }

  /**
   * Returns a rendered line-string query geometry descriptor.
   *
   * See `mln_rendered_query_geometry_line_string` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun renderedQueryGeometryLineString(points: List<ScreenPoint>): RenderedQueryGeometry =
    nativeCall(null, null, "mln_rendered_query_geometry_line_string") {
      val out = sized(40, 8)
      C.mln_rendered_query_geometry_line_string(
        array(points, 16, 8) { at, item -> putScreenPoint(at, item) },
        points.size.toLong(),
        out,
      )
      readRenderedQueryGeometry(out)
    }

  /**
   * Returns a rendered point query geometry descriptor.
   *
   * See `mln_rendered_query_geometry_point` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun renderedQueryGeometryPoint(point: ScreenPoint): RenderedQueryGeometry =
    nativeCall(null, null, "mln_rendered_query_geometry_point") {
      val out = sized(40, 8)
      C.mln_rendered_query_geometry_point(writeScreenPoint(point), out)
      readRenderedQueryGeometry(out)
    }

  /**
   * Copies a replacement URL into C API-managed storage for the current callback.
   *
   * See `mln_resource_transform_response_set_url` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun resourceTransformResponseSetUrl(
    response: ResourceTransformResponse,
    url: String,
  ): Unit =
    nativeRespond(
      response.bindingScope,
      response.bindingAddress,
      "mln_resource_transform_response_set_url",
    ) {
      val urlUtf8 = url.encodeToByteArray()
      check(
        C.mln_resource_transform_response_set_url(
          handle,
          bytes(urlUtf8),
          urlUtf8.size.toLong(),
          diagnostic,
        )
      )
    }

  /**
   * Creates a runtime with a new core-owned worker.
   *
   * See `mln_runtime_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun runtimeCreate(options: RuntimeOptions): RuntimeHandle =
    nativeCall(null, null, "mln_runtime_create") {
      val out = allocate(8, 8)
      check(C.mln_runtime_create(writeRuntimeOptions(options), out, diagnostic))
      adoptRegistered(
        out,
        GeneratedOwnerDisposal::runtime,
        { RuntimeHandle(it) },
        { it.bindingCallbacks },
        { it.dispose() },
      )
    }

  /**
   * Returns runtime options initialized for this C API version.
   *
   * See `mln_runtime_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun runtimeOptionsDefault(): RuntimeOptions =
    nativeCall(null, null, "mln_runtime_options_default") {
      val out = sized(w(40, 64), 8)
      C.mln_runtime_options_default(out)
      readRuntimeOptions(out)
    }

  /**
   * Returns default source feature query options.
   *
   * See `mln_source_feature_query_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun sourceFeatureQueryOptionsDefault(): SourceFeatureQueryOptions =
    nativeCall(null, null, "mln_source_feature_query_options_default") {
      val out = sized(w(24, 40), w(4, 8))
      C.mln_source_feature_query_options_default(out)
      readSourceFeatureQueryOptions(out)
    }

  /**
   * Returns default runtime style image metadata.
   *
   * See `mln_style_image_info_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun styleImageInfoDefault(): StyleImageInfo =
    nativeCall(null, null, "mln_style_image_info_default") {
      val out = sized(w(64, 80), w(4, 8))
      C.mln_style_image_info_default(out)
      readStyleImageInfo(out)
    }

  /**
   * Returns default runtime style image options.
   *
   * See `mln_style_image_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun styleImageOptionsDefault(): StyleImageOptions =
    nativeCall(null, null, "mln_style_image_options_default") {
      val out = sized(w(56, 72), w(4, 8))
      C.mln_style_image_options_default(out)
      readStyleImageOptions(out)
    }

  /**
   * Returns default tile source options.
   *
   * See `mln_style_tile_source_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun styleTileSourceOptionsDefault(): StyleTileSourceOptions =
    nativeCall(null, null, "mln_style_tile_source_options_default") {
      val out = sized(w(88, 96), 8)
      C.mln_style_tile_source_options_default(out)
      readStyleTileSourceOptions(out)
    }

  /**
   * Returns default global style transition options.
   *
   * See `mln_style_transition_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun styleTransitionOptionsDefault(): StyleTransitionOptions =
    nativeCall(null, null, "mln_style_transition_options_default") {
      val out = sized(32, 8)
      C.mln_style_transition_options_default(out)
      readStyleTransitionOptions(out)
    }

  /**
   * Reports the render backends available in this native library build.
   *
   * See `mln_supported_render_backend_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
   */
  public fun supportedRenderBackendMask(): RenderBackendFlag =
    nativeCall(null, null, "mln_supported_render_backend_mask") {
      val raw = C.mln_supported_render_backend_mask()
      RenderBackendFlag(raw.toUInt())
    }

  /**
   * Returns texture image info defaults for this C API version.
   *
   * See `mln_texture_image_info_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun textureImageInfoDefault(): TextureImageInfo =
    nativeCall(null, null, "mln_texture_image_info_default") {
      val out = sized(w(20, 24), w(4, 8))
      C.mln_texture_image_info_default(out)
      readTextureImageInfo(out)
    }

  /**
   * Returns Vulkan borrowed-texture descriptor defaults for this C API version.
   *
   * See `mln_vulkan_borrowed_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun vulkanBorrowedTextureDescriptorDefault(): VulkanBorrowedTextureDescriptor =
    nativeCall(null, null, "mln_vulkan_borrowed_texture_descriptor_default") {
      val out = sized(w(104, 136), 8)
      C.mln_vulkan_borrowed_texture_descriptor_default(out)
      readVulkanBorrowedTextureDescriptor(out)
    }

  /**
   * Returns Vulkan owned-texture descriptor defaults for this C API version.
   *
   * See `mln_vulkan_owned_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun vulkanOwnedTextureDescriptorDefault(): VulkanOwnedTextureDescriptor =
    nativeCall(null, null, "mln_vulkan_owned_texture_descriptor_default") {
      val out = sized(w(64, 96), 8)
      C.mln_vulkan_owned_texture_descriptor_default(out)
      readVulkanOwnedTextureDescriptor(out)
    }

  /**
   * Returns Vulkan surface descriptor defaults for this C API version.
   *
   * See `mln_vulkan_surface_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun vulkanSurfaceDescriptorDefault(): VulkanSurfaceDescriptor =
    nativeCall(null, null, "mln_vulkan_surface_descriptor_default") {
      val out = sized(w(72, 104), 8)
      C.mln_vulkan_surface_descriptor_default(out)
      readVulkanSurfaceDescriptor(out)
    }

  /**
   * Returns WebGPU borrowed-texture descriptor defaults for this C API version.
   *
   * See `mln_webgpu_borrowed_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun webgpuBorrowedTextureDescriptorDefault(): WebgpuBorrowedTextureDescriptor =
    nativeCall(null, null, "mln_webgpu_borrowed_texture_descriptor_default") {
      val out = sized(w(72, 96), 8)
      C.mln_webgpu_borrowed_texture_descriptor_default(out)
      readWebgpuBorrowedTextureDescriptor(out)
    }

  /**
   * Returns WebGPU owned-texture descriptor defaults for this C API version.
   *
   * See `mln_webgpu_owned_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun webgpuOwnedTextureDescriptorDefault(): WebgpuOwnedTextureDescriptor =
    nativeCall(null, null, "mln_webgpu_owned_texture_descriptor_default") {
      val out = sized(w(48, 64), 8)
      C.mln_webgpu_owned_texture_descriptor_default(out)
      readWebgpuOwnedTextureDescriptor(out)
    }

  /**
   * Returns WebGPU surface descriptor defaults for this C API version.
   *
   * See `mln_webgpu_surface_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
   */
  public fun webgpuSurfaceDescriptorDefault(): WebgpuSurfaceDescriptor =
    nativeCall(null, null, "mln_webgpu_surface_descriptor_default") {
      val out = sized(w(56, 80), 8)
      C.mln_webgpu_surface_descriptor_default(out)
      readWebgpuSurfaceDescriptor(out)
    }
}
