// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

/// Initializes Android platform services.
///
/// See `mln_android_init` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
///
/// # Safety
/// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
#[cfg(target_os = "android")]
pub unsafe fn android_init(
    jni_env: *mut std::ffi::c_void,
    context: *mut std::ffi::c_void,
) -> Result<()> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_android_init(jni_env, context, out_diagnostic)
    })?;
    Ok(())
}

/// Returns empty animation options initialized for this C API version.
///
/// See `mln_animation_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn animation_options_default() -> AnimationOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_animation_options_default() });
    convert::native_default(value)
}

/// Returns empty map bound options initialized for this C API version.
///
/// See `mln_bound_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn bound_options_default() -> BoundOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_bound_options_default() });
    convert::native_default(value)
}

/// Reports the C ABI contract version. The value is 0 while the ABI is
/// unstable, and will increment on each SemVer major release.
///
/// See `mln_c_version` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
pub fn c_version() -> u32 {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_c_version() });
    value
}

/// Returns an empty relative camera update initialized for this API
/// version.
///
/// See `mln_camera_delta_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn camera_delta_default() -> CameraDelta {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_camera_delta_default() });
    convert::native_default(value)
}

/// Returns empty camera fitting options initialized for this C API version.
///
/// See `mln_camera_fit_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn camera_fit_options_default() -> CameraFitOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_camera_fit_options_default() });
    convert::native_default(value)
}

/// Returns empty camera options initialized for this C API version.
///
/// See `mln_camera_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn camera_options_default() -> CameraOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_camera_options_default() });
    convert::native_default(value)
}

/// Returns an empty atomic camera update initialized for this API version.
///
/// See `mln_camera_update_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn camera_update_default() -> CameraUpdate {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_camera_update_default() });
    convert::native_default(value)
}

/// Returns default custom geometry source options.
///
/// See `mln_custom_geometry_source_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub fn custom_geometry_source_options_default() -> CustomGeometrySourceOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_custom_geometry_source_options_default() });
    convert::native_default(value)
}

/// Returns default custom MVT vector source options.
///
/// See `mln_custom_mvt_vector_source_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub fn custom_mvt_vector_source_options_default() -> CustomMvtVectorSourceOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_custom_mvt_vector_source_options_default() });
    convert::native_default(value)
}

/// Returns a zero-token, render-if-needed, nonpresenting frame demand.
///
/// See `mln_frame_demand_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
pub fn frame_demand_default() -> FrameDemand {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_frame_demand_default() });
    convert::native_default(value)
}

/// Returns empty free camera options initialized for this C API version.
///
/// See `mln_free_camera_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn free_camera_options_default() -> FreeCameraOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_free_camera_options_default() });
    convert::native_default(value)
}

/// Prepares GeoJSON source data for installation on a map.
///
/// See `mln_geojson_source_data_create` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub fn geojson_source_data_create(
    data: &str,
    options: Option<&GeojsonSourceOptions>,
) -> Result<GeojsonSourceDataHandle> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let mut out_data = sys::mln_geojson_source_data(0);
    let data = call.input(&data)?;
    let options = call.optional_reference(options.as_ref())?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_geojson_source_data_create(data, options, &mut out_data, out_diagnostic)
    })?;
    Ok(GeojsonSourceDataHandle::adopt(out_data, None)?)
}

/// Returns default GeoJSON source options.
///
/// See `mln_geojson_source_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub fn geojson_source_options_default() -> GeojsonSourceOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_geojson_source_options_default() });
    convert::native_default(value)
}

/// Returns CPU-complete synchronization for this C API version.
///
/// See `mln_gpu_sync_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub fn gpu_sync_default() -> GpuSync {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_gpu_sync_default() });
    convert::native_default(value)
}

/// Returns an exact-match header rule for every resource kind, with no
/// headers.
///
/// See `mln_http_header_rule_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub fn http_header_rule_default() -> HttpHeaderRule {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_http_header_rule_default() });
    convert::native_default(value)
}

