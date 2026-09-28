// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.lifecycle.OwnerAdoption
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

@OptIn(ExperimentalForeignApi::class)
public actual object GeneratedApi {
  public actual fun androidInit(
    jniEnv: org.maplibre.nativeffi.render.NativePointer,
    jniClass: org.maplibre.nativeffi.render.NativePointer,
    context: org.maplibre.nativeffi.render.NativePointer,
  ): Unit {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_android_init")
    return memScoped {
      val arena = this
      BindingStatus.check(
        mln_android_init(
          (jniEnv.address).toCPointer<ByteVar>(),
          (jniClass.address).toCPointer<ByteVar>(),
          (context.address).toCPointer<ByteVar>(),
        )
      )
      Unit
    }
  }

  public actual fun animationOptionsDefault(): AnimationOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_animation_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_animation_options_default()
      nativeResult.useContents { GeneratedValues.readAnimationOptions(this) }
    }
  }

  public actual fun boundOptionsDefault(): BoundOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_bound_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_bound_options_default()
      nativeResult.useContents { GeneratedValues.readBoundOptions(this) }
    }
  }

  public actual fun cVersion(): UInt {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_c_version")
    return memScoped {
      val arena = this
      val nativeResult = mln_c_version()
      nativeResult
    }
  }

  public actual fun cameraDeltaDefault(): CameraDelta {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_delta_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_camera_delta_default()
      nativeResult.useContents { GeneratedValues.readCameraDelta(this) }
    }
  }

  public actual fun cameraFitOptionsDefault(): CameraFitOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_fit_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_camera_fit_options_default()
      nativeResult.useContents { GeneratedValues.readCameraFitOptions(this) }
    }
  }

  public actual fun cameraOptionsDefault(): CameraOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_camera_options_default()
      nativeResult.useContents { GeneratedValues.readCameraOptions(this) }
    }
  }

  public actual fun cameraUpdateDefault(): CameraUpdate {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_update_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_camera_update_default()
      nativeResult.useContents { GeneratedValues.readCameraUpdate(this) }
    }
  }

  public actual fun customGeometrySourceOptionsDefault(): CustomGeometrySourceOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_custom_geometry_source_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_custom_geometry_source_options_default()
      nativeResult.useContents { GeneratedCallbacks.readCustomGeometrySourceOptions(this) }
    }
  }

  public actual fun customMvtVectorSourceOptionsDefault(): CustomMvtVectorSourceOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_custom_mvt_vector_source_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_custom_mvt_vector_source_options_default()
      nativeResult.useContents { GeneratedCallbacks.readCustomMvtVectorSourceOptions(this) }
    }
  }

  public actual fun frameDemandDefault(): FrameDemand {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_frame_demand_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_frame_demand_default()
      nativeResult.useContents { GeneratedValues.readFrameDemand(this) }
    }
  }

  public actual fun freeCameraOptionsDefault(): FreeCameraOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_free_camera_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_free_camera_options_default()
      nativeResult.useContents { GeneratedValues.readFreeCameraOptions(this) }
    }
  }

  public actual fun geojsonSourceDataCreate(
    data: ByteArray,
    options: GeojsonSourceOptions?,
  ): org.maplibre.nativeffi.style.GeoJsonSourceDataHandle {
    org.maplibre.nativeffi.Maplibre.loadNativeLibrary()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_geojson_source_data_create",
    )
    return memScoped {
      val arena = this
      val output = arena.alloc<ULongVar>().also { it.value = 0uL }
      BindingStatus.check(
        mln_geojson_source_data_create(
          GeneratedValues.byteView(arena, data).pointed.readValue(),
          if (options == null) null
          else GeneratedValues.writeGeojsonSourceOptions(arena, options!!),
          output.ptr,
        )
      )
      adoptOwned(
        output.value,
        { GeneratedOwnerDisposal.geojsonSourceData(it.toLong()) },
        { OwnerAdoption.geojsonSourceData(it) },
      )
    }
  }

  public actual fun geojsonSourceOptionsDefault(): GeojsonSourceOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_geojson_source_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_geojson_source_options_default()
      nativeResult.useContents { GeneratedValues.readGeojsonSourceOptions(this) }
    }
  }

  public actual fun gpuSyncDefault(): GpuSync {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_gpu_sync_default")
    return memScoped {
      val arena = this
      val nativeResult = mln_gpu_sync_default()
      nativeResult.useContents { GeneratedValues.readGpuSync(this) }
    }
  }

  public actual fun httpHeaderTransformResponseSet(
    response: HttpHeaderTransformResponse,
    name: String,
    valueValue: String,
  ): Unit {
    return memScoped {
      val arena = this
      response.bindingScope.ensureActive()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        response.bindingAddress,
        "mln_http_header_transform_response_set",
      )
      BindingStatus.check(
        mln_http_header_transform_response_set(
          (response.bindingAddress.also { response.bindingScope.ensureActive() }).toCPointer<
            mln_http_header_transform_response
          >()!!,
          GeneratedValues.rawBytes(arena, name.encodeToByteArray()),
          name.encodeToByteArray().size.convert(),
          GeneratedValues.rawBytes(arena, valueValue.encodeToByteArray()),
          valueValue.encodeToByteArray().size.convert(),
        )
      )
    }
  }

  public actual fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_lat_lng_for_projected_meters",
    )
    return memScoped {
      val arena = this
      val output = arena.alloc<mln_lat_lng>()
      BindingStatus.check(
        mln_lat_lng_for_projected_meters(
          GeneratedValues.writeProjectedMeters(arena, meters).pointed.readValue(),
          output.ptr,
        )
      )
      GeneratedValues.readLatLng(output)
    }
  }

  public actual fun logClearCallback(): Unit {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_log_clear_callback")
    return memScoped {
      val arena = this
      BindingStatus.check(mln_log_clear_callback())
      Unit
    }
  }

  public actual fun logSetAsyncSeverityMask(mask: LogSeverityMask): Unit {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_log_set_async_severity_mask",
    )
    return memScoped {
      val arena = this
      BindingStatus.check(mln_log_set_async_severity_mask(mask.rawValue))
      Unit
    }
  }

  public actual fun logSetCallback(callback: LogCallback) {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_log_set_callback")
    org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use { registrations ->
      val token = registrations.register(GeneratedLogCallbackRegistration(callback))
      BindingStatus.check(
        mln_log_set_callback(
          GeneratedDirectCallbacks.LogCallbackStub,
          token.toCPointer<ByteVar>(),
          GeneratedDirectCallbacks.LogCallbackReleaseStub,
        )
      )
      registrations.accept(org.maplibre.nativeffi.internal.callback.CallbackOwner.global)
    }
  }

  public actual fun mapOptionsDefault(): MapOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_map_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_map_options_default()
      nativeResult.useContents { GeneratedValues.readMapOptions(this) }
    }
  }

  public actual fun mapTileOptionsDefault(): MapTileOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_map_tile_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_map_tile_options_default()
      nativeResult.useContents { GeneratedValues.readMapTileOptions(this) }
    }
  }

  public actual fun mapViewportOptionsDefault(): MapViewportOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_map_viewport_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_map_viewport_options_default()
      nativeResult.useContents { GeneratedValues.readMapViewportOptions(this) }
    }
  }

  public actual fun metalBorrowedTextureDescriptorDefault(): MetalBorrowedTextureDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_metal_borrowed_texture_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_metal_borrowed_texture_descriptor_default()
      nativeResult.useContents { GeneratedValues.readMetalBorrowedTextureDescriptor(this) }
    }
  }

  public actual fun metalOwnedTextureDescriptorDefault(): MetalOwnedTextureDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_metal_owned_texture_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_metal_owned_texture_descriptor_default()
      nativeResult.useContents { GeneratedValues.readMetalOwnedTextureDescriptor(this) }
    }
  }

  public actual fun metalSurfaceDescriptorDefault(): MetalSurfaceDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_metal_surface_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_metal_surface_descriptor_default()
      nativeResult.useContents { GeneratedValues.readMetalSurfaceDescriptor(this) }
    }
  }

  public actual fun networkStatusGet(): NetworkStatus {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_network_status_get")
    return memScoped {
      val arena = this
      val output = arena.alloc<UIntVar>()
      BindingStatus.check(mln_network_status_get(output.ptr))
      NetworkStatus(output.value)
    }
  }

  public actual fun networkStatusSet(status: NetworkStatus): Unit {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_network_status_set")
    return memScoped {
      val arena = this
      BindingStatus.check(mln_network_status_set(status.rawValue))
      Unit
    }
  }

  public actual fun openglBorrowedTextureDescriptorDefault(): OpenglBorrowedTextureDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_borrowed_texture_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_opengl_borrowed_texture_descriptor_default()
      nativeResult.useContents { GeneratedValues.readOpenglBorrowedTextureDescriptor(this) }
    }
  }

  public actual fun openglOwnedTextureDescriptorDefault(): OpenglOwnedTextureDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_owned_texture_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_opengl_owned_texture_descriptor_default()
      nativeResult.useContents { GeneratedValues.readOpenglOwnedTextureDescriptor(this) }
    }
  }

  public actual fun openglSupportedContextProviderMask(): OpenglContextProviderFlag {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_supported_context_provider_mask",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_opengl_supported_context_provider_mask()
      OpenglContextProviderFlag(nativeResult)
    }
  }

  public actual fun openglSurfaceDescriptorDefault(): OpenglSurfaceDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_surface_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_opengl_surface_descriptor_default()
      nativeResult.useContents { GeneratedValues.readOpenglSurfaceDescriptor(this) }
    }
  }

  public actual fun pluginGetRegisterFunctionV1(): org.maplibre.nativeffi.render.NativePointer {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_plugin_get_register_function_v1",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_plugin_get_register_function_v1()
      org.maplibre.nativeffi.render.NativePointer.ofAddress(
        (nativeResult?.rawValue?.toLong() ?: 0L)
      )
    }
  }

  public actual fun premultipliedRgba8ImageDefault(): PremultipliedRgba8Image {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_premultiplied_rgba8_image_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_premultiplied_rgba8_image_default()
      nativeResult.useContents { GeneratedValues.readPremultipliedRgba8Image(this) }
    }
  }

  public actual fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_projected_meters_for_lat_lng",
    )
    return memScoped {
      val arena = this
      val output = arena.alloc<mln_projected_meters>()
      BindingStatus.check(
        mln_projected_meters_for_lat_lng(
          GeneratedValues.writeLatLng(arena, coordinate).pointed.readValue(),
          output.ptr,
        )
      )
      GeneratedValues.readProjectedMeters(output)
    }
  }

  public actual fun projectionModeDefault(): ProjectionMode {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_projection_mode_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_projection_mode_default()
      nativeResult.useContents { GeneratedValues.readProjectionMode(this) }
    }
  }

  public actual fun renderSessionAttachOptionsDefault(): RenderSessionAttachOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_render_session_attach_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_render_session_attach_options_default()
      nativeResult.useContents { GeneratedValues.readRenderSessionAttachOptions(this) }
    }
  }

  public actual fun renderTargetExtentPhysicalSize(
    extent: RenderTargetExtent
  ): RenderTargetExtentPhysicalSizeResult = memScoped {
    val arena = this
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_render_target_extent_physical_size",
    )
    val out0 = arena.alloc<UIntVar>()
    val out1 = arena.alloc<UIntVar>()
    BindingStatus.check(
      mln_render_target_extent_physical_size(
        GeneratedValues.writeRenderTargetExtent(arena, extent),
        out0.ptr,
        out1.ptr,
      )
    )
    RenderTargetExtentPhysicalSizeResult(width = out0.value, height = out1.value)
  }

  public actual fun renderedFeatureQueryOptionsDefault(): RenderedFeatureQueryOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_feature_query_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_rendered_feature_query_options_default()
      nativeResult.useContents { GeneratedValues.readRenderedFeatureQueryOptions(this) }
    }
  }

  public actual fun renderedQueryGeometryBox(box: ScreenBox): RenderedQueryGeometry {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_query_geometry_box",
    )
    return memScoped {
      val arena = this
      val nativeResult =
        mln_rendered_query_geometry_box(
          GeneratedValues.writeScreenBox(arena, box).pointed.readValue()
        )
      nativeResult.useContents { GeneratedValues.readRenderedQueryGeometry(this) }
    }
  }

  public actual fun renderedQueryGeometryLineString(
    points: List<ScreenPoint>
  ): RenderedQueryGeometry {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_query_geometry_line_string",
    )
    return memScoped {
      val arena = this
      val nativeResult =
        mln_rendered_query_geometry_line_string(
          GeneratedValues.writeScreenPointArray(arena, points),
          points.size.convert(),
        )
      nativeResult.useContents { GeneratedValues.readRenderedQueryGeometry(this) }
    }
  }

  public actual fun renderedQueryGeometryPoint(point: ScreenPoint): RenderedQueryGeometry {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_query_geometry_point",
    )
    return memScoped {
      val arena = this
      val nativeResult =
        mln_rendered_query_geometry_point(
          GeneratedValues.writeScreenPoint(arena, point).pointed.readValue()
        )
      nativeResult.useContents { GeneratedValues.readRenderedQueryGeometry(this) }
    }
  }

  public actual fun resourceTransformResponseSetUrl(
    response: ResourceTransformResponse,
    url: String,
  ): Unit {
    return memScoped {
      val arena = this
      response.bindingScope.ensureActive()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        response.bindingAddress,
        "mln_resource_transform_response_set_url",
      )
      BindingStatus.check(
        mln_resource_transform_response_set_url(
          (response.bindingAddress.also { response.bindingScope.ensureActive() }).toCPointer<
            mln_resource_transform_response
          >()!!,
          GeneratedValues.rawBytes(arena, url.encodeToByteArray()),
          url.encodeToByteArray().size.convert(),
        )
      )
    }
  }

  public actual fun runtimeCreate(
    options: RuntimeOptions
  ): org.maplibre.nativeffi.runtime.RuntimeHandle {
    org.maplibre.nativeffi.Maplibre.loadNativeLibrary()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_runtime_create")
    return run {
      val registrations = CallbackRegistrationScope()
      try {
        memScoped {
          val arena = this
          val output = arena.alloc<ULongVar>().also { it.value = 0uL }
          BindingStatus.check(
            mln_runtime_create(
              GeneratedValues.writeRuntimeOptions(arena, options, registrations),
              output.ptr,
            )
          )
          run {
            val owner =
              adoptOwned(
                output.value,
                { GeneratedOwnerDisposal.runtime(it.toLong()) },
                { OwnerAdoption.runtime(it) },
              )
            try {
              registrations.accept(owner.bindingCallbacks)
              owner
            } catch (failure: Throwable) {
              try {
                owner.dispose()
              } catch (cleanup: Throwable) {
                failure.addSuppressed(cleanup)
              }
              throw failure
            }
          }
        }
      } finally {
        registrations.close()
      }
    }
  }

  public actual fun runtimeOptionsDefault(): RuntimeOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_runtime_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_runtime_options_default()
      nativeResult.useContents { GeneratedValues.readRuntimeOptions(this) }
    }
  }

  public actual fun sourceFeatureQueryOptionsDefault(): SourceFeatureQueryOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_source_feature_query_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_source_feature_query_options_default()
      nativeResult.useContents { GeneratedValues.readSourceFeatureQueryOptions(this) }
    }
  }

  public actual fun styleImageInfoDefault(): StyleImageInfo {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_image_info_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_style_image_info_default()
      nativeResult.useContents { GeneratedValues.readStyleImageInfo(this) }
    }
  }

  public actual fun styleImageOptionsDefault(): StyleImageOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_image_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_style_image_options_default()
      nativeResult.useContents { GeneratedValues.readStyleImageOptions(this) }
    }
  }

  public actual fun styleTileSourceOptionsDefault(): StyleTileSourceOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_tile_source_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_style_tile_source_options_default()
      nativeResult.useContents { GeneratedValues.readStyleTileSourceOptions(this) }
    }
  }

  public actual fun styleTransitionOptionsDefault(): StyleTransitionOptions {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_transition_options_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_style_transition_options_default()
      nativeResult.useContents { GeneratedValues.readStyleTransitionOptions(this) }
    }
  }

  public actual fun supportedRenderBackendMask(): RenderBackendFlag {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_supported_render_backend_mask",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_supported_render_backend_mask()
      RenderBackendFlag(nativeResult)
    }
  }

  public actual fun textureImageInfoDefault(): TextureImageInfo {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_texture_image_info_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_texture_image_info_default()
      nativeResult.useContents { GeneratedValues.readTextureImageInfo(this) }
    }
  }

  public actual fun threadLastErrorMessage(): String {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_thread_last_error_message",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_thread_last_error_message()
      nativeResult!!.toKString()
    }
  }

  public actual fun vulkanBorrowedTextureDescriptorDefault(): VulkanBorrowedTextureDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_vulkan_borrowed_texture_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_vulkan_borrowed_texture_descriptor_default()
      nativeResult.useContents { GeneratedValues.readVulkanBorrowedTextureDescriptor(this) }
    }
  }

  public actual fun vulkanOwnedTextureDescriptorDefault(): VulkanOwnedTextureDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_vulkan_owned_texture_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_vulkan_owned_texture_descriptor_default()
      nativeResult.useContents { GeneratedValues.readVulkanOwnedTextureDescriptor(this) }
    }
  }

  public actual fun vulkanSurfaceDescriptorDefault(): VulkanSurfaceDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_vulkan_surface_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_vulkan_surface_descriptor_default()
      nativeResult.useContents { GeneratedValues.readVulkanSurfaceDescriptor(this) }
    }
  }

  public actual fun webgpuBorrowedTextureDescriptorDefault(): WebgpuBorrowedTextureDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_webgpu_borrowed_texture_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_webgpu_borrowed_texture_descriptor_default()
      nativeResult.useContents { GeneratedValues.readWebgpuBorrowedTextureDescriptor(this) }
    }
  }

  public actual fun webgpuOwnedTextureDescriptorDefault(): WebgpuOwnedTextureDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_webgpu_owned_texture_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_webgpu_owned_texture_descriptor_default()
      nativeResult.useContents { GeneratedValues.readWebgpuOwnedTextureDescriptor(this) }
    }
  }

  public actual fun webgpuSurfaceDescriptorDefault(): WebgpuSurfaceDescriptor {
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_webgpu_surface_descriptor_default",
    )
    return memScoped {
      val arena = this
      val nativeResult = mln_webgpu_surface_descriptor_default()
      nativeResult.useContents { GeneratedValues.readWebgpuSurfaceDescriptor(this) }
    }
  }
}
