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
  public fun androidInit(
    jniEnv: NativePointer,
    jniClass: NativePointer,
    context: NativePointer,
  ): Unit =
    nativeCall(null, null, "mln_android_init") {
      check(C.mln_android_init(jniEnv.address, jniClass.address, context.address, diagnostic))
    }

  public fun animationOptionsDefault(): AnimationOptions =
    nativeCall(null, null, "mln_animation_options_default") {
      val out = sized(72, 8)
      C.mln_animation_options_default(out)
      readAnimationOptions(out)
    }

  public fun boundOptionsDefault(): BoundOptions =
    nativeCall(null, null, "mln_bound_options_default") {
      val out = sized(72, 8)
      C.mln_bound_options_default(out)
      readBoundOptions(out)
    }

  public fun cVersion(): UInt =
    nativeCall(null, null, "mln_c_version") {
      val raw = C.mln_c_version()
      raw.toUInt()
    }

  public fun cameraDeltaDefault(): CameraDelta =
    nativeCall(null, null, "mln_camera_delta_default") {
      val out = sized(128, 8)
      C.mln_camera_delta_default(out)
      readCameraDelta(out)
    }

  public fun cameraFitOptionsDefault(): CameraFitOptions =
    nativeCall(null, null, "mln_camera_fit_options_default") {
      val out = sized(56, 8)
      C.mln_camera_fit_options_default(out)
      readCameraFitOptions(out)
    }

  public fun cameraOptionsDefault(): CameraOptions =
    nativeCall(null, null, "mln_camera_options_default") {
      val out = sized(120, 8)
      C.mln_camera_options_default(out)
      readCameraOptions(out)
    }

  public fun cameraUpdateDefault(): CameraUpdate =
    nativeCall(null, null, "mln_camera_update_default") {
      val out = sized(208, 8)
      C.mln_camera_update_default(out)
      readCameraUpdate(out)
    }

  public fun customGeometrySourceOptionsDefault(): CustomGeometrySourceOptions =
    nativeCall(null, null, "mln_custom_geometry_source_options_default") {
      val out = sized(w(64, 80), 8)
      C.mln_custom_geometry_source_options_default(out)
      readCustomGeometrySourceOptions(out)
    }

  public fun customMvtVectorSourceOptionsDefault(): CustomMvtVectorSourceOptions =
    nativeCall(null, null, "mln_custom_mvt_vector_source_options_default") {
      val out = sized(w(48, 56), 8)
      C.mln_custom_mvt_vector_source_options_default(out)
      readCustomMvtVectorSourceOptions(out)
    }

  public fun frameDemandDefault(): FrameDemand =
    nativeCall(null, null, "mln_frame_demand_default") {
      val out = sized(32, 8)
      C.mln_frame_demand_default(out)
      readFrameDemand(out)
    }

  public fun freeCameraOptionsDefault(): FreeCameraOptions =
    nativeCall(null, null, "mln_free_camera_options_default") {
      val out = sized(64, 8)
      C.mln_free_camera_options_default(out)
      readFreeCameraOptions(out)
    }

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

  public fun geojsonSourceOptionsDefault(): GeojsonSourceOptions =
    nativeCall(null, null, "mln_geojson_source_options_default") {
      val out = sized(w(72, 80), 8)
      C.mln_geojson_source_options_default(out)
      readGeojsonSourceOptions(out)
    }

  public fun gpuSyncDefault(): GpuSync =
    nativeCall(null, null, "mln_gpu_sync_default") {
      val out = sized(24, 8)
      C.mln_gpu_sync_default(out)
      readGpuSync(out)
    }

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

  public fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng =
    nativeCall(null, null, "mln_lat_lng_for_projected_meters") {
      val out = allocate(16, 8)
      check(C.mln_lat_lng_for_projected_meters(writeProjectedMeters(meters), out, diagnostic))
      readLatLng(out)
    }

  public fun logClearCallback(): Unit =
    nativeCall(null, null, "mln_log_clear_callback") { check(C.mln_log_clear_callback(diagnostic)) }

  public fun logSetAsyncSeverityMask(mask: LogSeverityMask): Unit =
    nativeCall(null, null, "mln_log_set_async_severity_mask") {
      check(C.mln_log_set_async_severity_mask(mask.rawValue.toInt(), diagnostic))
    }

  public fun logSetCallback(callback: LogCallback): Unit =
    nativeCall(null, null, "mln_log_set_callback") {
      val token = registrations.register(GeneratedLogCallbackRegistration(callback))
      check(
        C.mln_log_set_callback(UpcallStubs.logCallback, token, UpcallStubs.releaseRoot, diagnostic)
      )
      accept(CallbackOwner.global)
    }

  public fun mapOptionsDefault(): MapOptions =
    nativeCall(null, null, "mln_map_options_default") {
      val out = sized(40, 8)
      C.mln_map_options_default(out)
      readMapOptions(out)
    }

  public fun mapTileOptionsDefault(): MapTileOptions =
    nativeCall(null, null, "mln_map_tile_options_default") {
      val out = sized(56, 8)
      C.mln_map_tile_options_default(out)
      readMapTileOptions(out)
    }

  public fun mapViewportOptionsDefault(): MapViewportOptions =
    nativeCall(null, null, "mln_map_viewport_options_default") {
      val out = sized(56, 8)
      C.mln_map_viewport_options_default(out)
      readMapViewportOptions(out)
    }

  public fun metalBorrowedTextureDescriptorDefault(): MetalBorrowedTextureDescriptor =
    nativeCall(null, null, "mln_metal_borrowed_texture_descriptor_default") {
      val out = sized(48, 8)
      C.mln_metal_borrowed_texture_descriptor_default(out)
      readMetalBorrowedTextureDescriptor(out)
    }

  public fun metalOwnedTextureDescriptorDefault(): MetalOwnedTextureDescriptor =
    nativeCall(null, null, "mln_metal_owned_texture_descriptor_default") {
      val out = sized(w(40, 48), 8)
      C.mln_metal_owned_texture_descriptor_default(out)
      readMetalOwnedTextureDescriptor(out)
    }

  public fun metalSurfaceDescriptorDefault(): MetalSurfaceDescriptor =
    nativeCall(null, null, "mln_metal_surface_descriptor_default") {
      val out = sized(w(48, 56), 8)
      C.mln_metal_surface_descriptor_default(out)
      readMetalSurfaceDescriptor(out)
    }

  public fun networkStatusGet(): NetworkStatus =
    nativeCall(null, null, "mln_network_status_get") {
      val out = allocate(4, 4)
      check(C.mln_network_status_get(out, diagnostic))
      NetworkStatus(readU32(out))
    }

  public fun networkStatusSet(status: NetworkStatus): Unit =
    nativeCall(null, null, "mln_network_status_set") {
      check(C.mln_network_status_set(status.rawValue.toInt(), diagnostic))
    }

  public fun openglBorrowedTextureDescriptorDefault(): OpenglBorrowedTextureDescriptor =
    nativeCall(null, null, "mln_opengl_borrowed_texture_descriptor_default") {
      val out = sized(w(88, 112), 8)
      C.mln_opengl_borrowed_texture_descriptor_default(out)
      readOpenglBorrowedTextureDescriptor(out)
    }

  public fun openglOwnedTextureDescriptorDefault(): OpenglOwnedTextureDescriptor =
    nativeCall(null, null, "mln_opengl_owned_texture_descriptor_default") {
      val out = sized(w(72, 96), 8)
      C.mln_opengl_owned_texture_descriptor_default(out)
      readOpenglOwnedTextureDescriptor(out)
    }

  public fun openglSupportedContextProviderMask(): OpenglContextProviderFlag =
    nativeCall(null, null, "mln_opengl_supported_context_provider_mask") {
      val raw = C.mln_opengl_supported_context_provider_mask()
      OpenglContextProviderFlag(raw.toUInt())
    }

  public fun openglSurfaceDescriptorDefault(): OpenglSurfaceDescriptor =
    nativeCall(null, null, "mln_opengl_surface_descriptor_default") {
      val out = sized(w(72, 104), 8)
      C.mln_opengl_surface_descriptor_default(out)
      readOpenglSurfaceDescriptor(out)
    }

  public fun pluginGetRegisterFunctionV1(): NativePointer =
    nativeCall(null, null, "mln_plugin_get_register_function_v1") {
      val raw = C.mln_plugin_get_register_function_v1()
      NativePointer.ofAddress(raw)
    }

  public fun premultipliedRgba8ImageDefault(): PremultipliedRgba8Image =
    nativeCall(null, null, "mln_premultiplied_rgba8_image_default") {
      val out = sized(w(24, 32), w(4, 8))
      C.mln_premultiplied_rgba8_image_default(out)
      readPremultipliedRgba8Image(out)
    }

  public fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters =
    nativeCall(null, null, "mln_projected_meters_for_lat_lng") {
      val out = allocate(16, 8)
      check(C.mln_projected_meters_for_lat_lng(writeLatLng(coordinate), out, diagnostic))
      readProjectedMeters(out)
    }

  public fun projectionModeDefault(): ProjectionMode =
    nativeCall(null, null, "mln_projection_mode_default") {
      val out = sized(32, 8)
      C.mln_projection_mode_default(out)
      readProjectionMode(out)
    }

  public fun renderSessionAttachOptionsDefault(): RenderSessionAttachOptions =
    nativeCall(null, null, "mln_render_session_attach_options_default") {
      val out = sized(w(48, 80), w(4, 8))
      C.mln_render_session_attach_options_default(out)
      readRenderSessionAttachOptions(out)
    }

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

  public fun renderedFeatureQueryOptionsDefault(): RenderedFeatureQueryOptions =
    nativeCall(null, null, "mln_rendered_feature_query_options_default") {
      val out = sized(w(20, 32), w(4, 8))
      C.mln_rendered_feature_query_options_default(out)
      readRenderedFeatureQueryOptions(out)
    }

  public fun renderedQueryGeometryBox(box: ScreenBox): RenderedQueryGeometry =
    nativeCall(null, null, "mln_rendered_query_geometry_box") {
      val out = sized(40, 8)
      C.mln_rendered_query_geometry_box(writeScreenBox(box), out)
      readRenderedQueryGeometry(out)
    }

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

  public fun renderedQueryGeometryPoint(point: ScreenPoint): RenderedQueryGeometry =
    nativeCall(null, null, "mln_rendered_query_geometry_point") {
      val out = sized(40, 8)
      C.mln_rendered_query_geometry_point(writeScreenPoint(point), out)
      readRenderedQueryGeometry(out)
    }

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

  public fun runtimeOptionsDefault(): RuntimeOptions =
    nativeCall(null, null, "mln_runtime_options_default") {
      val out = sized(w(40, 64), 8)
      C.mln_runtime_options_default(out)
      readRuntimeOptions(out)
    }

  public fun sourceFeatureQueryOptionsDefault(): SourceFeatureQueryOptions =
    nativeCall(null, null, "mln_source_feature_query_options_default") {
      val out = sized(w(20, 32), w(4, 8))
      C.mln_source_feature_query_options_default(out)
      readSourceFeatureQueryOptions(out)
    }

  public fun styleImageInfoDefault(): StyleImageInfo =
    nativeCall(null, null, "mln_style_image_info_default") {
      val out = sized(w(60, 72), w(4, 8))
      C.mln_style_image_info_default(out)
      readStyleImageInfo(out)
    }

  public fun styleImageOptionsDefault(): StyleImageOptions =
    nativeCall(null, null, "mln_style_image_options_default") {
      val out = sized(w(56, 72), w(4, 8))
      C.mln_style_image_options_default(out)
      readStyleImageOptions(out)
    }

  public fun styleTileSourceOptionsDefault(): StyleTileSourceOptions =
    nativeCall(null, null, "mln_style_tile_source_options_default") {
      val out = sized(w(88, 96), 8)
      C.mln_style_tile_source_options_default(out)
      readStyleTileSourceOptions(out)
    }

  public fun styleTransitionOptionsDefault(): StyleTransitionOptions =
    nativeCall(null, null, "mln_style_transition_options_default") {
      val out = sized(32, 8)
      C.mln_style_transition_options_default(out)
      readStyleTransitionOptions(out)
    }

  public fun supportedRenderBackendMask(): RenderBackendFlag =
    nativeCall(null, null, "mln_supported_render_backend_mask") {
      val raw = C.mln_supported_render_backend_mask()
      RenderBackendFlag(raw.toUInt())
    }

  public fun textureImageInfoDefault(): TextureImageInfo =
    nativeCall(null, null, "mln_texture_image_info_default") {
      val out = sized(w(20, 24), w(4, 8))
      C.mln_texture_image_info_default(out)
      readTextureImageInfo(out)
    }

  public fun vulkanBorrowedTextureDescriptorDefault(): VulkanBorrowedTextureDescriptor =
    nativeCall(null, null, "mln_vulkan_borrowed_texture_descriptor_default") {
      val out = sized(w(104, 136), 8)
      C.mln_vulkan_borrowed_texture_descriptor_default(out)
      readVulkanBorrowedTextureDescriptor(out)
    }

  public fun vulkanOwnedTextureDescriptorDefault(): VulkanOwnedTextureDescriptor =
    nativeCall(null, null, "mln_vulkan_owned_texture_descriptor_default") {
      val out = sized(w(64, 96), 8)
      C.mln_vulkan_owned_texture_descriptor_default(out)
      readVulkanOwnedTextureDescriptor(out)
    }

  public fun vulkanSurfaceDescriptorDefault(): VulkanSurfaceDescriptor =
    nativeCall(null, null, "mln_vulkan_surface_descriptor_default") {
      val out = sized(w(72, 104), 8)
      C.mln_vulkan_surface_descriptor_default(out)
      readVulkanSurfaceDescriptor(out)
    }

  public fun webgpuBorrowedTextureDescriptorDefault(): WebgpuBorrowedTextureDescriptor =
    nativeCall(null, null, "mln_webgpu_borrowed_texture_descriptor_default") {
      val out = sized(w(72, 96), 8)
      C.mln_webgpu_borrowed_texture_descriptor_default(out)
      readWebgpuBorrowedTextureDescriptor(out)
    }

  public fun webgpuOwnedTextureDescriptorDefault(): WebgpuOwnedTextureDescriptor =
    nativeCall(null, null, "mln_webgpu_owned_texture_descriptor_default") {
      val out = sized(w(48, 64), 8)
      C.mln_webgpu_owned_texture_descriptor_default(out)
      readWebgpuOwnedTextureDescriptor(out)
    }

  public fun webgpuSurfaceDescriptorDefault(): WebgpuSurfaceDescriptor =
    nativeCall(null, null, "mln_webgpu_surface_descriptor_default") {
      val out = sized(w(56, 80), 8)
      C.mln_webgpu_surface_descriptor_default(out)
      readWebgpuSurfaceDescriptor(out)
    }
}
