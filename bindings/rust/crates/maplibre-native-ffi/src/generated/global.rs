// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

/// # Safety
/// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
/// Calls `mln_android_init` using its header execution and ownership contract.
pub unsafe fn android_init(
    binding_arg_0: *mut std::ffi::c_void,
    binding_arg_1: *mut std::ffi::c_void,
    binding_arg_2: *mut std::ffi::c_void,
) -> Result<()> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_android_init", 0)?;
    maplibre_core::check(unsafe {
        sys::mln_android_init(binding_arg_0, binding_arg_1, binding_arg_2)
    })?;
    Ok(())
}

/// Calls `mln_animation_options_default` using its header execution and ownership contract.
pub fn animation_options_default() -> Result<maplibre_core::generated::AnimationOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_animation_options_default", 0)?;
    let value = unsafe { sys::mln_animation_options_default() };
    Ok(maplibre_core::generated::AnimationOptions::from_native(
        value,
    ))
}

/// Calls `mln_bound_options_default` using its header execution and ownership contract.
pub fn bound_options_default() -> Result<maplibre_core::generated::BoundOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_bound_options_default", 0)?;
    let value = unsafe { sys::mln_bound_options_default() };
    Ok(maplibre_core::generated::BoundOptions::from_native(value))
}

/// Calls `mln_c_version` using its header execution and ownership contract.
pub fn c_version() -> Result<u32> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_c_version", 0)?;
    let value = unsafe { sys::mln_c_version() };
    Ok(value)
}

/// Calls `mln_camera_delta_default` using its header execution and ownership contract.
pub fn camera_delta_default() -> Result<maplibre_core::generated::CameraDelta> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_camera_delta_default", 0)?;
    let value = unsafe { sys::mln_camera_delta_default() };
    Ok(maplibre_core::generated::CameraDelta::from_native(value))
}

/// Calls `mln_camera_fit_options_default` using its header execution and ownership contract.
pub fn camera_fit_options_default() -> Result<maplibre_core::generated::CameraFitOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_camera_fit_options_default", 0)?;
    let value = unsafe { sys::mln_camera_fit_options_default() };
    Ok(maplibre_core::generated::CameraFitOptions::from_native(
        value,
    ))
}

/// Calls `mln_camera_options_default` using its header execution and ownership contract.
pub fn camera_options_default() -> Result<maplibre_core::generated::CameraOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_camera_options_default", 0)?;
    let value = unsafe { sys::mln_camera_options_default() };
    Ok(maplibre_core::generated::CameraOptions::from_native(value))
}

/// Calls `mln_camera_update_default` using its header execution and ownership contract.
pub fn camera_update_default() -> Result<maplibre_core::generated::CameraUpdate> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_camera_update_default", 0)?;
    let value = unsafe { sys::mln_camera_update_default() };
    Ok(maplibre_core::generated::CameraUpdate::from_native(value))
}

/// Calls `mln_custom_geometry_source_options_default` using its header execution and ownership contract.
pub fn custom_geometry_source_options_default()
-> Result<maplibre_core::generated::CustomGeometrySourceOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_custom_geometry_source_options_default", 0)?;
    let value = unsafe { sys::mln_custom_geometry_source_options_default() };
    Ok(unsafe { maplibre_core::generated::CustomGeometrySourceOptions::from_native(value) }?)
}

/// Calls `mln_custom_mvt_vector_source_options_default` using its header execution and ownership contract.
pub fn custom_mvt_vector_source_options_default()
-> Result<maplibre_core::generated::CustomMvtVectorSourceOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_custom_mvt_vector_source_options_default", 0)?;
    let value = unsafe { sys::mln_custom_mvt_vector_source_options_default() };
    Ok(unsafe { maplibre_core::generated::CustomMvtVectorSourceOptions::from_native(value) }?)
}

/// Calls `mln_frame_demand_default` using its header execution and ownership contract.
pub fn frame_demand_default() -> Result<maplibre_core::generated::FrameDemand> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_frame_demand_default", 0)?;
    let value = unsafe { sys::mln_frame_demand_default() };
    Ok(maplibre_core::generated::FrameDemand::from_native(value))
}

/// Calls `mln_free_camera_options_default` using its header execution and ownership contract.
pub fn free_camera_options_default() -> Result<maplibre_core::generated::FreeCameraOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_free_camera_options_default", 0)?;
    let value = unsafe { sys::mln_free_camera_options_default() };
    Ok(maplibre_core::generated::FreeCameraOptions::from_native(
        value,
    ))
}