/// Converts spherical Mercator projected meters to a geographic coordinate.
///
/// See `mln_lat_lng_for_projected_meters` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
pub fn lat_lng_for_projected_meters(meters: ProjectedMeters) -> Result<LatLng> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
    let meters = call.input(&meters)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_lat_lng_for_projected_meters(meters, &mut out_coordinate, out_diagnostic)
    })?;
    Ok(unsafe { from_native(out_coordinate) }?)
}

/// Clears the process-global log callback.
///
/// See `mln_log_clear_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub fn log_clear_callback() -> Result<()> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    call.status(|_, out_diagnostic| unsafe { sys::mln_log_clear_callback(out_diagnostic) })?;
    Ok(())
}

/// Controls which log severities MapLibre Native may dispatch
/// asynchronously.
///
/// See `mln_log_set_async_severity_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub fn log_set_async_severity_mask(mask: LogSeverityFlag) -> Result<()> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_log_set_async_severity_mask(mask.bits(), out_diagnostic)
    })?;
    Ok(())
}

/// Installs a process-global MapLibre Native log callback.
///
/// See `mln_log_set_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub fn log_set_callback(handler: LogHandler) -> Result<()> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let handler = call.reference(&handler)?;
    call.status(|_, out_diagnostic| unsafe { sys::mln_log_set_callback(handler, out_diagnostic) })?;
    Ok(())
}

/// Returns a 256 by 256 logical extent with one device pixel per UI pixel.
///
/// See `mln_logical_extent_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub fn logical_extent_default() -> LogicalExtent {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_logical_extent_default() });
    convert::native_default(value)
}

/// Computes the physical device-pixel size of a logical extent.
///
/// See `mln_logical_extent_physical_size` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub fn logical_extent_physical_size(extent: LogicalExtent) -> Result<(u32, u32)> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let mut out_width: u32 = Default::default();
    let mut out_height: u32 = Default::default();
    let extent = call.input(&extent)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_logical_extent_physical_size(
            extent,
            &mut out_width,
            &mut out_height,
            out_diagnostic,
        )
    })?;
    Ok((out_width, out_height))
}

/// Returns map options initialized for this C API version.
///
/// See `mln_map_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
pub fn map_options_default() -> MapOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_map_options_default() });
    convert::native_default(value)
}

/// Creates a standalone projection of a view, such as the view of a
/// rendered frame or of a map snapshot.
///
/// See `mln_map_projection_create` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
pub fn map_projection_create(view: &MapView) -> Result<MapProjectionHandle> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let mut out_projection = sys::mln_map_projection(0);
    let view = call.reference(&view)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_map_projection_create(view, &mut out_projection, out_diagnostic)
    })?;
    Ok(MapProjectionHandle::adopt(out_projection, None)?)
}

/// Returns empty tile tuning options initialized for this C API version.
///
/// See `mln_map_tile_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn map_tile_options_default() -> MapTileOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_map_tile_options_default() });
    convert::native_default(value)
}

/// Returns empty viewport options initialized for this C API version.
///
/// See `mln_map_viewport_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn map_viewport_options_default() -> MapViewportOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_map_viewport_options_default() });
    convert::native_default(value)
}

/// Returns Metal borrowed-texture descriptor defaults for this C API
/// version.
///
/// See `mln_metal_borrowed_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
pub fn metal_borrowed_texture_descriptor_default() -> MetalBorrowedTextureDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_metal_borrowed_texture_descriptor_default() });
    convert::native_default(value)
}

/// Returns Metal owned-texture descriptor defaults for this C API version.
///
/// See `mln_metal_owned_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
pub fn metal_owned_texture_descriptor_default() -> MetalOwnedTextureDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_metal_owned_texture_descriptor_default() });
    convert::native_default(value)
}

/// Returns Metal surface descriptor defaults for this C API version.
///
/// See `mln_metal_surface_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
pub fn metal_surface_descriptor_default() -> MetalSurfaceDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_metal_surface_descriptor_default() });
    convert::native_default(value)
}

