// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.bytedeco.javacpp.*
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

public actual object GeneratedApi {
  public actual fun androidInit(
    jniEnv: org.maplibre.nativeffi.render.NativePointer,
    jniClass: org.maplibre.nativeffi.render.NativePointer,
    context: org.maplibre.nativeffi.render.NativePointer,
  ): Unit {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_android_init")
    return PointerScope().use { arena ->
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_android_init(
          org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(jniEnv.address),
          org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(jniClass.address),
          org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(context.address),
          diagnostic,
        )
      }
      Unit
    }
  }

  public actual fun animationOptionsDefault(): AnimationOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_animation_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_animation_options_default()
      GeneratedValues.readAnimationOptions(nativeResult)
    }
  }

  public actual fun boundOptionsDefault(): BoundOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_bound_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_bound_options_default()
      GeneratedValues.readBoundOptions(nativeResult)
    }
  }

  public actual fun cVersion(): UInt {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_c_version")
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_c_version()
      nativeResult.toUInt()
    }
  }

  public actual fun cameraDeltaDefault(): CameraDelta {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_delta_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_camera_delta_default()
      GeneratedValues.readCameraDelta(nativeResult)
    }
  }

  public actual fun cameraFitOptionsDefault(): CameraFitOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_fit_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_camera_fit_options_default()
      GeneratedValues.readCameraFitOptions(nativeResult)
    }
  }

  public actual fun cameraOptionsDefault(): CameraOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_camera_options_default()
      GeneratedValues.readCameraOptions(nativeResult)
    }
  }

  public actual fun cameraUpdateDefault(): CameraUpdate {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_update_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_camera_update_default()
      GeneratedValues.readCameraUpdate(nativeResult)
    }
  }

  public actual fun customGeometrySourceOptionsDefault(): CustomGeometrySourceOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_custom_geometry_source_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_custom_geometry_source_options_default()
      GeneratedCallbacks.readCustomGeometrySourceOptions(nativeResult)
    }
  }

  public actual fun customMvtVectorSourceOptionsDefault(): CustomMvtVectorSourceOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_custom_mvt_vector_source_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_custom_mvt_vector_source_options_default()
      GeneratedCallbacks.readCustomMvtVectorSourceOptions(nativeResult)
    }
  }

  public actual fun frameDemandDefault(): FrameDemand {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_frame_demand_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_frame_demand_default()
      GeneratedValues.readFrameDemand(nativeResult)
    }
  }

  public actual fun freeCameraOptionsDefault(): FreeCameraOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_free_camera_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_free_camera_options_default()
      GeneratedValues.readFreeCameraOptions(nativeResult)
    }
  }

  public actual fun geojsonSourceDataCreate(
    data: ByteArray,
    options: GeojsonSourceOptions?,
  ): org.maplibre.nativeffi.generated.GeojsonSourceDataHandle {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_geojson_source_data_create",
    )
    return PointerScope().use { arena ->
      val output = LongPointer(1L).put(0L)
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_geojson_source_data_create(
          GeneratedValues.byteView(arena, data),
          if (options == null) null
          else GeneratedValues.writeGeojsonSourceOptions(arena, options!!),
          output,
          diagnostic,
        )
      }
      adoptOwned(
        output.get(),
        { GeneratedOwnerDisposal.geojsonSourceData(it) },
        { GeojsonSourceDataHandle(it) },
      )
    }
  }

  public actual fun geojsonSourceOptionsDefault(): GeojsonSourceOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_geojson_source_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_geojson_source_options_default()
      GeneratedValues.readGeojsonSourceOptions(nativeResult)
    }
  }

  public actual fun gpuSyncDefault(): GpuSync {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_gpu_sync_default")
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_gpu_sync_default()
      GeneratedValues.readGpuSync(nativeResult)
    }
  }

  public actual fun httpHeaderTransformResponseSet(
    response: HttpHeaderTransformResponse,
    name: String,
    valueValue: String,
  ): Unit {
    NativeAccess.ensureLoaded()
    return PointerScope().use { arena ->
      response.bindingScope.ensureActive()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        response.bindingAddress,
        "mln_http_header_transform_response_set",
      )
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_http_header_transform_response_set(
          MaplibreNativeC.mln_http_header_transform_response(
            org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(
              response.bindingAddress.also { response.bindingScope.ensureActive() }
            )
          ),
          GeneratedValues.rawBytes(arena, name.encodeToByteArray()),
          name.encodeToByteArray().size.toLong(),
          GeneratedValues.rawBytes(arena, valueValue.encodeToByteArray()),
          valueValue.encodeToByteArray().size.toLong(),
          diagnostic,
        )
      }
    }
  }

  public actual fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_lat_lng_for_projected_meters",
    )
    return PointerScope().use { arena ->
      val output = MaplibreNativeC.mln_lat_lng()
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_lat_lng_for_projected_meters(
          GeneratedValues.writeProjectedMeters(arena, meters),
          output,
          diagnostic,
        )
      }
      GeneratedValues.readLatLng(output)
    }
  }

  public actual fun logClearCallback(): Unit {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_log_clear_callback")
    return PointerScope().use { arena ->
      NativeDiagnostics.check { diagnostic -> MaplibreNativeC.mln_log_clear_callback(diagnostic) }
      Unit
    }
  }

  public actual fun logSetAsyncSeverityMask(mask: LogSeverityMask): Unit {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_log_set_async_severity_mask",
    )
    return PointerScope().use { arena ->
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_log_set_async_severity_mask(mask.rawValue.toInt(), diagnostic)
      }
      Unit
    }
  }

  public actual fun logSetCallback(callback: LogCallback) {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_log_set_callback")
    org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use { registrations ->
      val token = registrations.register(GeneratedLogCallbackRegistration(callback))
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_log_set_callback(
          GeneratedDirectCallbacks.LogCallbackStub,
          org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(token),
          GeneratedDirectCallbacks.LogCallbackReleaseStub,
          diagnostic,
        )
      }
      registrations.accept(org.maplibre.nativeffi.internal.callback.CallbackOwner.global)
    }
  }

  public actual fun mapOptionsDefault(): MapOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_map_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_map_options_default()
      GeneratedValues.readMapOptions(nativeResult)
    }
  }

  public actual fun mapTileOptionsDefault(): MapTileOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_map_tile_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_map_tile_options_default()
      GeneratedValues.readMapTileOptions(nativeResult)
    }
  }

  public actual fun mapViewportOptionsDefault(): MapViewportOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_map_viewport_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_map_viewport_options_default()
      GeneratedValues.readMapViewportOptions(nativeResult)
    }
  }

  public actual fun metalBorrowedTextureDescriptorDefault(): MetalBorrowedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_metal_borrowed_texture_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_metal_borrowed_texture_descriptor_default()
      GeneratedValues.readMetalBorrowedTextureDescriptor(nativeResult)
    }
  }

  public actual fun metalOwnedTextureDescriptorDefault(): MetalOwnedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_metal_owned_texture_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_metal_owned_texture_descriptor_default()
      GeneratedValues.readMetalOwnedTextureDescriptor(nativeResult)
    }
  }

  public actual fun metalSurfaceDescriptorDefault(): MetalSurfaceDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_metal_surface_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_metal_surface_descriptor_default()
      GeneratedValues.readMetalSurfaceDescriptor(nativeResult)
    }
  }

  public actual fun networkStatusGet(): NetworkStatus {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_network_status_get")
    return PointerScope().use { arena ->
      val output = IntPointer(1L)
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_network_status_get(output, diagnostic)
      }
      NetworkStatus(output.get().toUInt())
    }
  }

  public actual fun networkStatusSet(status: NetworkStatus): Unit {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_network_status_set")
    return PointerScope().use { arena ->
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_network_status_set(status.rawValue.toInt(), diagnostic)
      }
      Unit
    }
  }

  public actual fun openglBorrowedTextureDescriptorDefault(): OpenglBorrowedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_borrowed_texture_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_opengl_borrowed_texture_descriptor_default()
      GeneratedValues.readOpenglBorrowedTextureDescriptor(nativeResult)
    }
  }

  public actual fun openglOwnedTextureDescriptorDefault(): OpenglOwnedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_owned_texture_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_opengl_owned_texture_descriptor_default()
      GeneratedValues.readOpenglOwnedTextureDescriptor(nativeResult)
    }
  }

  public actual fun openglSupportedContextProviderMask(): OpenglContextProviderFlag {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_supported_context_provider_mask",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_opengl_supported_context_provider_mask()
      OpenglContextProviderFlag(nativeResult.toUInt())
    }
  }

  public actual fun openglSurfaceDescriptorDefault(): OpenglSurfaceDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_surface_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_opengl_surface_descriptor_default()
      GeneratedValues.readOpenglSurfaceDescriptor(nativeResult)
    }
  }

  public actual fun pluginGetRegisterFunctionV1(): org.maplibre.nativeffi.render.NativePointer {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_plugin_get_register_function_v1",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_plugin_get_register_function_v1()
      org.maplibre.nativeffi.render.NativePointer.ofAddress((nativeResult?.address() ?: 0L))
    }
  }

  public actual fun premultipliedRgba8ImageDefault(): PremultipliedRgba8Image {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_premultiplied_rgba8_image_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_premultiplied_rgba8_image_default()
      GeneratedValues.readPremultipliedRgba8Image(nativeResult)
    }
  }

  public actual fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_projected_meters_for_lat_lng",
    )
    return PointerScope().use { arena ->
      val output = MaplibreNativeC.mln_projected_meters()
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_projected_meters_for_lat_lng(
          GeneratedValues.writeLatLng(arena, coordinate),
          output,
          diagnostic,
        )
      }
      GeneratedValues.readProjectedMeters(output)
    }
  }

  public actual fun projectionModeDefault(): ProjectionMode {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_projection_mode_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_projection_mode_default()
      GeneratedValues.readProjectionMode(nativeResult)
    }
  }

  public actual fun renderSessionAttachOptionsDefault(): RenderSessionAttachOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_render_session_attach_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_render_session_attach_options_default()
      GeneratedValues.readRenderSessionAttachOptions(nativeResult)
    }
  }

  public actual fun renderTargetExtentPhysicalSize(
    extent: RenderTargetExtent
  ): RenderTargetExtentPhysicalSizeResult =
    PointerScope().use { arena ->
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        null,
        "mln_render_target_extent_physical_size",
      )
      val out0 = IntPointer(1L)
      val out1 = IntPointer(1L)
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_render_target_extent_physical_size(
          GeneratedValues.writeRenderTargetExtent(arena, extent),
          out0,
          out1,
          diagnostic,
        )
      }
      RenderTargetExtentPhysicalSizeResult(
        width = out0.get().toUInt(),
        height = out1.get().toUInt(),
      )
    }

  public actual fun renderedFeatureQueryOptionsDefault(): RenderedFeatureQueryOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_feature_query_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_rendered_feature_query_options_default()
      GeneratedValues.readRenderedFeatureQueryOptions(nativeResult)
    }
  }

  public actual fun renderedQueryGeometryBox(box: ScreenBox): RenderedQueryGeometry {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_query_geometry_box",
    )
    return PointerScope().use { arena ->
      val nativeResult =
        MaplibreNativeC.mln_rendered_query_geometry_box(GeneratedValues.writeScreenBox(arena, box))
      GeneratedValues.readRenderedQueryGeometry(nativeResult)
    }
  }

  public actual fun renderedQueryGeometryLineString(
    points: List<ScreenPoint>
  ): RenderedQueryGeometry {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_query_geometry_line_string",
    )
    return PointerScope().use { arena ->
      val nativeResult =
        MaplibreNativeC.mln_rendered_query_geometry_line_string(
          GeneratedValues.writeScreenPointArray(arena, points),
          points.size.toLong(),
        )
      GeneratedValues.readRenderedQueryGeometry(nativeResult)
    }
  }

  public actual fun renderedQueryGeometryPoint(point: ScreenPoint): RenderedQueryGeometry {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_query_geometry_point",
    )
    return PointerScope().use { arena ->
      val nativeResult =
        MaplibreNativeC.mln_rendered_query_geometry_point(
          GeneratedValues.writeScreenPoint(arena, point)
        )
      GeneratedValues.readRenderedQueryGeometry(nativeResult)
    }
  }

  public actual fun resourceTransformResponseSetUrl(
    response: ResourceTransformResponse,
    url: String,
  ): Unit {
    NativeAccess.ensureLoaded()
    return PointerScope().use { arena ->
      response.bindingScope.ensureActive()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        response.bindingAddress,
        "mln_resource_transform_response_set_url",
      )
      NativeDiagnostics.check { diagnostic ->
        MaplibreNativeC.mln_resource_transform_response_set_url(
          MaplibreNativeC.mln_resource_transform_response(
            org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(
              response.bindingAddress.also { response.bindingScope.ensureActive() }
            )
          ),
          GeneratedValues.rawBytes(arena, url.encodeToByteArray()),
          url.encodeToByteArray().size.toLong(),
          diagnostic,
        )
      }
    }
  }

  public actual fun runtimeCreate(
    options: RuntimeOptions
  ): org.maplibre.nativeffi.generated.RuntimeHandle {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_runtime_create")
    return run {
      val registrations = CallbackRegistrationScope()
      try {
        PointerScope().use { arena ->
          val output = LongPointer(1L).put(0L)
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_runtime_create(
              GeneratedValues.writeRuntimeOptions(arena, options, registrations),
              output,
              diagnostic,
            )
          }
          run {
            val owner =
              adoptOwned(
                output.get(),
                { GeneratedOwnerDisposal.runtime(it) },
                { RuntimeHandle(it) },
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
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_runtime_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_runtime_options_default()
      GeneratedValues.readRuntimeOptions(nativeResult)
    }
  }

  public actual fun sourceFeatureQueryOptionsDefault(): SourceFeatureQueryOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_source_feature_query_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_source_feature_query_options_default()
      GeneratedValues.readSourceFeatureQueryOptions(nativeResult)
    }
  }

  public actual fun styleImageInfoDefault(): StyleImageInfo {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_image_info_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_style_image_info_default()
      GeneratedValues.readStyleImageInfo(nativeResult)
    }
  }

  public actual fun styleImageOptionsDefault(): StyleImageOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_image_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_style_image_options_default()
      GeneratedValues.readStyleImageOptions(nativeResult)
    }
  }

  public actual fun styleTileSourceOptionsDefault(): StyleTileSourceOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_tile_source_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_style_tile_source_options_default()
      GeneratedValues.readStyleTileSourceOptions(nativeResult)
    }
  }

  public actual fun styleTransitionOptionsDefault(): StyleTransitionOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_transition_options_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_style_transition_options_default()
      GeneratedValues.readStyleTransitionOptions(nativeResult)
    }
  }

  public actual fun supportedRenderBackendMask(): RenderBackendFlag {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_supported_render_backend_mask",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_supported_render_backend_mask()
      RenderBackendFlag(nativeResult.toUInt())
    }
  }

  public actual fun textureImageInfoDefault(): TextureImageInfo {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_texture_image_info_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_texture_image_info_default()
      GeneratedValues.readTextureImageInfo(nativeResult)
    }
  }

  public actual fun vulkanBorrowedTextureDescriptorDefault(): VulkanBorrowedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_vulkan_borrowed_texture_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_vulkan_borrowed_texture_descriptor_default()
      GeneratedValues.readVulkanBorrowedTextureDescriptor(nativeResult)
    }
  }

  public actual fun vulkanOwnedTextureDescriptorDefault(): VulkanOwnedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_vulkan_owned_texture_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_vulkan_owned_texture_descriptor_default()
      GeneratedValues.readVulkanOwnedTextureDescriptor(nativeResult)
    }
  }

  public actual fun vulkanSurfaceDescriptorDefault(): VulkanSurfaceDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_vulkan_surface_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_vulkan_surface_descriptor_default()
      GeneratedValues.readVulkanSurfaceDescriptor(nativeResult)
    }
  }

  public actual fun webgpuBorrowedTextureDescriptorDefault(): WebgpuBorrowedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_webgpu_borrowed_texture_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_webgpu_borrowed_texture_descriptor_default()
      GeneratedValues.readWebgpuBorrowedTextureDescriptor(nativeResult)
    }
  }

  public actual fun webgpuOwnedTextureDescriptorDefault(): WebgpuOwnedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_webgpu_owned_texture_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_webgpu_owned_texture_descriptor_default()
      GeneratedValues.readWebgpuOwnedTextureDescriptor(nativeResult)
    }
  }

  public actual fun webgpuSurfaceDescriptorDefault(): WebgpuSurfaceDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_webgpu_surface_descriptor_default",
    )
    return PointerScope().use { arena ->
      val nativeResult = MaplibreNativeC.mln_webgpu_surface_descriptor_default()
      GeneratedValues.readWebgpuSurfaceDescriptor(nativeResult)
    }
  }
}