/// Calls `mln_geojson_source_data_create` using its header execution and ownership contract.
pub fn geojson_source_data_create(
    binding_arg_0: &[u8],
    binding_arg_1: Option<&maplibre_core::generated::GeojsonSourceOptions>,
) -> Result<crate::GeojsonSourceDataHandle> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_geojson_source_data_create", 0)?;
    maplibre_core::validate_abi_version()?;
    let mut arena = maplibre_core::input::InputArena::default();
    let binding_arg_0 = maplibre_native_ffi_sys::mln_buffer_view {
        data: (binding_arg_0).as_ptr().cast(),
        size: (binding_arg_0).len(),
    };
    let binding_arg_1 = binding_arg_1
        .map(|value| value.to_native(&mut arena))
        .transpose()?;
    let mut binding_arg_2 = sys::mln_geojson_source_data(0);
    maplibre_core::check(unsafe {
        sys::mln_geojson_source_data_create(
            binding_arg_0,
            binding_arg_1
                .as_ref()
                .map_or(std::ptr::null(), |value| value),
            &mut binding_arg_2,
        )
    })?;
    Ok(crate::GeojsonSourceDataHandle::from_native(binding_arg_2)?)
}

/// Calls `mln_geojson_source_options_default` using its header execution and ownership contract.
pub fn geojson_source_options_default() -> Result<maplibre_core::generated::GeojsonSourceOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_geojson_source_options_default", 0)?;
    let value = unsafe { sys::mln_geojson_source_options_default() };
    Ok(unsafe { maplibre_core::generated::GeojsonSourceOptions::from_native(value) }?)
}

/// Calls `mln_gpu_sync_default` using its header execution and ownership contract.
pub fn gpu_sync_default() -> Result<maplibre_core::generated::GpuSync> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_gpu_sync_default", 0)?;
    let value = unsafe { sys::mln_gpu_sync_default() };
    Ok(maplibre_core::generated::GpuSync::from_native(value))
}

/// Calls `mln_lat_lng_for_projected_meters` using its header execution and ownership contract.
pub fn lat_lng_for_projected_meters(
    binding_arg_0: maplibre_core::generated::ProjectedMeters,
) -> Result<maplibre_core::generated::LatLng> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_lat_lng_for_projected_meters", 0)?;
    let mut binding_arg_1: sys::mln_lat_lng =
        maplibre_core::generated::LatLng::default().to_native();
    maplibre_core::check(unsafe {
        sys::mln_lat_lng_for_projected_meters(binding_arg_0.to_native(), &mut binding_arg_1)
    })?;
    Ok(maplibre_core::generated::LatLng::from_native(binding_arg_1))
}

/// Calls `mln_log_clear_callback` using its header execution and ownership contract.
pub fn log_clear_callback() -> Result<()> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_log_clear_callback", 0)?;
    maplibre_core::check(unsafe { sys::mln_log_clear_callback() })?;
    Ok(())
}

/// Calls `mln_log_set_async_severity_mask` using its header execution and ownership contract.
pub fn log_set_async_severity_mask(
    binding_arg_0: maplibre_core::generated::LogSeverityMask,
) -> Result<()> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_log_set_async_severity_mask", 0)?;
    maplibre_core::check(unsafe {
        sys::mln_log_set_async_severity_mask(binding_arg_0.to_native())
    })?;
    Ok(())
}

/// Calls `mln_log_set_callback` using its header execution and ownership contract.
pub fn log_set_callback(
    binding_arg_0: Option<maplibre_core::generated::LogCallback>,
) -> Result<()> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_log_set_callback", 0)?;
    let mut arena = maplibre_core::input::InputArena::default();
    let binding_registration_0 =
        maplibre_core::generated::log_callback_registration(binding_arg_0, &mut arena);
    maplibre_core::check(unsafe {
        sys::mln_log_set_callback(
            binding_registration_0.0,
            binding_registration_0.1,
            binding_registration_0.2,
        )
    })?;
    arena.accept_registrations();
    Ok(())
}

/// Calls `mln_map_options_default` using its header execution and ownership contract.
pub fn map_options_default() -> Result<maplibre_core::generated::MapOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_map_options_default", 0)?;
    let value = unsafe { sys::mln_map_options_default() };
    Ok(maplibre_core::generated::MapOptions::from_native(value))
}

