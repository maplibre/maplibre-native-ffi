// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

/// Creates a frame pacer that follows the display through the calling
/// thread's `AChoreographer`.
///
/// Call it on a thread with an `ALooper`, such as the app's main thread,
/// and keep that thread running its looper while sessions use the pacer.
///
/// See `mln_android_frame_pacer_create` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
///
/// # Errors
///
/// - `Status::InvalidState` when the calling thread has no `ALooper`.
#[cfg(target_os = "android")]
pub fn android_frame_pacer_create(options: Option<&FramePacerOptions>) -> Result<FramePacerHandle> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    let mut out_pacer = sys::mln_frame_pacer(0);
    let options = call.optional_reference(options.as_ref())?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_android_frame_pacer_create(options, &mut out_pacer, out_diagnostic)
    })?;
    Ok(FramePacerHandle::adopt(out_pacer)?)
}

/// Gives the library the Java VM and the app context that it needs on
/// Android.
///
/// Call it before creating a runtime. `jni_env` is the calling thread's
/// `JNIEnv*`, and `context` is an Android `Context`, such as the
/// application.
///
/// See `mln_android_init` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/android_8h.html).
///
/// # Safety
/// Native graphics objects must have the types, lifetimes, and synchronization required by the C operation.
#[cfg(target_os = "android")]
pub unsafe fn init_android(
    jni_env: *mut std::ffi::c_void,
    context: *mut std::ffi::c_void,
) -> Result<()> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_android_init(jni_env, context, out_diagnostic)
    })?;
    Ok(())
}

/// Returns the API epoch of the library.
///
/// Headers and a library with different epochs aren't compatible.
///
/// See `mln_c_version` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
pub fn c_version() -> u32 {
    let mut call = Call::global();
    let value = call.run(|_| unsafe { sys::mln_c_version() });
    value
}

/// Computes the camera that shows exactly one tile on a square map.
///
/// Jump a map in `MapMode::Tile` to the camera to render `tile`, where
/// `logical_size` is the map's size. The camera centers the tile, zooms
/// until the tile fills the map, and looks straight down with north up and
/// no padding. The map keeps its camera within its zoom limits, so for a
/// deep tile, raise the maximum zoom past the camera's with
/// `MapHandle::set_bounds()`.
///
/// See `mln_camera_for_tile` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/camera_8h.html).
///
/// # Errors
///
/// - `Status::InvalidArgument` when `logical_size` isn't square, or is
///   wider than 512 pixels. For denser tiles, raise the map's scale factor
///   instead.
/// - `Status::InvalidArgument` when `tile.x` or `tile.y` isn't less than 2
///   to the power `tile.z`, or when the tile can't fill the map at a zoom
///   from 0 to 25.5, as for tile 0/0/0 on a 256-pixel map.
pub fn camera_for_tile(tile: CanonicalTileId, logical_size: LogicalSize) -> Result<CameraOptions> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    let mut out_camera: sys::mln_camera_options = unsafe { std::mem::zeroed() };
    out_camera.size = std::mem::size_of::<sys::mln_camera_options>() as _;
    let tile = call.input(&tile)?;
    let logical_size = call.input(&logical_size)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_camera_for_tile(tile, logical_size, &mut out_camera, out_diagnostic)
    })?;
    Ok(unsafe { from_native(out_camera) }?)
}

/// Converts meters in the Web Mercator projection to a coordinate.
///
/// See `mln_lat_lng_for_projected_meters` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
pub fn lat_lng_for_projected_meters(meters: ProjectedMeters) -> Result<LatLng> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    let mut out_coordinate: sys::mln_lat_lng = unsafe { std::mem::zeroed() };
    let meters = call.input(&meters)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_lat_lng_for_projected_meters(meters, &mut out_coordinate, out_diagnostic)
    })?;
    Ok(unsafe { from_native(out_coordinate) }?)
}

/// Sends log messages to the platform's log again, instead of a callback.
///
/// See `mln_log_clear_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub fn clear_log_callback() -> Result<()> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    call.status(|_, out_diagnostic| unsafe { sys::mln_log_clear_callback(out_diagnostic) })?;
    Ok(())
}

/// Selects the severities that the library logs on a background thread.
///
/// The library logs other severities on the thread that logged them. `mask`
/// applies to every runtime in the process.
///
/// See `mln_log_set_async_severity_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub fn set_log_async_severity_mask(mask: LogSeverityFlag) -> Result<()> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_log_set_async_severity_mask(mask.bits(), out_diagnostic)
    })?;
    Ok(())
}

