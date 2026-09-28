// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.async.adoptOwned
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.lifecycle.OwnerAdoption
import org.maplibre.nativeffi.internal.loader.NativeAccess
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

public actual object GeneratedApi {
  public actual fun androidInit(
    jniEnv: org.maplibre.nativeffi.render.NativePointer,
    jniClass: org.maplibre.nativeffi.render.NativePointer,
    context: org.maplibre.nativeffi.render.NativePointer,
  ): Unit {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_android_init")
    return Arena.ofConfined().use { arena ->
      BindingStatus.check(
        MapLibreNativeC.mln_android_init(
          MemorySegment.ofAddress(jniEnv.address),
          MemorySegment.ofAddress(jniClass.address),
          MemorySegment.ofAddress(context.address),
        )
      )
      Unit
    }
  }

  public actual fun animationOptionsDefault(): AnimationOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_animation_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_animation_options_default(arena)
      GeneratedValues.readAnimationOptions(nativeResult)
    }
  }

  public actual fun boundOptionsDefault(): BoundOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_bound_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_bound_options_default(arena)
      GeneratedValues.readBoundOptions(nativeResult)
    }
  }

  public actual fun cVersion(): UInt {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_c_version")
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_c_version()
      nativeResult.toUInt()
    }
  }

  public actual fun cameraDeltaDefault(): CameraDelta {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_delta_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_camera_delta_default(arena)
      GeneratedValues.readCameraDelta(nativeResult)
    }
  }

  public actual fun cameraFitOptionsDefault(): CameraFitOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_fit_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_camera_fit_options_default(arena)
      GeneratedValues.readCameraFitOptions(nativeResult)
    }
  }

  public actual fun cameraOptionsDefault(): CameraOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_camera_options_default(arena)
      GeneratedValues.readCameraOptions(nativeResult)
    }
  }

  public actual fun cameraUpdateDefault(): CameraUpdate {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_camera_update_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_camera_update_default(arena)
      GeneratedValues.readCameraUpdate(nativeResult)
    }
  }

  public actual fun customGeometrySourceOptionsDefault(): CustomGeometrySourceOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_custom_geometry_source_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_custom_geometry_source_options_default(arena)
      GeneratedCallbacks.readCustomGeometrySourceOptions(nativeResult)
    }
  }

  public actual fun customMvtVectorSourceOptionsDefault(): CustomMvtVectorSourceOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_custom_mvt_vector_source_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_custom_mvt_vector_source_options_default(arena)
      GeneratedCallbacks.readCustomMvtVectorSourceOptions(nativeResult)
    }
  }

  public actual fun frameDemandDefault(): FrameDemand {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_frame_demand_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_frame_demand_default(arena)
      GeneratedValues.readFrameDemand(nativeResult)
    }
  }

  public actual fun freeCameraOptionsDefault(): FreeCameraOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_free_camera_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_free_camera_options_default(arena)
      GeneratedValues.readFreeCameraOptions(nativeResult)
    }
  }

  public actual fun geojsonSourceDataCreate(
    data: ByteArray,
    options: GeojsonSourceOptions?,
  ): org.maplibre.nativeffi.style.GeoJsonSourceDataHandle {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_geojson_source_data_create",
    )
    return Arena.ofConfined().use { arena ->
      val output = arena.allocate(ValueLayout.JAVA_LONG)
      BindingStatus.check(
        MapLibreNativeC.mln_geojson_source_data_create(
          GeneratedValues.byteView(arena, data),
          if (options == null) MemorySegment.NULL
          else GeneratedValues.writeGeojsonSourceOptions(arena, options!!),
          output,
        )
      )
      adoptOwned(
        output.get(ValueLayout.JAVA_LONG, 0),
        { GeneratedOwnerDisposal.geojsonSourceData(it) },
        { OwnerAdoption.geojsonSourceData(it) },
      )
    }
  }

  public actual fun geojsonSourceOptionsDefault(): GeojsonSourceOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_geojson_source_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_geojson_source_options_default(arena)
      GeneratedValues.readGeojsonSourceOptions(nativeResult)
    }
  }

  public actual fun gpuSyncDefault(): GpuSync {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_gpu_sync_default")
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_gpu_sync_default(arena)
      GeneratedValues.readGpuSync(nativeResult)
    }
  }

  public actual fun httpHeaderTransformResponseSet(
    response: HttpHeaderTransformResponse,
    name: String,
    valueValue: String,
  ): Unit {
    NativeAccess.ensureLoaded()
    return Arena.ofConfined().use { arena ->
      response.bindingScope.ensureActive()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        response.bindingAddress,
        "mln_http_header_transform_response_set",
      )
      BindingStatus.check(
        MapLibreNativeC.mln_http_header_transform_response_set(
          MemorySegment.ofAddress(
              response.bindingAddress.also { response.bindingScope.ensureActive() }
            )
            .reinterpret(mln_http_header_transform_response.sizeof()),
          GeneratedValues.rawBytes(arena, name.encodeToByteArray()),
          name.encodeToByteArray().size.toLong(),
          GeneratedValues.rawBytes(arena, valueValue.encodeToByteArray()),
          valueValue.encodeToByteArray().size.toLong(),
        )
      )
    }
  }

  public actual fun latLngForProjectedMeters(meters: ProjectedMeters): LatLng {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_lat_lng_for_projected_meters",
    )
    return Arena.ofConfined().use { arena ->
      val output = mln_lat_lng.allocate(arena)
      BindingStatus.check(
        MapLibreNativeC.mln_lat_lng_for_projected_meters(
          GeneratedValues.writeProjectedMeters(arena, meters),
          output,
        )
      )
      GeneratedValues.readLatLng(output)
    }
  }

  public actual fun logClearCallback(): Unit {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_log_clear_callback")
    return Arena.ofConfined().use { arena ->
      BindingStatus.check(MapLibreNativeC.mln_log_clear_callback())
      Unit
    }
  }

  public actual fun logSetAsyncSeverityMask(mask: LogSeverityMask): Unit {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_log_set_async_severity_mask",
    )
    return Arena.ofConfined().use { arena ->
      BindingStatus.check(MapLibreNativeC.mln_log_set_async_severity_mask(mask.rawValue.toInt()))
      Unit
    }
  }

  public actual fun logSetCallback(callback: LogCallback) {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_log_set_callback")
    org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use { registrations ->
      val token = registrations.register(GeneratedLogCallbackRegistration(callback))
      BindingStatus.check(
        MapLibreNativeC.mln_log_set_callback(
          GeneratedDirectCallbacks.LogCallbackStub,
          MemorySegment.ofAddress(token),
          GeneratedDirectCallbacks.LogCallbackReleaseStub,
        )
      )
      registrations.accept(org.maplibre.nativeffi.internal.callback.CallbackOwner.global)
    }
  }

  public actual fun mapOptionsDefault(): MapOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_map_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_map_options_default(arena)
      GeneratedValues.readMapOptions(nativeResult)
    }
  }

  public actual fun mapTileOptionsDefault(): MapTileOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_map_tile_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_map_tile_options_default(arena)
      GeneratedValues.readMapTileOptions(nativeResult)
    }
  }

  public actual fun mapViewportOptionsDefault(): MapViewportOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_map_viewport_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_map_viewport_options_default(arena)
      GeneratedValues.readMapViewportOptions(nativeResult)
    }
  }

  public actual fun metalBorrowedTextureDescriptorDefault(): MetalBorrowedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_metal_borrowed_texture_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_metal_borrowed_texture_descriptor_default(arena)
      GeneratedValues.readMetalBorrowedTextureDescriptor(nativeResult)
    }
  }

  public actual fun metalOwnedTextureDescriptorDefault(): MetalOwnedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_metal_owned_texture_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_metal_owned_texture_descriptor_default(arena)
      GeneratedValues.readMetalOwnedTextureDescriptor(nativeResult)
    }
  }

  public actual fun metalSurfaceDescriptorDefault(): MetalSurfaceDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_metal_surface_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_metal_surface_descriptor_default(arena)
      GeneratedValues.readMetalSurfaceDescriptor(nativeResult)
    }
  }

  public actual fun networkStatusGet(): NetworkStatus {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_network_status_get")
    return Arena.ofConfined().use { arena ->
      val output = arena.allocate(ValueLayout.JAVA_INT)
      BindingStatus.check(MapLibreNativeC.mln_network_status_get(output))
      NetworkStatus(output.get(ValueLayout.JAVA_INT, 0).toUInt())
    }
  }

  public actual fun networkStatusSet(status: NetworkStatus): Unit {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_network_status_set")
    return Arena.ofConfined().use { arena ->
      BindingStatus.check(MapLibreNativeC.mln_network_status_set(status.rawValue.toInt()))
      Unit
    }
  }

  public actual fun openglBorrowedTextureDescriptorDefault(): OpenglBorrowedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_borrowed_texture_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_opengl_borrowed_texture_descriptor_default(arena)
      GeneratedValues.readOpenglBorrowedTextureDescriptor(nativeResult)
    }
  }

  public actual fun openglOwnedTextureDescriptorDefault(): OpenglOwnedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_owned_texture_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_opengl_owned_texture_descriptor_default(arena)
      GeneratedValues.readOpenglOwnedTextureDescriptor(nativeResult)
    }
  }

  public actual fun openglSupportedContextProviderMask(): OpenglContextProviderFlag {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_supported_context_provider_mask",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_opengl_supported_context_provider_mask()
      OpenglContextProviderFlag(nativeResult.toUInt())
    }
  }

  public actual fun openglSurfaceDescriptorDefault(): OpenglSurfaceDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_opengl_surface_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_opengl_surface_descriptor_default(arena)
      GeneratedValues.readOpenglSurfaceDescriptor(nativeResult)
    }
  }

  public actual fun pluginGetRegisterFunctionV1(): org.maplibre.nativeffi.render.NativePointer {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_plugin_get_register_function_v1",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_plugin_get_register_function_v1()
      org.maplibre.nativeffi.render.NativePointer.ofAddress(nativeResult.address())
    }
  }

  public actual fun premultipliedRgba8ImageDefault(): PremultipliedRgba8Image {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_premultiplied_rgba8_image_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_premultiplied_rgba8_image_default(arena)
      GeneratedValues.readPremultipliedRgba8Image(nativeResult)
    }
  }

  public actual fun projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_projected_meters_for_lat_lng",
    )
    return Arena.ofConfined().use { arena ->
      val output = mln_projected_meters.allocate(arena)
      BindingStatus.check(
        MapLibreNativeC.mln_projected_meters_for_lat_lng(
          GeneratedValues.writeLatLng(arena, coordinate),
          output,
        )
      )
      GeneratedValues.readProjectedMeters(output)
    }
  }

  public actual fun projectionModeDefault(): ProjectionMode {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_projection_mode_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_projection_mode_default(arena)
      GeneratedValues.readProjectionMode(nativeResult)
    }
  }

  public actual fun renderSessionAttachOptionsDefault(): RenderSessionAttachOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_render_session_attach_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_render_session_attach_options_default(arena)
      GeneratedValues.readRenderSessionAttachOptions(nativeResult)
    }
  }

  public actual fun renderTargetExtentPhysicalSize(
    extent: RenderTargetExtent
  ): RenderTargetExtentPhysicalSizeResult =
    Arena.ofConfined().use { arena ->
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        null,
        "mln_render_target_extent_physical_size",
      )
      val out0 = arena.allocate(ValueLayout.JAVA_INT)
      val out1 = arena.allocate(ValueLayout.JAVA_INT)
      BindingStatus.check(
        MapLibreNativeC.mln_render_target_extent_physical_size(
          GeneratedValues.writeRenderTargetExtent(arena, extent),
          out0,
          out1,
        )
      )
      RenderTargetExtentPhysicalSizeResult(
        width = out0.get(ValueLayout.JAVA_INT, 0).toUInt(),
        height = out1.get(ValueLayout.JAVA_INT, 0).toUInt(),
      )
    }

  public actual fun renderedFeatureQueryOptionsDefault(): RenderedFeatureQueryOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_feature_query_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_rendered_feature_query_options_default(arena)
      GeneratedValues.readRenderedFeatureQueryOptions(nativeResult)
    }
  }

  public actual fun renderedQueryGeometryBox(box: ScreenBox): RenderedQueryGeometry {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_rendered_query_geometry_box",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult =
        MapLibreNativeC.mln_rendered_query_geometry_box(
          arena,
          GeneratedValues.writeScreenBox(arena, box),
        )
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
    return Arena.ofConfined().use { arena ->
      val nativeResult =
        MapLibreNativeC.mln_rendered_query_geometry_line_string(
          arena,
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
    return Arena.ofConfined().use { arena ->
      val nativeResult =
        MapLibreNativeC.mln_rendered_query_geometry_point(
          arena,
          GeneratedValues.writeScreenPoint(arena, point),
        )
      GeneratedValues.readRenderedQueryGeometry(nativeResult)
    }
  }

  public actual fun resourceTransformResponseSetUrl(
    response: ResourceTransformResponse,
    url: String,
  ): Unit {
    NativeAccess.ensureLoaded()
    return Arena.ofConfined().use { arena ->
      response.bindingScope.ensureActive()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        response.bindingAddress,
        "mln_resource_transform_response_set_url",
      )
      BindingStatus.check(
        MapLibreNativeC.mln_resource_transform_response_set_url(
          MemorySegment.ofAddress(
              response.bindingAddress.also { response.bindingScope.ensureActive() }
            )
            .reinterpret(mln_resource_transform_response.sizeof()),
          GeneratedValues.rawBytes(arena, url.encodeToByteArray()),
          url.encodeToByteArray().size.toLong(),
        )
      )
    }
  }

  public actual fun runtimeCreate(
    options: RuntimeOptions
  ): org.maplibre.nativeffi.runtime.RuntimeHandle {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(null, "mln_runtime_create")
    return run {
      val registrations = CallbackRegistrationScope()
      try {
        Arena.ofConfined().use { arena ->
          val output = arena.allocate(ValueLayout.JAVA_LONG)
          BindingStatus.check(
            MapLibreNativeC.mln_runtime_create(
              GeneratedValues.writeRuntimeOptions(arena, options, registrations),
              output,
            )
          )
          run {
            val owner =
              adoptOwned(
                output.get(ValueLayout.JAVA_LONG, 0),
                { GeneratedOwnerDisposal.runtime(it) },
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
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_runtime_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_runtime_options_default(arena)
      GeneratedValues.readRuntimeOptions(nativeResult)
    }
  }

  public actual fun sourceFeatureQueryOptionsDefault(): SourceFeatureQueryOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_source_feature_query_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_source_feature_query_options_default(arena)
      GeneratedValues.readSourceFeatureQueryOptions(nativeResult)
    }
  }

  public actual fun styleImageInfoDefault(): StyleImageInfo {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_image_info_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_style_image_info_default(arena)
      GeneratedValues.readStyleImageInfo(nativeResult)
    }
  }

  public actual fun styleImageOptionsDefault(): StyleImageOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_image_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_style_image_options_default(arena)
      GeneratedValues.readStyleImageOptions(nativeResult)
    }
  }

  public actual fun styleTileSourceOptionsDefault(): StyleTileSourceOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_tile_source_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_style_tile_source_options_default(arena)
      GeneratedValues.readStyleTileSourceOptions(nativeResult)
    }
  }

  public actual fun styleTransitionOptionsDefault(): StyleTransitionOptions {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_style_transition_options_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_style_transition_options_default(arena)
      GeneratedValues.readStyleTransitionOptions(nativeResult)
    }
  }

  public actual fun supportedRenderBackendMask(): RenderBackendFlag {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_supported_render_backend_mask",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_supported_render_backend_mask()
      RenderBackendFlag(nativeResult.toUInt())
    }
  }

  public actual fun textureImageInfoDefault(): TextureImageInfo {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_texture_image_info_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_texture_image_info_default(arena)
      GeneratedValues.readTextureImageInfo(nativeResult)
    }
  }

  public actual fun threadLastErrorMessage(): String {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_thread_last_error_message",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_thread_last_error_message()
      nativeResult.reinterpret(Long.MAX_VALUE).getString(0)
    }
  }

  public actual fun vulkanBorrowedTextureDescriptorDefault(): VulkanBorrowedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_vulkan_borrowed_texture_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_vulkan_borrowed_texture_descriptor_default(arena)
      GeneratedValues.readVulkanBorrowedTextureDescriptor(nativeResult)
    }
  }

  public actual fun vulkanOwnedTextureDescriptorDefault(): VulkanOwnedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_vulkan_owned_texture_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_vulkan_owned_texture_descriptor_default(arena)
      GeneratedValues.readVulkanOwnedTextureDescriptor(nativeResult)
    }
  }

  public actual fun vulkanSurfaceDescriptorDefault(): VulkanSurfaceDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_vulkan_surface_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_vulkan_surface_descriptor_default(arena)
      GeneratedValues.readVulkanSurfaceDescriptor(nativeResult)
    }
  }

  public actual fun webgpuBorrowedTextureDescriptorDefault(): WebgpuBorrowedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_webgpu_borrowed_texture_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_webgpu_borrowed_texture_descriptor_default(arena)
      GeneratedValues.readWebgpuBorrowedTextureDescriptor(nativeResult)
    }
  }

  public actual fun webgpuOwnedTextureDescriptorDefault(): WebgpuOwnedTextureDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_webgpu_owned_texture_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_webgpu_owned_texture_descriptor_default(arena)
      GeneratedValues.readWebgpuOwnedTextureDescriptor(nativeResult)
    }
  }

  public actual fun webgpuSurfaceDescriptorDefault(): WebgpuSurfaceDescriptor {
    NativeAccess.ensureLoaded()
    org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
      null,
      "mln_webgpu_surface_descriptor_default",
    )
    return Arena.ofConfined().use { arena ->
      val nativeResult = MapLibreNativeC.mln_webgpu_surface_descriptor_default(arena)
      GeneratedValues.readWebgpuSurfaceDescriptor(nativeResult)
    }
  }
}