/// Calls `mln_map_tile_options_default` using its header execution and ownership contract.
pub fn map_tile_options_default() -> Result<maplibre_core::generated::MapTileOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_map_tile_options_default", 0)?;
    let value = unsafe { sys::mln_map_tile_options_default() };
    Ok(maplibre_core::generated::MapTileOptions::from_native(value))
}

/// Calls `mln_map_viewport_options_default` using its header execution and ownership contract.
pub fn map_viewport_options_default() -> Result<maplibre_core::generated::MapViewportOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_map_viewport_options_default", 0)?;
    let value = unsafe { sys::mln_map_viewport_options_default() };
    Ok(maplibre_core::generated::MapViewportOptions::from_native(
        value,
    ))
}

/// Calls `mln_metal_borrowed_texture_descriptor_default` using its header execution and ownership contract.
pub fn metal_borrowed_texture_descriptor_default()
-> Result<maplibre_core::generated::MetalBorrowedTextureDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_metal_borrowed_texture_descriptor_default", 0)?;
    let value = unsafe { sys::mln_metal_borrowed_texture_descriptor_default() };
    Ok(maplibre_core::generated::MetalBorrowedTextureDescriptor::from_native(value))
}

/// Calls `mln_metal_owned_texture_descriptor_default` using its header execution and ownership contract.
pub fn metal_owned_texture_descriptor_default()
-> Result<maplibre_core::generated::MetalOwnedTextureDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_metal_owned_texture_descriptor_default", 0)?;
    let value = unsafe { sys::mln_metal_owned_texture_descriptor_default() };
    Ok(maplibre_core::generated::MetalOwnedTextureDescriptor::from_native(value))
}

/// Calls `mln_metal_surface_descriptor_default` using its header execution and ownership contract.
pub fn metal_surface_descriptor_default() -> Result<maplibre_core::generated::MetalSurfaceDescriptor>
{
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_metal_surface_descriptor_default", 0)?;
    let value = unsafe { sys::mln_metal_surface_descriptor_default() };
    Ok(maplibre_core::generated::MetalSurfaceDescriptor::from_native(value))
}

/// Calls `mln_network_status_get` using its header execution and ownership contract.
pub fn network_status_get() -> Result<maplibre_core::generated::NetworkStatus> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_network_status_get", 0)?;
    let mut binding_arg_0: u32 = Default::default();
    maplibre_core::check(unsafe { sys::mln_network_status_get(&mut binding_arg_0) })?;
    Ok(maplibre_core::generated::NetworkStatus::from_native(
        binding_arg_0,
    ))
}

/// Calls `mln_network_status_set` using its header execution and ownership contract.
pub fn network_status_set(binding_arg_0: maplibre_core::generated::NetworkStatus) -> Result<()> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_network_status_set", 0)?;
    maplibre_core::check(unsafe { sys::mln_network_status_set(binding_arg_0.to_native()) })?;
    Ok(())
}

/// Calls `mln_opengl_borrowed_texture_descriptor_default` using its header execution and ownership contract.
pub fn opengl_borrowed_texture_descriptor_default()
-> Result<maplibre_core::generated::OpenglBorrowedTextureDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_opengl_borrowed_texture_descriptor_default", 0)?;
    let value = unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() };
    Ok(unsafe { maplibre_core::generated::OpenglBorrowedTextureDescriptor::from_native(value) }?)
}

/// Calls `mln_opengl_owned_texture_descriptor_default` using its header execution and ownership contract.
pub fn opengl_owned_texture_descriptor_default()
-> Result<maplibre_core::generated::OpenglOwnedTextureDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_opengl_owned_texture_descriptor_default", 0)?;
    let value = unsafe { sys::mln_opengl_owned_texture_descriptor_default() };
    Ok(unsafe { maplibre_core::generated::OpenglOwnedTextureDescriptor::from_native(value) }?)
}

/// Calls `mln_opengl_supported_context_provider_mask` using its header execution and ownership contract.
pub fn opengl_supported_context_provider_mask()
-> Result<maplibre_core::generated::OpenglContextProviderFlag> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_opengl_supported_context_provider_mask", 0)?;
    let value = unsafe { sys::mln_opengl_supported_context_provider_mask() };
    Ok(maplibre_core::generated::OpenglContextProviderFlag::from_native(value))
}

