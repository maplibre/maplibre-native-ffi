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

/// Reports the C ABI contract version. The value is 0 while the ABI is
/// unstable, and will increment on each SemVer major release.
///
/// See `mln_c_version` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
pub fn c_version() -> u32 {
    let mut call = Call::global();
    let value = call.run(|_| unsafe { sys::mln_c_version() });
    value
}

/// Prepares GeoJSON source data for installation on a map.
///
/// See `mln_geojson_source_data_create` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
pub fn geojson_source_data_create(
    data: &str,
    options: Option<&GeoJsonSourceOptions>,
) -> Result<GeoJsonSourceDataHandle> {
    let mut call = Call::global();
    maplibre_core::validate_abi_version()?;
    let mut out_data = sys::mln_geojson_source_data(0);
    let data = call.input(&data)?;
    let options = call.optional_reference(options.as_ref())?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_geojson_source_data_create(data, options, &mut out_data, out_diagnostic)
    })?;
    Ok(GeoJsonSourceDataHandle::adopt(out_data, None)?)
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

/// Returns OpenGL context providers supported by this build.
///
/// See `mln_opengl_supported_context_provider_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub fn opengl_supported_context_provider_mask() -> OpenGlContextProviderFlag {
    let mut call = Call::global();
    maplibre_core::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_opengl_supported_context_provider_mask() });
    OpenGlContextProviderFlag::from_bits_retain(value)
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
