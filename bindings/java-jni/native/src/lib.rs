//! JNI bridge entry points for the Java JNI binding.
//!
//! This crate owns JNI registration and delegates shared ABI adaptation to the
//! Rust binding crates.

use std::ffi::c_void;
use std::panic::{AssertUnwindSafe, catch_unwind};

use jni::objects::{JClass, JIntArray, JObject};
use jni::sys::{JNI_VERSION_1_8, jint, jlong, jstring};
use jni::{JNIEnv, JavaVM, NativeMethod};
use maplibre_native_core::error::capture_thread_diagnostic;
use maplibre_native_sys as sys;

const BRIDGE_CLASS: &str = "org/maplibre/nativejni/internal/bridge/NativeBridge";

#[unsafe(no_mangle)]
pub extern "system" fn JNI_OnLoad(vm: JavaVM, _reserved: *mut c_void) -> jint {
    match catch_unwind(AssertUnwindSafe(|| registration::register_natives(&vm))) {
        Ok(Ok(())) => JNI_VERSION_1_8,
        _ => 0,
    }
}

mod registration {
    use super::*;

    type NoArgStatusMethod = extern "system" fn(JNIEnv<'_>, JClass<'_>) -> jint;

    pub(super) fn register_natives(vm: &JavaVM) -> jni::errors::Result<()> {
        register_legacy_bridge(vm)?;
        register_base(vm)?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/LogNative",
            &[
                "mln_log_set_callback",
                "mln_log_clear_callback",
                "mln_log_set_async_severity_mask",
            ],
        )?;
        register_runtime(vm)?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/OfflineNative",
            &[
                "mln_runtime_offline_region_create_start",
                "mln_runtime_offline_region_get_start",
                "mln_runtime_offline_regions_list_start",
                "mln_runtime_offline_regions_merge_database_start",
                "mln_runtime_offline_region_update_metadata_start",
                "mln_runtime_offline_region_get_status_start",
                "mln_runtime_offline_region_set_observed_start",
                "mln_runtime_offline_region_set_download_state_start",
                "mln_runtime_offline_region_invalidate_start",
                "mln_runtime_offline_region_delete_start",
                "mln_runtime_offline_region_create_take_result",
                "mln_runtime_offline_region_get_take_result",
                "mln_runtime_offline_regions_list_take_result",
                "mln_runtime_offline_regions_merge_database_take_result",
                "mln_runtime_offline_region_update_metadata_take_result",
                "mln_runtime_offline_region_get_status_take_result",
                "mln_offline_region_snapshot_get",
                "mln_offline_region_snapshot_destroy",
                "mln_offline_region_list_count",
                "mln_offline_region_list_get",
                "mln_offline_region_list_destroy",
            ],
        )?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/MapNative",
            &[
                "mln_map_options_default",
                "mln_map_create",
                "mln_map_request_repaint",
                "mln_map_request_still_image",
                "mln_map_destroy",
                "mln_map_set_style_url",
                "mln_map_set_style_json",
            ],
        )?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/CameraNative",
            &[
                "mln_camera_options_default",
                "mln_animation_options_default",
                "mln_camera_fit_options_default",
                "mln_bound_options_default",
                "mln_free_camera_options_default",
                "mln_projection_mode_default",
                "mln_map_viewport_options_default",
                "mln_map_tile_options_default",
                "mln_map_set_debug_options",
                "mln_map_get_debug_options",
                "mln_map_set_rendering_stats_view_enabled",
                "mln_map_get_rendering_stats_view_enabled",
                "mln_map_is_fully_loaded",
                "mln_map_dump_debug_logs",
                "mln_map_get_viewport_options",
                "mln_map_set_viewport_options",
                "mln_map_get_tile_options",
                "mln_map_set_tile_options",
                "mln_map_get_camera",
                "mln_map_jump_to",
                "mln_map_ease_to",
                "mln_map_fly_to",
                "mln_map_move_by",
                "mln_map_move_by_animated",
                "mln_map_scale_by",
                "mln_map_scale_by_animated",
                "mln_map_rotate_by",
                "mln_map_rotate_by_animated",
                "mln_map_pitch_by",
                "mln_map_pitch_by_animated",
                "mln_map_cancel_transitions",
                "mln_map_camera_for_lat_lng_bounds",
                "mln_map_camera_for_lat_lngs",
                "mln_map_camera_for_geometry",
                "mln_map_lat_lng_bounds_for_camera",
                "mln_map_lat_lng_bounds_for_camera_unwrapped",
                "mln_map_get_bounds",
                "mln_map_set_bounds",
                "mln_map_get_free_camera_options",
                "mln_map_set_free_camera_options",
                "mln_map_get_projection_mode",
                "mln_map_set_projection_mode",
                "mln_map_pixel_for_lat_lng",
                "mln_map_lat_lng_for_pixel",
                "mln_map_pixels_for_lat_lngs",
                "mln_map_lat_lngs_for_pixels",
            ],
        )?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/ProjectionNative",
            &[
                "mln_map_projection_create",
                "mln_map_projection_destroy",
                "mln_map_projection_get_camera",
                "mln_map_projection_set_camera",
                "mln_map_projection_set_visible_coordinates",
                "mln_map_projection_set_visible_geometry",
                "mln_map_projection_pixel_for_lat_lng",
                "mln_map_projection_lat_lng_for_pixel",
                "mln_projected_meters_for_lat_lng",
                "mln_lat_lng_for_projected_meters",
            ],
        )?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/QueryNative",
            &[
                "mln_rendered_feature_query_options_default",
                "mln_source_feature_query_options_default",
                "mln_rendered_query_geometry_point",
                "mln_rendered_query_geometry_box",
                "mln_rendered_query_geometry_line_string",
                "mln_render_session_query_rendered_features",
                "mln_render_session_query_source_features",
                "mln_render_session_query_feature_extensions",
                "mln_feature_query_result_count",
                "mln_feature_query_result_get",
                "mln_feature_query_result_destroy",
                "mln_feature_extension_result_get",
                "mln_feature_extension_result_destroy",
            ],
        )?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/RenderSessionNative",
            &[
                "mln_render_session_resize",
                "mln_render_session_render_update",
                "mln_render_session_detach",
                "mln_render_session_destroy",
                "mln_render_session_reduce_memory_use",
                "mln_render_session_clear_data",
                "mln_render_session_dump_debug_logs",
                "mln_render_session_set_feature_state",
                "mln_render_session_get_feature_state",
                "mln_render_session_remove_feature_state",
                "mln_json_snapshot_get",
                "mln_json_snapshot_destroy",
            ],
        )?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/SurfaceNative",
            &[
                "mln_metal_surface_descriptor_default",
                "mln_vulkan_surface_descriptor_default",
                "mln_metal_surface_attach",
                "mln_vulkan_surface_attach",
            ],
        )?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/TextureNative",
            &[
                "mln_metal_owned_texture_descriptor_default",
                "mln_metal_borrowed_texture_descriptor_default",
                "mln_vulkan_owned_texture_descriptor_default",
                "mln_vulkan_borrowed_texture_descriptor_default",
                "mln_texture_image_info_default",
                "mln_metal_owned_texture_attach",
                "mln_metal_borrowed_texture_attach",
                "mln_vulkan_owned_texture_attach",
                "mln_vulkan_borrowed_texture_attach",
                "mln_texture_read_premultiplied_rgba8",
                "mln_metal_owned_texture_acquire_frame",
                "mln_metal_owned_texture_release_frame",
                "mln_vulkan_owned_texture_acquire_frame",
                "mln_vulkan_owned_texture_release_frame",
            ],
        )?;
        register_no_arg_status_class(
            vm,
            "org/maplibre/nativejni/internal/bridge/StyleNative",
            &[
                "mln_style_tile_source_options_default",
                "mln_custom_geometry_source_options_default",
                "mln_premultiplied_rgba8_image_default",
                "mln_style_image_options_default",
                "mln_style_image_info_default",
                "mln_style_id_list_count",
                "mln_style_id_list_get",
                "mln_style_id_list_destroy",
                "mln_map_add_style_source_json",
                "mln_map_remove_style_source",
                "mln_map_style_source_exists",
                "mln_map_get_style_source_type",
                "mln_map_get_style_source_info",
                "mln_map_copy_style_source_attribution",
                "mln_map_list_style_source_ids",
                "mln_map_add_geojson_source_url",
                "mln_map_add_geojson_source_data",
                "mln_map_set_geojson_source_url",
                "mln_map_set_geojson_source_data",
                "mln_map_add_vector_source_url",
                "mln_map_add_vector_source_tiles",
                "mln_map_add_raster_source_url",
                "mln_map_add_raster_source_tiles",
                "mln_map_add_raster_dem_source_url",
                "mln_map_add_raster_dem_source_tiles",
                "mln_map_add_custom_geometry_source",
                "mln_map_set_custom_geometry_source_tile_data",
                "mln_map_invalidate_custom_geometry_source_tile",
                "mln_map_invalidate_custom_geometry_source_region",
                "mln_map_set_style_image",
                "mln_map_remove_style_image",
                "mln_map_style_image_exists",
                "mln_map_get_style_image_info",
                "mln_map_copy_style_image_premultiplied_rgba8",
                "mln_map_add_image_source_url",
                "mln_map_add_image_source_image",
                "mln_map_set_image_source_url",
                "mln_map_set_image_source_image",
                "mln_map_set_image_source_coordinates",
                "mln_map_get_image_source_coordinates",
                "mln_map_add_hillshade_layer",
                "mln_map_add_color_relief_layer",
                "mln_map_add_location_indicator_layer",
                "mln_map_set_location_indicator_location",
                "mln_map_set_location_indicator_bearing",
                "mln_map_set_location_indicator_accuracy_radius",
                "mln_map_set_location_indicator_image_name",
                "mln_map_add_style_layer_json",
                "mln_map_remove_style_layer",
                "mln_map_style_layer_exists",
                "mln_map_get_style_layer_type",
                "mln_map_list_style_layer_ids",
                "mln_map_move_style_layer",
                "mln_map_get_style_layer_json",
                "mln_map_set_style_light_json",
                "mln_map_set_style_light_property",
                "mln_map_get_style_light_property",
                "mln_map_set_layer_property",
                "mln_map_get_layer_property",
                "mln_map_set_layer_filter",
                "mln_map_get_layer_filter",
            ],
        )?;
        Ok(())
    }

    fn register_legacy_bridge(vm: &JavaVM) -> jni::errors::Result<()> {
        register_methods(
            vm,
            BRIDGE_CLASS,
            vec![
                NativeMethod {
                    name: "cVersion".into(),
                    sig: "()J".into(),
                    fn_ptr: c_version as *mut c_void,
                },
                NativeMethod {
                    name: "supportedRenderBackendMask".into(),
                    sig: "()I".into(),
                    fn_ptr: supported_render_backend_mask as *mut c_void,
                },
                NativeMethod {
                    name: "networkStatusGet".into(),
                    sig: "([I)I".into(),
                    fn_ptr: network_status_get as *mut c_void,
                },
                NativeMethod {
                    name: "networkStatusSet".into(),
                    sig: "(I)I".into(),
                    fn_ptr: network_status_set as *mut c_void,
                },
                NativeMethod {
                    name: "threadLastErrorMessage".into(),
                    sig: "()Ljava/lang/String;".into(),
                    fn_ptr: thread_last_error_message as *mut c_void,
                },
            ],
        )
    }

    fn register_base(vm: &JavaVM) -> jni::errors::Result<()> {
        register_methods(
            vm,
            "org/maplibre/nativejni/internal/bridge/BaseNative",
            vec![
                NativeMethod {
                    name: "mln_c_version".into(),
                    sig: "()J".into(),
                    fn_ptr: c_version as *mut c_void,
                },
                NativeMethod {
                    name: "mln_supported_render_backend_mask".into(),
                    sig: "()I".into(),
                    fn_ptr: supported_render_backend_mask as *mut c_void,
                },
                NativeMethod {
                    name: "mln_thread_last_error_message".into(),
                    sig: "()Ljava/lang/String;".into(),
                    fn_ptr: thread_last_error_message as *mut c_void,
                },
            ],
        )
    }

    fn register_runtime(vm: &JavaVM) -> jni::errors::Result<()> {
        let mut methods = no_arg_status_methods(&[
            "mln_runtime_options_default",
            "mln_runtime_create",
            "mln_runtime_set_resource_provider",
            "mln_resource_request_complete",
            "mln_resource_request_cancelled",
            "mln_resource_request_release",
            "mln_runtime_set_resource_transform",
            "mln_runtime_clear_resource_transform",
            "mln_runtime_run_ambient_cache_operation_start",
            "mln_runtime_offline_operation_discard",
            "mln_runtime_destroy",
            "mln_runtime_run_once",
            "mln_runtime_poll_event",
        ]);
        methods.push(NativeMethod {
            name: "mln_network_status_get".into(),
            sig: "([I)I".into(),
            fn_ptr: network_status_get as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_network_status_set".into(),
            sig: "(I)I".into(),
            fn_ptr: network_status_set as *mut c_void,
        });
        register_methods(
            vm,
            "org/maplibre/nativejni/internal/bridge/RuntimeNative",
            methods,
        )
    }

    fn register_no_arg_status_class(
        vm: &JavaVM,
        class_name: &str,
        names: &[&str],
    ) -> jni::errors::Result<()> {
        register_methods(vm, class_name, no_arg_status_methods(names))
    }

    fn no_arg_status_methods(names: &[&str]) -> Vec<NativeMethod> {
        names
            .iter()
            .map(|name| NativeMethod {
                name: (*name).into(),
                sig: "()I".into(),
                fn_ptr: unsupported_status as NoArgStatusMethod as *mut c_void,
            })
            .collect()
    }

    fn register_methods(
        vm: &JavaVM,
        class_name: &str,
        methods: Vec<NativeMethod>,
    ) -> jni::errors::Result<()> {
        let mut env = vm.get_env()?;
        let class = env.find_class(class_name)?;
        env.register_native_methods(class, &methods)
    }
}