/// Calls `mln_opengl_surface_descriptor_default` using its header execution and ownership contract.
pub fn opengl_surface_descriptor_default()
-> Result<maplibre_core::generated::OpenglSurfaceDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_opengl_surface_descriptor_default", 0)?;
    let value = unsafe { sys::mln_opengl_surface_descriptor_default() };
    Ok(unsafe { maplibre_core::generated::OpenglSurfaceDescriptor::from_native(value) }?)
}

/// Calls `mln_plugin_get_register_function_v1` using its header execution and ownership contract.
pub fn plugin_get_register_function_v1()
-> Result<maplibre_native_ffi_sys::mln_plugin_register_function_v1> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_plugin_get_register_function_v1", 0)?;
    let value = unsafe { sys::mln_plugin_get_register_function_v1() };
    Ok(value)
}

/// Calls `mln_premultiplied_rgba8_image_default` using its header execution and ownership contract.
pub fn premultiplied_rgba8_image_default()
-> Result<maplibre_core::generated::PremultipliedRgba8Image> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_premultiplied_rgba8_image_default", 0)?;
    let value = unsafe { sys::mln_premultiplied_rgba8_image_default() };
    Ok(unsafe { maplibre_core::generated::PremultipliedRgba8Image::from_native(value) }?)
}

/// Calls `mln_projected_meters_for_lat_lng` using its header execution and ownership contract.
pub fn projected_meters_for_lat_lng(
    binding_arg_0: maplibre_core::generated::LatLng,
) -> Result<maplibre_core::generated::ProjectedMeters> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_projected_meters_for_lat_lng", 0)?;
    let mut binding_arg_1: sys::mln_projected_meters =
        maplibre_core::generated::ProjectedMeters::default().to_native();
    maplibre_core::check(unsafe {
        sys::mln_projected_meters_for_lat_lng(binding_arg_0.to_native(), &mut binding_arg_1)
    })?;
    Ok(maplibre_core::generated::ProjectedMeters::from_native(
        binding_arg_1,
    ))
}

/// Calls `mln_projection_mode_default` using its header execution and ownership contract.
pub fn projection_mode_default() -> Result<maplibre_core::generated::ProjectionMode> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_projection_mode_default", 0)?;
    let value = unsafe { sys::mln_projection_mode_default() };
    Ok(maplibre_core::generated::ProjectionMode::from_native(value))
}

/// Calls `mln_render_session_attach_options_default` using its header execution and ownership contract.
pub fn render_session_attach_options_default()
-> Result<maplibre_core::generated::RenderSessionAttachOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_render_session_attach_options_default", 0)?;
    let value = unsafe { sys::mln_render_session_attach_options_default() };
    Ok(unsafe { maplibre_core::generated::RenderSessionAttachOptions::from_native(value) }?)
}

/// Calls `mln_render_target_extent_physical_size` using its header execution and ownership contract.
pub fn render_target_extent_physical_size(
    binding_arg_0: &maplibre_core::generated::RenderTargetExtent,
) -> Result<(u32, u32)> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_render_target_extent_physical_size", 0)?;
    let binding_arg_0 = binding_arg_0.to_native();
    let mut binding_arg_1: u32 = Default::default();
    let mut binding_arg_2: u32 = Default::default();
    maplibre_core::check(unsafe {
        sys::mln_render_target_extent_physical_size(
            &binding_arg_0,
            &mut binding_arg_1,
            &mut binding_arg_2,
        )
    })?;
    Ok((binding_arg_1, binding_arg_2))
}

/// Calls `mln_rendered_feature_query_options_default` using its header execution and ownership contract.
pub fn rendered_feature_query_options_default()
-> Result<maplibre_core::generated::RenderedFeatureQueryOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_rendered_feature_query_options_default", 0)?;
    let value = unsafe { sys::mln_rendered_feature_query_options_default() };
    Ok(unsafe { maplibre_core::generated::RenderedFeatureQueryOptions::from_native(value) }?)
}

/// Calls `mln_rendered_query_geometry_box` using its header execution and ownership contract.
pub fn rendered_query_geometry_box(
    binding_arg_0: maplibre_core::generated::ScreenBox,
) -> Result<maplibre_core::generated::RenderedQueryGeometry> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_rendered_query_geometry_box", 0)?;
    let value = unsafe { sys::mln_rendered_query_geometry_box(binding_arg_0.to_native()) };
    Ok(unsafe { maplibre_core::generated::RenderedQueryGeometry::from_native(value) }?)
}

