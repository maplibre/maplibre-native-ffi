// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

/// Calls `mln_android_init`.
///
/// # Safety
/// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
pub unsafe fn android_init(
    jni_env: *mut std::ffi::c_void,
    jni_class: *mut std::ffi::c_void,
    context: *mut std::ffi::c_void,
) -> Result<()> {
    let mut call = Call::global("mln_android_init")?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_android_init(jni_env, jni_class, context, out_diagnostic)
    })?;
    Ok(())
}

/// Calls `mln_animation_options_default`.
pub fn animation_options_default() -> Result<AnimationOptions> {
    let mut call = Call::global("mln_animation_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_animation_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_bound_options_default`.
pub fn bound_options_default() -> Result<BoundOptions> {
    let mut call = Call::global("mln_bound_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_bound_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_c_version`.
pub fn c_version() -> Result<u32> {
    let mut call = Call::global("mln_c_version")?;
    let value = call.run(|_| unsafe { sys::mln_c_version() });
    Ok(value)
}

/// Calls `mln_camera_delta_default`.
pub fn camera_delta_default() -> Result<CameraDelta> {
    let mut call = Call::global("mln_camera_delta_default")?;
    let value = call.run(|_| unsafe { sys::mln_camera_delta_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_camera_fit_options_default`.
pub fn camera_fit_options_default() -> Result<CameraFitOptions> {
    let mut call = Call::global("mln_camera_fit_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_camera_fit_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_camera_options_default`.
pub fn camera_options_default() -> Result<CameraOptions> {
    let mut call = Call::global("mln_camera_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_camera_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_camera_update_default`.
pub fn camera_update_default() -> Result<CameraUpdate> {
    let mut call = Call::global("mln_camera_update_default")?;
    let value = call.run(|_| unsafe { sys::mln_camera_update_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_custom_geometry_source_options_default`.
pub fn custom_geometry_source_options_default() -> Result<CustomGeometrySourceOptions> {
    let mut call = Call::global("mln_custom_geometry_source_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_custom_geometry_source_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_custom_mvt_vector_source_options_default`.
pub fn custom_mvt_vector_source_options_default() -> Result<CustomMvtVectorSourceOptions> {
    let mut call = Call::global("mln_custom_mvt_vector_source_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_custom_mvt_vector_source_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_frame_demand_default`.
pub fn frame_demand_default() -> Result<FrameDemand> {
    let mut call = Call::global("mln_frame_demand_default")?;
    let value = call.run(|_| unsafe { sys::mln_frame_demand_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_free_camera_options_default`.
pub fn free_camera_options_default() -> Result<FreeCameraOptions> {
    let mut call = Call::global("mln_free_camera_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_free_camera_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_geojson_source_data_create`.
pub fn geojson_source_data_create(
    data: &[u8],
    options: Option<&GeojsonSourceOptions>,
) -> Result<GeojsonSourceDataHandle> {
    let mut call = Call::global("mln_geojson_source_data_create")?;
    maplibre_core::validate_abi_version()?;
    let mut out_data = sys::mln_geojson_source_data(0);
    let data = call.input(&data)?;
    let options = call.optional_reference(options.as_ref())?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_geojson_source_data_create(data, options, &mut out_data, out_diagnostic)
    })?;
    Ok(GeojsonSourceDataHandle::adopt(out_data, None)?)
}

/// Calls `mln_geojson_source_options_default`.
pub fn geojson_source_options_default() -> Result<GeojsonSourceOptions> {
    let mut call = Call::global("mln_geojson_source_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_geojson_source_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_gpu_sync_default`.
pub fn gpu_sync_default() -> Result<GpuSync> {
    let mut call = Call::global("mln_gpu_sync_default")?;
    let value = call.run(|_| unsafe { sys::mln_gpu_sync_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_lat_lng_for_projected_meters`.
pub fn lat_lng_for_projected_meters(meters: ProjectedMeters) -> Result<LatLng> {
    let mut call = Call::global("mln_lat_lng_for_projected_meters")?;
    let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
    let meters = call.input(&meters)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_lat_lng_for_projected_meters(meters, &mut out_coordinate, out_diagnostic)
    })?;
    Ok(unsafe { from_native(out_coordinate) }?)
}

/// Calls `mln_log_clear_callback`.
pub fn log_clear_callback() -> Result<()> {
    let mut call = Call::global("mln_log_clear_callback")?;
    call.status(|_, out_diagnostic| unsafe { sys::mln_log_clear_callback(out_diagnostic) })?;
    Ok(())
}

/// Calls `mln_log_set_async_severity_mask`.
pub fn log_set_async_severity_mask(mask: LogSeverityMask) -> Result<()> {
    let mut call = Call::global("mln_log_set_async_severity_mask")?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_log_set_async_severity_mask(mask.to_native(), out_diagnostic)
    })?;
    Ok(())
}

/// Calls `mln_log_set_callback`.
pub fn log_set_callback(callback_: Option<LogCallback>) -> Result<()> {
    let mut call = Call::global("mln_log_set_callback")?;
    let (callback_, user_data, release_user_data) =
        log_callback_registration(callback_, call.arena());
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_log_set_callback(callback_, user_data, release_user_data, out_diagnostic)
    })?;
    Ok(())
}

/// Calls `mln_map_options_default`.
pub fn map_options_default() -> Result<MapOptions> {
    let mut call = Call::global("mln_map_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_map_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_map_tile_options_default`.
pub fn map_tile_options_default() -> Result<MapTileOptions> {
    let mut call = Call::global("mln_map_tile_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_map_tile_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_map_viewport_options_default`.
pub fn map_viewport_options_default() -> Result<MapViewportOptions> {
    let mut call = Call::global("mln_map_viewport_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_map_viewport_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_metal_borrowed_texture_descriptor_default`.
pub fn metal_borrowed_texture_descriptor_default() -> Result<MetalBorrowedTextureDescriptor> {
    let mut call = Call::global("mln_metal_borrowed_texture_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_metal_borrowed_texture_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_metal_owned_texture_descriptor_default`.
pub fn metal_owned_texture_descriptor_default() -> Result<MetalOwnedTextureDescriptor> {
    let mut call = Call::global("mln_metal_owned_texture_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_metal_owned_texture_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_metal_surface_descriptor_default`.
pub fn metal_surface_descriptor_default() -> Result<MetalSurfaceDescriptor> {
    let mut call = Call::global("mln_metal_surface_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_metal_surface_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_network_status_get`.
pub fn network_status_get() -> Result<NetworkStatus> {
    let mut call = Call::global("mln_network_status_get")?;
    let mut out_status: u32 = Default::default();
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_network_status_get(&mut out_status, out_diagnostic)
    })?;
    Ok(unsafe { from_native(out_status) }?)
}

/// Calls `mln_network_status_set`.
pub fn network_status_set(status: NetworkStatus) -> Result<()> {
    let mut call = Call::global("mln_network_status_set")?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_network_status_set(status.to_native(), out_diagnostic)
    })?;
    Ok(())
}

/// Calls `mln_opengl_borrowed_texture_descriptor_default`.
pub fn opengl_borrowed_texture_descriptor_default() -> Result<OpenglBorrowedTextureDescriptor> {
    let mut call = Call::global("mln_opengl_borrowed_texture_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_opengl_owned_texture_descriptor_default`.
pub fn opengl_owned_texture_descriptor_default() -> Result<OpenglOwnedTextureDescriptor> {
    let mut call = Call::global("mln_opengl_owned_texture_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_opengl_owned_texture_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_opengl_supported_context_provider_mask`.
pub fn opengl_supported_context_provider_mask() -> Result<OpenglContextProviderFlag> {
    let mut call = Call::global("mln_opengl_supported_context_provider_mask")?;
    let value = call.run(|_| unsafe { sys::mln_opengl_supported_context_provider_mask() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_opengl_surface_descriptor_default`.
pub fn opengl_surface_descriptor_default() -> Result<OpenglSurfaceDescriptor> {
    let mut call = Call::global("mln_opengl_surface_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_opengl_surface_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_plugin_get_register_function_v1`.
pub fn plugin_get_register_function_v1() -> Result<sys::mln_plugin_register_function_v1> {
    let mut call = Call::global("mln_plugin_get_register_function_v1")?;
    let value = call.run(|_| unsafe { sys::mln_plugin_get_register_function_v1() });
    Ok(value)
}

/// Calls `mln_premultiplied_rgba8_image_default`.
pub fn premultiplied_rgba8_image_default() -> Result<PremultipliedRgba8Image> {
    let mut call = Call::global("mln_premultiplied_rgba8_image_default")?;
    let value = call.run(|_| unsafe { sys::mln_premultiplied_rgba8_image_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_projected_meters_for_lat_lng`.
pub fn projected_meters_for_lat_lng(coordinate: LatLng) -> Result<ProjectedMeters> {
    let mut call = Call::global("mln_projected_meters_for_lat_lng")?;
    let mut out_meters: sys::mln_projected_meters = unsafe { std::mem::zeroed() };
    let coordinate = call.input(&coordinate)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_projected_meters_for_lat_lng(coordinate, &mut out_meters, out_diagnostic)
    })?;
    Ok(unsafe { from_native(out_meters) }?)
}

/// Calls `mln_projection_mode_default`.
pub fn projection_mode_default() -> Result<ProjectionMode> {
    let mut call = Call::global("mln_projection_mode_default")?;
    let value = call.run(|_| unsafe { sys::mln_projection_mode_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_render_session_attach_options_default`.
pub fn render_session_attach_options_default() -> Result<RenderSessionAttachOptions> {
    let mut call = Call::global("mln_render_session_attach_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_render_session_attach_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_render_target_extent_physical_size`.
pub fn render_target_extent_physical_size(extent: &RenderTargetExtent) -> Result<(u32, u32)> {
    let mut call = Call::global("mln_render_target_extent_physical_size")?;
    let mut out_width: u32 = Default::default();
    let mut out_height: u32 = Default::default();
    let extent = call.reference(&extent)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_render_target_extent_physical_size(
            extent,
            &mut out_width,
            &mut out_height,
            out_diagnostic,
        )
    })?;
    Ok((out_width, out_height))
}

/// Calls `mln_rendered_feature_query_options_default`.
pub fn rendered_feature_query_options_default() -> Result<RenderedFeatureQueryOptions> {
    let mut call = Call::global("mln_rendered_feature_query_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_rendered_feature_query_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_rendered_query_geometry_box`.
pub fn rendered_query_geometry_box(r#box: ScreenBox) -> Result<RenderedQueryGeometry> {
    let mut call = Call::global("mln_rendered_query_geometry_box")?;
    let r#box = call.input(&r#box)?;
    let value = call.run(|_| unsafe { sys::mln_rendered_query_geometry_box(r#box) });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_rendered_query_geometry_line_string`.
pub fn rendered_query_geometry_line_string(
    points: &[ScreenPoint],
) -> Result<RenderedQueryGeometry> {
    let mut call = Call::global("mln_rendered_query_geometry_line_string")?;
    let point_count = convert::count(points.len())?;
    let points = call.array(points)?;
    let value =
        call.run(|_| unsafe { sys::mln_rendered_query_geometry_line_string(points, point_count) });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_rendered_query_geometry_point`.
pub fn rendered_query_geometry_point(point: ScreenPoint) -> Result<RenderedQueryGeometry> {
    let mut call = Call::global("mln_rendered_query_geometry_point")?;
    let point = call.input(&point)?;
    let value = call.run(|_| unsafe { sys::mln_rendered_query_geometry_point(point) });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_runtime_create`.
pub fn runtime_create(options: &RuntimeOptions) -> Result<RuntimeHandle> {
    let mut call = Call::global("mln_runtime_create")?;
    maplibre_core::validate_abi_version()?;
    let mut out_runtime = sys::mln_runtime(0);
    let options = call.reference(&options)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_runtime_create(options, &mut out_runtime, out_diagnostic)
    })?;
    Ok(RuntimeHandle::adopt(out_runtime, None)?)
}

/// Calls `mln_runtime_options_default`.
pub fn runtime_options_default() -> Result<RuntimeOptions> {
    let mut call = Call::global("mln_runtime_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_runtime_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_source_feature_query_options_default`.
pub fn source_feature_query_options_default() -> Result<SourceFeatureQueryOptions> {
    let mut call = Call::global("mln_source_feature_query_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_source_feature_query_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_style_image_info_default`.
pub fn style_image_info_default() -> Result<StyleImageInfo> {
    let mut call = Call::global("mln_style_image_info_default")?;
    let value = call.run(|_| unsafe { sys::mln_style_image_info_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_style_image_options_default`.
pub fn style_image_options_default() -> Result<StyleImageOptions> {
    let mut call = Call::global("mln_style_image_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_style_image_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_style_tile_source_options_default`.
pub fn style_tile_source_options_default() -> Result<StyleTileSourceOptions> {
    let mut call = Call::global("mln_style_tile_source_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_style_tile_source_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_style_transition_options_default`.
pub fn style_transition_options_default() -> Result<StyleTransitionOptions> {
    let mut call = Call::global("mln_style_transition_options_default")?;
    let value = call.run(|_| unsafe { sys::mln_style_transition_options_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_supported_render_backend_mask`.
pub fn supported_render_backend_mask() -> Result<RenderBackendFlag> {
    let mut call = Call::global("mln_supported_render_backend_mask")?;
    let value = call.run(|_| unsafe { sys::mln_supported_render_backend_mask() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_texture_image_info_default`.
pub fn texture_image_info_default() -> Result<TextureImageInfo> {
    let mut call = Call::global("mln_texture_image_info_default")?;
    let value = call.run(|_| unsafe { sys::mln_texture_image_info_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_vulkan_borrowed_texture_descriptor_default`.
pub fn vulkan_borrowed_texture_descriptor_default() -> Result<VulkanBorrowedTextureDescriptor> {
    let mut call = Call::global("mln_vulkan_borrowed_texture_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_vulkan_owned_texture_descriptor_default`.
pub fn vulkan_owned_texture_descriptor_default() -> Result<VulkanOwnedTextureDescriptor> {
    let mut call = Call::global("mln_vulkan_owned_texture_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_vulkan_owned_texture_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_vulkan_surface_descriptor_default`.
pub fn vulkan_surface_descriptor_default() -> Result<VulkanSurfaceDescriptor> {
    let mut call = Call::global("mln_vulkan_surface_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_vulkan_surface_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_webgpu_borrowed_texture_descriptor_default`.
pub fn webgpu_borrowed_texture_descriptor_default() -> Result<WebgpuBorrowedTextureDescriptor> {
    let mut call = Call::global("mln_webgpu_borrowed_texture_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_webgpu_owned_texture_descriptor_default`.
pub fn webgpu_owned_texture_descriptor_default() -> Result<WebgpuOwnedTextureDescriptor> {
    let mut call = Call::global("mln_webgpu_owned_texture_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_webgpu_owned_texture_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}

/// Calls `mln_webgpu_surface_descriptor_default`.
pub fn webgpu_surface_descriptor_default() -> Result<WebgpuSurfaceDescriptor> {
    let mut call = Call::global("mln_webgpu_surface_descriptor_default")?;
    let value = call.run(|_| unsafe { sys::mln_webgpu_surface_descriptor_default() });
    Ok(unsafe { from_native(value) }?)
}