/// Reads MapLibre Native's process-global network status.
///
/// See `mln_network_get_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub fn network_get_status() -> Result<NetworkStatus> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let mut out_status: sys::mln_network_status = Default::default();
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_network_get_status(&mut out_status, out_diagnostic)
    })?;
    Ok(NetworkStatus::from_raw(out_status))
}

/// Sets MapLibre Native's process-global network status.
///
/// See `mln_network_set_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub fn network_set_status(status: NetworkStatus) -> Result<()> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_network_set_status(status.as_raw(), out_diagnostic)
    })?;
    Ok(())
}

/// Returns OpenGL borrowed-texture descriptor defaults for this C API
/// version.
///
/// See `mln_opengl_borrowed_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
pub fn opengl_borrowed_texture_descriptor_default() -> OpenglBorrowedTextureDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_opengl_borrowed_texture_descriptor_default() });
    convert::native_default(value)
}

/// Returns OpenGL owned-texture descriptor defaults for this C API version.
///
/// See `mln_opengl_owned_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
pub fn opengl_owned_texture_descriptor_default() -> OpenglOwnedTextureDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_opengl_owned_texture_descriptor_default() });
    convert::native_default(value)
}

/// Returns OpenGL context providers supported by this build.
///
/// See `mln_opengl_supported_context_provider_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub fn opengl_supported_context_provider_mask() -> OpenglContextProviderFlag {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_opengl_supported_context_provider_mask() });
    OpenglContextProviderFlag::from_bits_retain(value)
}

/// Returns OpenGL surface descriptor defaults for this C API version.
///
/// See `mln_opengl_surface_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
pub fn opengl_surface_descriptor_default() -> OpenglSurfaceDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_opengl_surface_descriptor_default() });
    convert::native_default(value)
}

/// Returns the process-wide `mln_plugin_register_v1` entry point; never
/// null.
///
/// See `mln_plugin_get_register_function_v1` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
pub fn plugin_get_register_function_v1() -> sys::mln_plugin_register_function_v1 {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_plugin_get_register_function_v1() });
    value
}

/// Returns a default premultiplied RGBA8 image descriptor.
///
/// See `mln_premultiplied_rgba8_image_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub fn premultiplied_rgba8_image_default() -> PremultipliedRgba8Image {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_premultiplied_rgba8_image_default() });
    convert::native_default(value)
}

/// Converts a geographic coordinate to spherical Mercator projected meters.
///
/// See `mln_projected_meters_for_lat_lng` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
pub fn projected_meters_for_lat_lng(coordinate: LatLng) -> Result<ProjectedMeters> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let mut out_meters: sys::mln_projected_meters = unsafe { std::mem::zeroed() };
    let coordinate = call.input(&coordinate)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_projected_meters_for_lat_lng(coordinate, &mut out_meters, out_diagnostic)
    })?;
    Ok(unsafe { from_native(out_meters) }?)
}

/// Returns empty axonometric rendering options initialized for this C API
/// version.
///
/// See `mln_projection_mode_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
pub fn projection_mode_default() -> ProjectionMode {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_projection_mode_default() });
    convert::native_default(value)
}

/// Returns attachment options initialized for this C API version, with each
/// field at its documented default.
///
/// See `mln_render_session_attach_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub fn render_session_attach_options_default() -> RenderSessionAttachOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_render_session_attach_options_default() });
    convert::native_default(value)
}

/// Returns default rendered feature query options.
///
/// See `mln_rendered_feature_query_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
pub fn rendered_feature_query_options_default() -> RenderedFeatureQueryOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_rendered_feature_query_options_default() });
    convert::native_default(value)
}

/// Returns an exact-match route for every resource kind.
///
/// See `mln_resource_route_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub fn resource_route_default() -> ResourceRoute {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_resource_route_default() });
    convert::native_default(value)
}

/// Creates a runtime.
///
/// See `mln_runtime_create` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub fn runtime_create(options: &RuntimeOptions) -> Result<RuntimeHandle> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let mut out_runtime = sys::mln_runtime(0);
    let options = call.reference(&options)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_runtime_create(options, &mut out_runtime, out_diagnostic)
    })?;
    Ok(RuntimeHandle::adopt(out_runtime, None)?)
}