/// Calls `mln_rendered_query_geometry_line_string` using its header execution and ownership contract.
pub fn rendered_query_geometry_line_string(
    binding_arg_0: &[maplibre_core::generated::ScreenPoint],
) -> Result<maplibre_core::generated::RenderedQueryGeometry> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_rendered_query_geometry_line_string", 0)?;
    let binding_arg_0: Vec<_> = binding_arg_0
        .iter()
        .map(|value| -> Result<_> { Ok((value).to_native()) })
        .collect::<Result<_>>()?;
    let binding_arg_1 = binding_arg_0
        .len()
        .try_into()
        .map_err(|_| crate::Error::invalid_argument("input exceeds native count range"))?;
    let value = unsafe {
        sys::mln_rendered_query_geometry_line_string(binding_arg_0.as_ptr(), binding_arg_1)
    };
    Ok(unsafe { maplibre_core::generated::RenderedQueryGeometry::from_native(value) }?)
}

/// Calls `mln_rendered_query_geometry_point` using its header execution and ownership contract.
pub fn rendered_query_geometry_point(
    binding_arg_0: maplibre_core::generated::ScreenPoint,
) -> Result<maplibre_core::generated::RenderedQueryGeometry> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_rendered_query_geometry_point", 0)?;
    let value = unsafe { sys::mln_rendered_query_geometry_point(binding_arg_0.to_native()) };
    Ok(unsafe { maplibre_core::generated::RenderedQueryGeometry::from_native(value) }?)
}

/// Calls `mln_runtime_create` using its header execution and ownership contract.
pub fn runtime_create(
    binding_arg_0: &maplibre_core::generated::RuntimeOptions,
) -> Result<crate::RuntimeHandle> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_runtime_create", 0)?;
    maplibre_core::validate_abi_version()?;
    let mut arena = maplibre_core::input::InputArena::default();
    let binding_arg_0 = binding_arg_0.to_native(&mut arena)?;
    let mut binding_arg_1 = sys::mln_runtime(0);
    maplibre_core::check(unsafe { sys::mln_runtime_create(&binding_arg_0, &mut binding_arg_1) })?;
    arena.accept_registrations();
    Ok(crate::RuntimeHandle::from_native(binding_arg_1)?)
}

/// Calls `mln_runtime_options_default` using its header execution and ownership contract.
pub fn runtime_options_default() -> Result<maplibre_core::generated::RuntimeOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_runtime_options_default", 0)?;
    let value = unsafe { sys::mln_runtime_options_default() };
    Ok(unsafe { maplibre_core::generated::RuntimeOptions::from_native(value) }?)
}

/// Calls `mln_source_feature_query_options_default` using its header execution and ownership contract.
pub fn source_feature_query_options_default()
-> Result<maplibre_core::generated::SourceFeatureQueryOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_source_feature_query_options_default", 0)?;
    let value = unsafe { sys::mln_source_feature_query_options_default() };
    Ok(unsafe { maplibre_core::generated::SourceFeatureQueryOptions::from_native(value) }?)
}

/// Calls `mln_style_image_info_default` using its header execution and ownership contract.
pub fn style_image_info_default() -> Result<maplibre_core::generated::StyleImageInfo> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_style_image_info_default", 0)?;
    let value = unsafe { sys::mln_style_image_info_default() };
    Ok(maplibre_core::generated::StyleImageInfo::from_native(value))
}

/// Calls `mln_style_image_options_default` using its header execution and ownership contract.
pub fn style_image_options_default() -> Result<maplibre_core::generated::StyleImageOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_style_image_options_default", 0)?;
    let value = unsafe { sys::mln_style_image_options_default() };
    Ok(unsafe { maplibre_core::generated::StyleImageOptions::from_native(value) }?)
}

/// Calls `mln_style_tile_source_options_default` using its header execution and ownership contract.
pub fn style_tile_source_options_default()
-> Result<maplibre_core::generated::StyleTileSourceOptions> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_style_tile_source_options_default", 0)?;
    let value = unsafe { sys::mln_style_tile_source_options_default() };
    Ok(unsafe { maplibre_core::generated::StyleTileSourceOptions::from_native(value) }?)
}

