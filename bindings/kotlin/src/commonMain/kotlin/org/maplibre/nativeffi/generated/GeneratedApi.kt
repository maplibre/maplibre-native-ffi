// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public object GeneratedApi {
  /**
   * Returns empty animation options initialized for this C API version.
   *
   * See `mln_animation_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun animationOptionsDefault(): AnimationOptions =
    nativeCall(null, null) {
      val out = allocate(w(96, 104), 8)
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
    nativeCall(null, null) {
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
    nativeCall(null, null) {
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
    nativeCall(null, null) {
      val out = sized(w(168, 176), 8)
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
    nativeCall(null, null) {
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
    nativeCall(null, null) {
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
    nativeCall(null, null) {
      val out = sized(w(232, 240), 8)
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
    nativeCall(null, null) {
      val out = sized(w(72, 88), 8)
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
    nativeCall(null, null) {
      val out = sized(w(56, 64), 8)
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
    nativeCall(null, null) {
      val out = sized(16, 8)
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
    nativeCall(null, null) {
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
    data: String,
    options: GeojsonSourceOptions? = null,
  ): GeojsonSourceDataHandle =
    nativeCall(null, null) {
      val out = allocate(8, 8)
      check(
        C.mln_geojson_source_data_create(
          view(data),
          options?.let { writeGeojsonSourceOptions(it) } ?: 0L,
          out,
          diagnostic,
        ),
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
    nativeCall(null, null) {
      val out = sized(w(64, 72), 8)
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
    nativeCall(null, null) {
      val out = sized(16, 8)
      C.mln_gpu_sync_default(out)
      readGpuSync(out)
    }

  /**
   * Returns an exact-match header rule for every resource kind, with no headers.
   *
   * See `mln_http_header_rule_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun httpHeaderRuleDefault(): HttpHeaderRule =
    nativeCall(null, null) {
      val out = allocate(w(24, 40), w(4, 8))
      C.mln_http_header_rule_default(out)
      readHttpHeaderRule(out)
    }

  /**
   * Converts spherical Mercator projected meters to a geographic coordinate.
   *
   * See `mln_lat_lng_for_projected_meters` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng =
    nativeCall(null, null) {
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
    nativeCall(null, null) { check(C.mln_log_clear_callback(diagnostic)) }

  /**
   * Controls which log severities MapLibre Native may dispatch asynchronously.
   *
   * See `mln_log_set_async_severity_mask` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
   */
  public fun logSetAsyncSeverityMask(mask: LogSeverityFlag): Unit =
    nativeCall(null, null) {
      check(C.mln_log_set_async_severity_mask(mask.rawValue.toInt(), diagnostic))
    }

  /**
   * Returns a 256 by 256 logical extent with one device pixel per UI pixel.
   *
   * See `mln_logical_extent_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun logicalExtentDefault(): LogicalExtent =
    nativeCall(null, null) {
      val out = allocate(16, 8)
      C.mln_logical_extent_default(out)
      readLogicalExtent(out)
    }

  /**
   * Computes the physical device-pixel size of a logical extent.
   *
   * See `mln_logical_extent_physical_size` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
   */
  public fun logicalExtentPhysicalSize(extent: LogicalExtent): LogicalExtentPhysicalSizeResult =
    nativeCall(null, null) {
      val out0 = allocate(4, 4)
      val out1 = allocate(4, 4)
      check(C.mln_logical_extent_physical_size(writeLogicalExtent(extent), out0, out1, diagnostic))
      LogicalExtentPhysicalSizeResult(width = readU32(out0), height = readU32(out1))
    }

  /**
   * Returns map options initialized for this C API version.
   *
   * See `mln_map_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
   */
  public fun mapOptionsDefault(): MapOptions =
    nativeCall(null, null) {
      val out = sized(40, 8)
      C.mln_map_options_default(out)
      readMapOptions(out)
    }

  /**
   * Creates a standalone projection of a view, such as the view of a rendered frame or of a map
   * snapshot.
   *
   * See `mln_map_projection_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
   */
  public fun mapProjectionCreate(view: MapView): MapProjectionHandle =
    nativeCall(null, null) {
      val out = allocate(8, 8)
      check(C.mln_map_projection_create(writeMapView(view), out, diagnostic))
      adopt(out, GeneratedOwnerDisposal::mapProjection) { MapProjectionHandle(it) }
    }

  /**
   * Returns empty tile tuning options initialized for this C API version.
   *
   * See `mln_map_tile_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
   */
  public fun mapTileOptionsDefault(): MapTileOptions =
    nativeCall(null, null) {
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
    nativeCall(null, null) {
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
    nativeCall(null, null) {
      val out = sized(w(40, 48), 8)
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
    nativeCall(null, null) {
      val out = sized(40, 8)
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
    nativeCall(null, null) {
      val out = sized(w(32, 40), 8)
      C.mln_metal_surface_descriptor_default(out)
      readMetalSurfaceDescriptor(out)
    }

  /**
   * Reads MapLibre Native's process-global network status.
   *
   * See `mln_network_get_status` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun networkGetStatus(): NetworkStatus =
    nativeCall(null, null) {
      val out = allocate(4, 4)
      check(C.mln_network_get_status(out, diagnostic))
      NetworkStatus(readU32(out))
    }

  /**
   * Sets MapLibre Native's process-global network status.
   *
   * See `mln_network_set_status` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun networkSetStatus(status: NetworkStatus): Unit =
    nativeCall(null, null) { check(C.mln_network_set_status(status.rawValue.toInt(), diagnostic)) }

  /**
   * Returns OpenGL borrowed-texture descriptor defaults for this C API version.
   *
   * See `mln_opengl_borrowed_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun openglBorrowedTextureDescriptorDefault(): OpenglBorrowedTextureDescriptor =
    nativeCall(null, null) {
      val out = sized(w(72, 104), 8)
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
    nativeCall(null, null) {
      val out = sized(w(56, 72), 8)
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
    nativeCall(null, null) {
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
    nativeCall(null, null) {
      val out = sized(w(56, 80), 8)
      C.mln_opengl_surface_descriptor_default(out)
      readOpenglSurfaceDescriptor(out)
    }

  /**
   * Returns a default premultiplied RGBA8 image descriptor.
   *
   * See `mln_premultiplied_rgba8_image_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun premultipliedRgba8ImageDefault(): PremultipliedRgba8Image =
    nativeCall(null, null) {
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
    nativeCall(null, null) {
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
    nativeCall(null, null) {
      val out = sized(32, 8)
      C.mln_projection_mode_default(out)
      readProjectionMode(out)
    }

  /**
   * Returns attachment options initialized for this C API version, with each field at its
   * documented default.
   *
   * See `mln_render_session_attach_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
   */
  public fun renderSessionAttachOptionsDefault(): RenderSessionAttachOptions =
    nativeCall(null, null) {
      val out = sized(w(56, 96), w(4, 8))
      C.mln_render_session_attach_options_default(out)
      readRenderSessionAttachOptions(out)
    }

  /**
   * Returns default rendered feature query options.
   *
   * See `mln_rendered_feature_query_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
   */
  public fun renderedFeatureQueryOptionsDefault(): RenderedFeatureQueryOptions =
    nativeCall(null, null) {
      val out = sized(w(24, 40), w(4, 8))
      C.mln_rendered_feature_query_options_default(out)
      readRenderedFeatureQueryOptions(out)
    }

  /**
   * Returns an exact-match route for every resource kind.
   *
   * See `mln_resource_route_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun resourceRouteDefault(): ResourceRoute =
    nativeCall(null, null) {
      val out = allocate(w(16, 24), w(4, 8))
      C.mln_resource_route_default(out)
      readResourceRoute(out)
    }

  /**
   * Creates a runtime.
   *
   * Suspends until the call's thread starts. On the JVM, Android, and Kotlin/Native the call waits
   * on the calling thread, which it blocks until then. On Kotlin/Wasm it runs on a thread of its
   * own, so the page can call it. Cancellation does not interrupt the wait: a caller cancelled
   * meanwhile receives the cancellation, and the binding disposes the result.
   *
   * See `mln_runtime_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public suspend fun runtimeCreate(options: RuntimeOptions): RuntimeHandle =
    nativeThreadStart({ it.dispose() }) {
      val out = allocate(8, 8)
      threadStart { diagnostic ->
        C.mln_runtime_create(writeRuntimeOptions(options), out, diagnostic)
      }
      adopt(out, GeneratedOwnerDisposal::runtime) { RuntimeHandle(it) }
    }

  /**
   * Returns runtime options initialized for this C API version.
   *
   * See `mln_runtime_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
   */
  public fun runtimeOptionsDefault(): RuntimeOptions =
    nativeCall(null, null) {
      val out = sized(w(20, 40), w(4, 8))
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
    nativeCall(null, null) {
      val out = sized(w(24, 40), w(4, 8))
      C.mln_source_feature_query_options_default(out)
      readSourceFeatureQueryOptions(out)
    }

  /**
   * Parses a style expression.
   *
   * See `mln_style_expression_create` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  public fun styleExpressionCreate(
    expression: String,
    options: StyleExpressionOptions? = null,
  ): StyleExpressionHandle =
    nativeCall(null, null) {
      val out = allocate(8, 8)
      check(
        C.mln_style_expression_create(
          view(expression),
          options?.let { writeStyleExpressionOptions(it) } ?: 0L,
          out,
          diagnostic,
        ),
      )
      adopt(out, GeneratedOwnerDisposal::styleExpression) { StyleExpressionHandle(it) }
    }

  /**
   * Returns default style expression inputs, which set no input.
   *
   * See `mln_style_expression_inputs_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  public fun styleExpressionInputsDefault(): StyleExpressionInputs =
    nativeCall(null, null) {
      val out = sized(w(80, 112), 8)
      C.mln_style_expression_inputs_default(out)
      readStyleExpressionInputs(out)
    }

  /**
   * Returns default style expression options.
   *
   * See `mln_style_expression_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
   */
  public fun styleExpressionOptionsDefault(): StyleExpressionOptions =
    nativeCall(null, null) {
      val out = sized(20, 4)
      C.mln_style_expression_options_default(out)
      readStyleExpressionOptions(out)
    }

  /**
   * Returns default runtime style image options.
   *
   * See `mln_style_image_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun styleImageOptionsDefault(): StyleImageOptions =
    nativeCall(null, null) {
      val out = sized(w(56, 72), w(4, 8))
      C.mln_style_image_options_default(out)
      readStyleImageOptions(out)
    }

  /**
   * Returns default global style transition options.
   *
   * See `mln_style_transition_options_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
   */
  public fun styleTransitionOptionsDefault(): StyleTransitionOptions =
    nativeCall(null, null) {
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
    nativeCall(null, null) {
      val raw = C.mln_supported_render_backend_mask()
      RenderBackendFlag(raw.toUInt())
    }

  /**
   * Returns Vulkan borrowed-texture descriptor defaults for this C API version.
   *
   * See `mln_vulkan_borrowed_texture_descriptor_default` in the
   * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
   */
  public fun vulkanBorrowedTextureDescriptorDefault(): VulkanBorrowedTextureDescriptor =
    nativeCall(null, null) {
      val out = sized(w(80, 120), 8)
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
    nativeCall(null, null) {
      val out = sized(w(56, 80), 8)
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
    nativeCall(null, null) {
      val out = sized(w(64, 88), 8)
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
    nativeCall(null, null) {
      val out = sized(w(56, 80), 8)
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
    nativeCall(null, null) {
      val out = sized(w(40, 48), 8)
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
    nativeCall(null, null) {
      val out = sized(w(48, 64), 8)
      C.mln_webgpu_surface_descriptor_default(out)
      readWebgpuSurfaceDescriptor(out)
    }
}