/// Returns runtime options initialized for this C API version.
///
/// See `mln_runtime_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub fn runtime_options_default() -> RuntimeOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_runtime_options_default() });
    convert::native_default(value)
}

/// Returns default source feature query options.
///
/// See `mln_source_feature_query_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/query_8h.html).
pub fn source_feature_query_options_default() -> SourceFeatureQueryOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_source_feature_query_options_default() });
    convert::native_default(value)
}

/// Parses a style expression.
///
/// See `mln_style_expression_create` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
pub fn style_expression_create(
    expression: &str,
    options: Option<&StyleExpressionOptions>,
) -> Result<StyleExpressionHandle> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let mut out_expression = sys::mln_style_expression(0);
    let expression = call.input(&expression)?;
    let options = call.optional_reference(options.as_ref())?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_style_expression_create(expression, options, &mut out_expression, out_diagnostic)
    })?;
    Ok(StyleExpressionHandle::adopt(out_expression, None)?)
}

/// Returns default style expression inputs, which set no input.
///
/// See `mln_style_expression_inputs_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
pub fn style_expression_inputs_default() -> StyleExpressionInputs {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_style_expression_inputs_default() });
    convert::native_default(value)
}

/// Returns default style expression options.
///
/// See `mln_style_expression_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
pub fn style_expression_options_default() -> StyleExpressionOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_style_expression_options_default() });
    convert::native_default(value)
}

/// Returns default runtime style image options.
///
/// See `mln_style_image_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub fn style_image_options_default() -> StyleImageOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_style_image_options_default() });
    convert::native_default(value)
}

/// Returns default global style transition options.
///
/// See `mln_style_transition_options_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub fn style_transition_options_default() -> StyleTransitionOptions {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_style_transition_options_default() });
    convert::native_default(value)
}

/// Reports the render backends available in this native library build.
///
/// See `mln_supported_render_backend_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
pub fn supported_render_backend_mask() -> RenderBackendFlag {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_supported_render_backend_mask() });
    RenderBackendFlag::from_bits_retain(value)
}

/// Returns Vulkan borrowed-texture descriptor defaults for this C API
/// version.
///
/// See `mln_vulkan_borrowed_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
pub fn vulkan_borrowed_texture_descriptor_default() -> VulkanBorrowedTextureDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_vulkan_borrowed_texture_descriptor_default() });
    convert::native_default(value)
}

/// Returns Vulkan owned-texture descriptor defaults for this C API version.
///
/// See `mln_vulkan_owned_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
pub fn vulkan_owned_texture_descriptor_default() -> VulkanOwnedTextureDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_vulkan_owned_texture_descriptor_default() });
    convert::native_default(value)
}

/// Returns Vulkan surface descriptor defaults for this C API version.
///
/// See `mln_vulkan_surface_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
pub fn vulkan_surface_descriptor_default() -> VulkanSurfaceDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_vulkan_surface_descriptor_default() });
    convert::native_default(value)
}

/// Returns WebGPU borrowed-texture descriptor defaults for this C API
/// version.
///
/// See `mln_webgpu_borrowed_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
pub fn webgpu_borrowed_texture_descriptor_default() -> WebgpuBorrowedTextureDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_webgpu_borrowed_texture_descriptor_default() });
    convert::native_default(value)
}

/// Returns WebGPU owned-texture descriptor defaults for this C API version.
///
/// See `mln_webgpu_owned_texture_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
pub fn webgpu_owned_texture_descriptor_default() -> WebgpuOwnedTextureDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_webgpu_owned_texture_descriptor_default() });
    convert::native_default(value)
}

/// Returns WebGPU surface descriptor defaults for this C API version.
///
/// See `mln_webgpu_surface_descriptor_default` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/surface_8h.html).
pub fn webgpu_surface_descriptor_default() -> WebgpuSurfaceDescriptor {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_webgpu_surface_descriptor_default() });
    convert::native_default(value)
}