/// Calls `mln_style_transition_options_default` using its header execution and ownership contract.
pub fn style_transition_options_default() -> Result<maplibre_core::generated::StyleTransitionOptions>
{
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_style_transition_options_default", 0)?;
    let value = unsafe { sys::mln_style_transition_options_default() };
    Ok(maplibre_core::generated::StyleTransitionOptions::from_native(value))
}

/// Calls `mln_supported_render_backend_mask` using its header execution and ownership contract.
pub fn supported_render_backend_mask() -> Result<maplibre_core::generated::RenderBackendFlag> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_supported_render_backend_mask", 0)?;
    let value = unsafe { sys::mln_supported_render_backend_mask() };
    Ok(maplibre_core::generated::RenderBackendFlag::from_native(
        value,
    ))
}

/// Calls `mln_texture_image_info_default` using its header execution and ownership contract.
pub fn texture_image_info_default() -> Result<maplibre_core::generated::TextureImageInfo> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_texture_image_info_default", 0)?;
    let value = unsafe { sys::mln_texture_image_info_default() };
    Ok(maplibre_core::generated::TextureImageInfo::from_native(
        value,
    ))
}

/// Calls `mln_thread_last_error_message` using its header execution and ownership contract.
pub fn thread_last_error_message() -> Result<String> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_thread_last_error_message", 0)?;
    let value = unsafe { sys::mln_thread_last_error_message() };
    Ok(unsafe { maplibre_core::string::copy_c_string(value) }?)
}

/// Calls `mln_vulkan_borrowed_texture_descriptor_default` using its header execution and ownership contract.
pub fn vulkan_borrowed_texture_descriptor_default()
-> Result<maplibre_core::generated::VulkanBorrowedTextureDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_vulkan_borrowed_texture_descriptor_default", 0)?;
    let value = unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() };
    Ok(maplibre_core::generated::VulkanBorrowedTextureDescriptor::from_native(value))
}

/// Calls `mln_vulkan_owned_texture_descriptor_default` using its header execution and ownership contract.
pub fn vulkan_owned_texture_descriptor_default()
-> Result<maplibre_core::generated::VulkanOwnedTextureDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_vulkan_owned_texture_descriptor_default", 0)?;
    let value = unsafe { sys::mln_vulkan_owned_texture_descriptor_default() };
    Ok(maplibre_core::generated::VulkanOwnedTextureDescriptor::from_native(value))
}

/// Calls `mln_vulkan_surface_descriptor_default` using its header execution and ownership contract.
pub fn vulkan_surface_descriptor_default()
-> Result<maplibre_core::generated::VulkanSurfaceDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_vulkan_surface_descriptor_default", 0)?;
    let value = unsafe { sys::mln_vulkan_surface_descriptor_default() };
    Ok(maplibre_core::generated::VulkanSurfaceDescriptor::from_native(value))
}

/// Calls `mln_webgpu_borrowed_texture_descriptor_default` using its header execution and ownership contract.
pub fn webgpu_borrowed_texture_descriptor_default()
-> Result<maplibre_core::generated::WebgpuBorrowedTextureDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_webgpu_borrowed_texture_descriptor_default", 0)?;
    let value = unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() };
    Ok(maplibre_core::generated::WebgpuBorrowedTextureDescriptor::from_native(value))
}

/// Calls `mln_webgpu_owned_texture_descriptor_default` using its header execution and ownership contract.
pub fn webgpu_owned_texture_descriptor_default()
-> Result<maplibre_core::generated::WebgpuOwnedTextureDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_webgpu_owned_texture_descriptor_default", 0)?;
    let value = unsafe { sys::mln_webgpu_owned_texture_descriptor_default() };
    Ok(maplibre_core::generated::WebgpuOwnedTextureDescriptor::from_native(value))
}

/// Calls `mln_webgpu_surface_descriptor_default` using its header execution and ownership contract.
pub fn webgpu_surface_descriptor_default()
-> Result<maplibre_core::generated::WebgpuSurfaceDescriptor> {
    // SAFETY: input storage lives through submission; callback values are copied before return.
    maplibre_core::callback::check("mln_webgpu_surface_descriptor_default", 0)?;
    let value = unsafe { sys::mln_webgpu_surface_descriptor_default() };
    Ok(maplibre_core::generated::WebgpuSurfaceDescriptor::from_native(value))
}