/// Sends the log messages of every runtime in the process to a callback.
///
/// The callback replaces the one set before.
///
/// Runs on the thread that logged the message, or on a background thread
/// for the severities that `set_log_async_severity_mask()` selects. The
/// callback can't call the library.
///
/// See `mln_log_set_callback` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
pub fn set_log_callback(handler: LogHandler) -> Result<()> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    let handler = call.reference(&handler)?;
    call.status(|_, out_diagnostic| unsafe { sys::mln_log_set_callback(handler, out_diagnostic) })?;
    Ok(())
}

/// Computes the size in pixels of a target with a logical size and scale
/// factor.
///
/// See `mln_logical_extent_physical_size` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub fn logical_extent_physical_size(extent: LogicalExtent) -> Result<(u32, u32)> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
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

/// Reads whether the library may use the network.
///
/// See `mln_network_get_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub fn get_network_status() -> Result<NetworkStatus> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    let mut out_status: sys::mln_network_status = Default::default();
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_network_get_status(&mut out_status, out_diagnostic)
    })?;
    Ok(NetworkStatus::from_raw(out_status))
}

/// Tells the library whether it may use the network.
///
/// While the status is `NetworkStatus::Offline`, the library starts no
/// network requests. The requests that wait start when the status returns
/// to `NetworkStatus::Online`. The status starts as online and applies to
/// every runtime in the process.
///
/// See `mln_network_set_status` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
pub fn set_network_status(status: NetworkStatus) -> Result<()> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_network_set_status(status.as_raw(), out_diagnostic)
    })?;
    Ok(())
}

/// Creates a frame pacer that follows the display through OpenHarmony's
/// `OH_NativeVSync`.
///
/// You can call it on any thread. The pacer needs no event loop of yours.
///
/// See `mln_ohos_frame_pacer_create` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/ohos_8h.html).
///
/// # Errors
///
/// - `Status::NativeError` when the system can't report the display's
///   refreshes to the process.
#[cfg(target_env = "ohos")]
pub fn ohos_frame_pacer_create(options: Option<&FramePacerOptions>) -> Result<FramePacerHandle> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    let mut out_pacer = sys::mln_frame_pacer(0);
    let options = call.optional_reference(options.as_ref())?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_ohos_frame_pacer_create(options, &mut out_pacer, out_diagnostic)
    })?;
    Ok(FramePacerHandle::adopt(out_pacer)?)
}

/// Reports the OpenGL context platforms that this build of the library has.
///
/// See `mln_open_gl_supported_context_provider_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
pub fn open_gl_supported_context_provider_mask() -> OpenGlContextProviderFlag {
    let mut call = Call::global();
    crate::abi::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_open_gl_supported_context_provider_mask() });
    OpenGlContextProviderFlag::from_bits_retain(value)
}

/// Returns the function that registers a plugin's style layer types.
///
/// Pass the function to the plugin's registration entry point before
/// loading a style that uses the plugin's layer types. Plugin callbacks run
/// on library threads and can keep running while the process exits, so they
/// must not use state that the process destroys at exit.
///
/// See `mln_plugin_get_register_function_v1` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/plugin_8h.html).
pub fn get_plugin_register_function_v1() -> sys::mln_plugin_register_function_v1 {
    let mut call = Call::global();
    crate::abi::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_plugin_get_register_function_v1() });
    value
}

/// Converts a coordinate to meters in the Web Mercator projection.
///
/// See `mln_projected_meters_for_lat_lng` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/projection_8h.html).
pub fn projected_meters_for_lat_lng(coordinate: LatLng) -> Result<ProjectedMeters> {
    let mut call = Call::global();
    crate::abi::validate_abi_version()?;
    let mut out_meters: sys::mln_projected_meters = unsafe { std::mem::zeroed() };
    let coordinate = call.input(&coordinate)?;
    call.status(|_, out_diagnostic| unsafe {
        sys::mln_projected_meters_for_lat_lng(coordinate, &mut out_meters, out_diagnostic)
    })?;
    Ok(unsafe { from_native(out_meters) }?)
}

/// Returns the render backends that this build of the library draws with.
///
/// See `mln_supported_render_backend_mask` in the
/// [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
pub fn supported_render_backend_mask() -> RenderBackendFlag {
    let mut call = Call::global();
    crate::abi::require_abi_version();
    let value = call.run(|_| unsafe { sys::mln_supported_render_backend_mask() });
    RenderBackendFlag::from_bits_retain(value)
}