extern "system" fn c_version(_env: JNIEnv<'_>, _class: JClass<'_>) -> jlong {
    catch_unwind(|| unsafe { sys::mln_c_version() as jlong }).unwrap_or(0)
}

extern "system" fn supported_render_backend_mask(_env: JNIEnv<'_>, _class: JClass<'_>) -> jint {
    catch_unwind(|| unsafe { sys::mln_supported_render_backend_mask() as jint }).unwrap_or(0)
}

extern "system" fn network_status_get(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    out_status: JIntArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_status.is_null() || env.get_array_length(&out_status).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }

        let mut status: sys::mln_network_status = 0;
        let result = unsafe { sys::mln_network_status_get(&mut status) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_int_array_region(&out_status, 0, &[status as jint])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn network_status_set(_env: JNIEnv<'_>, _class: JClass<'_>, status: jint) -> jint {
    catch_unwind(|| unsafe { sys::mln_network_status_set(status as sys::mln_network_status) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn thread_last_error_message(env: JNIEnv<'_>, _class: JClass<'_>) -> jstring {
    catch_unwind(AssertUnwindSafe(|| {
        let diagnostic = capture_thread_diagnostic();
        match env.new_string(diagnostic) {
            Ok(message) => message.into_raw(),
            Err(_) => JObject::null().into_raw(),
        }
    }))
    .unwrap_or_else(|_| JObject::null().into_raw())
}

extern "system" fn unsupported_status(_env: JNIEnv<'_>, _class: JClass<'_>) -> jint {
    sys::MLN_STATUS_UNSUPPORTED
}
