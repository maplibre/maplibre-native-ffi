//! JNI bridge entry points for the Java JNI binding.
//!
//! This crate owns JNI registration and delegates shared ABI adaptation to the
//! Rust binding crates.

use std::ffi::{CString, c_char, c_void};
use std::panic::{AssertUnwindSafe, catch_unwind};

use jni::objects::{
    JBooleanArray, JByteArray, JClass, JDoubleArray, JIntArray, JLongArray, JObject, JObjectArray,
    JString,
};
use jni::sys::{JNI_VERSION_1_8, jboolean, jint, jlong, jstring};
use jni::{JNIEnv, JavaVM, NativeMethod};
use maplibre_native_core::error::capture_thread_diagnostic;
use maplibre_native_sys as sys;

const BRIDGE_CLASS: &str = "org/maplibre/nativejni/internal/bridge/NativeBridge";

const LONG_SOURCE_ADDRESS: usize = 0;
const LONG_PAYLOAD_SIZE: usize = 1;
const LONG_TILE_OVERSCALED_Z: usize = 2;
const LONG_TILE_CANONICAL_Z: usize = 3;
const LONG_TILE_CANONICAL_X: usize = 4;
const LONG_TILE_CANONICAL_Y: usize = 5;
const LONG_REGION_ID: usize = 6;
const LONG_LIMIT: usize = 7;
const LONG_OPERATION_ID: usize = 8;
const LONG_COMPLETED_RESOURCE_COUNT: usize = 9;
const LONG_COMPLETED_RESOURCE_SIZE: usize = 10;
const LONG_COMPLETED_TILE_COUNT: usize = 11;
const LONG_REQUIRED_TILE_COUNT: usize = 12;
const LONG_COMPLETED_TILE_SIZE: usize = 13;
const LONG_REQUIRED_RESOURCE_COUNT: usize = 14;
const LONG_FRAME_COUNT: usize = 15;
const LONG_DRAW_CALL_COUNT: usize = 16;
const LONG_TOTAL_DRAW_CALL_COUNT: usize = 17;
const LONG_COUNT: usize = 18;

const INT_EVENT_TYPE: usize = 0;
const INT_SOURCE_TYPE: usize = 1;
const INT_CODE: usize = 2;
const INT_PAYLOAD_TYPE: usize = 3;
const INT_RENDER_MODE: usize = 4;
const INT_TILE_OPERATION: usize = 5;
const INT_TILE_WRAP: usize = 6;
const INT_RESOURCE_ERROR_REASON: usize = 7;
const INT_OFFLINE_DOWNLOAD_STATE: usize = 8;
const INT_OFFLINE_OPERATION_KIND: usize = 9;
const INT_OFFLINE_RESULT_KIND: usize = 10;
const INT_OFFLINE_RESULT_STATUS: usize = 11;
const INT_PAYLOAD_AVAILABLE: usize = 12;
const INT_COUNT: usize = 13;

const BOOLEAN_HAS_EVENT: usize = 0;
const BOOLEAN_NEEDS_REPAINT: usize = 1;
const BOOLEAN_PLACEMENT_CHANGED: usize = 2;
const BOOLEAN_REQUIRED_RESOURCE_COUNT_IS_PRECISE: usize = 3;
const BOOLEAN_COMPLETE: usize = 4;
const BOOLEAN_FOUND: usize = 5;
const BOOLEAN_COUNT: usize = 6;

const DOUBLE_ENCODING_TIME: usize = 0;
const DOUBLE_RENDERING_TIME: usize = 1;
const DOUBLE_COUNT: usize = 2;

const STRING_MESSAGE: i32 = 0;
const STRING_PAYLOAD: i32 = 1;
const STRING_COUNT: i32 = 2;

const CAMERA_FIELD_CENTER: usize = 0;
const CAMERA_FIELD_CENTER_ALTITUDE: usize = 1;
const CAMERA_FIELD_PADDING: usize = 2;
const CAMERA_FIELD_ANCHOR: usize = 3;
const CAMERA_FIELD_ZOOM: usize = 4;
const CAMERA_FIELD_BEARING: usize = 5;
const CAMERA_FIELD_PITCH: usize = 6;
const CAMERA_FIELD_ROLL: usize = 7;
const CAMERA_FIELD_FOV: usize = 8;
const CAMERA_FIELD_COUNT: usize = 9;

const CAMERA_VALUE_LATITUDE: usize = 0;
const CAMERA_VALUE_LONGITUDE: usize = 1;
const CAMERA_VALUE_CENTER_ALTITUDE: usize = 2;
const CAMERA_VALUE_PADDING_TOP: usize = 3;
const CAMERA_VALUE_PADDING_LEFT: usize = 4;
const CAMERA_VALUE_PADDING_BOTTOM: usize = 5;
const CAMERA_VALUE_PADDING_RIGHT: usize = 6;
const CAMERA_VALUE_ANCHOR_X: usize = 7;
const CAMERA_VALUE_ANCHOR_Y: usize = 8;
const CAMERA_VALUE_ZOOM: usize = 9;
const CAMERA_VALUE_BEARING: usize = 10;
const CAMERA_VALUE_PITCH: usize = 11;
const CAMERA_VALUE_ROLL: usize = 12;
const CAMERA_VALUE_FOV: usize = 13;
const CAMERA_VALUE_COUNT: usize = 14;

const FIT_FIELD_PADDING: usize = 0;
const FIT_FIELD_BEARING: usize = 1;
const FIT_FIELD_PITCH: usize = 2;
const FIT_FIELD_COUNT: usize = 3;
const FIT_VALUE_PADDING_TOP: usize = 0;
const FIT_VALUE_PADDING_LEFT: usize = 1;
const FIT_VALUE_PADDING_BOTTOM: usize = 2;
const FIT_VALUE_PADDING_RIGHT: usize = 3;
const FIT_VALUE_BEARING: usize = 4;
const FIT_VALUE_PITCH: usize = 5;
const FIT_VALUE_COUNT: usize = 6;

const BOUND_FIELD_BOUNDS: usize = 0;
const BOUND_FIELD_MIN_ZOOM: usize = 1;
const BOUND_FIELD_MAX_ZOOM: usize = 2;
const BOUND_FIELD_MIN_PITCH: usize = 3;
const BOUND_FIELD_MAX_PITCH: usize = 4;
const BOUND_FIELD_COUNT: usize = 5;

const BOUND_VALUE_SW_LATITUDE: usize = 0;
const BOUND_VALUE_SW_LONGITUDE: usize = 1;
const BOUND_VALUE_NE_LATITUDE: usize = 2;
const BOUND_VALUE_NE_LONGITUDE: usize = 3;
const BOUND_VALUE_MIN_ZOOM: usize = 4;
const BOUND_VALUE_MAX_ZOOM: usize = 5;
const BOUND_VALUE_MIN_PITCH: usize = 6;
const BOUND_VALUE_MAX_PITCH: usize = 7;
const BOUND_VALUE_COUNT: usize = 8;

const FREE_CAMERA_FIELD_POSITION: usize = 0;
const FREE_CAMERA_FIELD_ORIENTATION: usize = 1;
const FREE_CAMERA_FIELD_COUNT: usize = 2;
const FREE_CAMERA_VALUE_POSITION_X: usize = 0;
const FREE_CAMERA_VALUE_POSITION_Y: usize = 1;
const FREE_CAMERA_VALUE_POSITION_Z: usize = 2;
const FREE_CAMERA_VALUE_ORIENTATION_X: usize = 3;
const FREE_CAMERA_VALUE_ORIENTATION_Y: usize = 4;
const FREE_CAMERA_VALUE_ORIENTATION_Z: usize = 5;
const FREE_CAMERA_VALUE_ORIENTATION_W: usize = 6;
const FREE_CAMERA_VALUE_COUNT: usize = 7;

const PROJECTION_MODE_FIELD_AXONOMETRIC: usize = 0;
const PROJECTION_MODE_FIELD_X_SKEW: usize = 1;
const PROJECTION_MODE_FIELD_Y_SKEW: usize = 2;
const PROJECTION_MODE_FIELD_COUNT: usize = 3;
const PROJECTION_MODE_BOOLEAN_AXONOMETRIC: usize = 0;
const PROJECTION_MODE_BOOLEAN_COUNT: usize = 1;
const PROJECTION_MODE_VALUE_X_SKEW: usize = 0;
const PROJECTION_MODE_VALUE_Y_SKEW: usize = 1;
const PROJECTION_MODE_VALUE_COUNT: usize = 2;

const VIEWPORT_FIELD_NORTH_ORIENTATION: usize = 0;
const VIEWPORT_FIELD_CONSTRAIN_MODE: usize = 1;
const VIEWPORT_FIELD_VIEWPORT_MODE: usize = 2;
const VIEWPORT_FIELD_FRUSTUM_OFFSET: usize = 3;
const VIEWPORT_FIELD_COUNT: usize = 4;
const VIEWPORT_INT_NORTH_ORIENTATION: usize = 0;
const VIEWPORT_INT_CONSTRAIN_MODE: usize = 1;
const VIEWPORT_INT_VIEWPORT_MODE: usize = 2;
const VIEWPORT_INT_COUNT: usize = 3;
const VIEWPORT_VALUE_FRUSTUM_TOP: usize = 0;
const VIEWPORT_VALUE_FRUSTUM_LEFT: usize = 1;
const VIEWPORT_VALUE_FRUSTUM_BOTTOM: usize = 2;
const VIEWPORT_VALUE_FRUSTUM_RIGHT: usize = 3;
const VIEWPORT_VALUE_COUNT: usize = 4;

const TILE_FIELD_PREFETCH_ZOOM_DELTA: usize = 0;
const TILE_FIELD_LOD_MIN_RADIUS: usize = 1;
const TILE_FIELD_LOD_SCALE: usize = 2;
const TILE_FIELD_LOD_PITCH_THRESHOLD: usize = 3;
const TILE_FIELD_LOD_ZOOM_SHIFT: usize = 4;
const TILE_FIELD_LOD_MODE: usize = 5;
const TILE_FIELD_COUNT: usize = 6;
const TILE_INT_PREFETCH_ZOOM_DELTA: usize = 0;
const TILE_INT_LOD_MODE: usize = 1;
const TILE_INT_COUNT: usize = 2;
const TILE_VALUE_LOD_MIN_RADIUS: usize = 0;
const TILE_VALUE_LOD_SCALE: usize = 1;
const TILE_VALUE_LOD_PITCH_THRESHOLD: usize = 2;
const TILE_VALUE_LOD_ZOOM_SHIFT: usize = 3;
const TILE_VALUE_COUNT: usize = 4;

const ANIMATION_FIELD_DURATION: usize = 0;
const ANIMATION_FIELD_VELOCITY: usize = 1;
const ANIMATION_FIELD_MIN_ZOOM: usize = 2;
const ANIMATION_FIELD_EASING: usize = 3;
const ANIMATION_FIELD_COUNT: usize = 4;

const ANIMATION_VALUE_DURATION: usize = 0;
const ANIMATION_VALUE_VELOCITY: usize = 1;
const ANIMATION_VALUE_MIN_ZOOM: usize = 2;
const ANIMATION_VALUE_EASING_X1: usize = 3;
const ANIMATION_VALUE_EASING_Y1: usize = 4;
const ANIMATION_VALUE_EASING_X2: usize = 5;
const ANIMATION_VALUE_EASING_Y2: usize = 6;
const ANIMATION_VALUE_COUNT: usize = 7;

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
        register_offline(vm)?;
        register_map(vm)?;
        register_camera(vm)?;
        register_projection(vm)?;
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
        let mut style_methods = no_arg_status_methods(&[
            "mln_style_tile_source_options_default",
            "mln_custom_geometry_source_options_default",
            "mln_premultiplied_rgba8_image_default",
            "mln_style_image_options_default",
            "mln_style_image_info_default",
            "mln_style_id_list_count",
            "mln_style_id_list_get",
            "mln_style_id_list_destroy",
            "mln_map_add_style_source_json",
            "mln_map_add_geojson_source_data",
            "mln_map_set_geojson_source_data",
            "mln_map_add_custom_geometry_source",
            "mln_map_set_custom_geometry_source_tile_data",
            "mln_map_invalidate_custom_geometry_source_tile",
            "mln_map_invalidate_custom_geometry_source_region",
            "mln_map_add_style_layer_json",
            "mln_map_get_style_layer_json",
            "mln_map_set_style_light_json",
            "mln_map_set_style_light_property",
            "mln_map_get_style_light_property",
            "mln_map_set_layer_property",
            "mln_map_get_layer_property",
            "mln_map_set_layer_filter",
            "mln_map_get_layer_filter",
        ]);
        style_methods.push(NativeMethod {
            name: "mln_map_remove_style_source".into(),
            sig: "(JLjava/lang/String;[Z)I".into(),
            fn_ptr: map_remove_style_source as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_style_source_exists".into(),
            sig: "(JLjava/lang/String;[Z)I".into(),
            fn_ptr: map_style_source_exists as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_get_style_source_type".into(),
            sig: "(JLjava/lang/String;[I[Z)I".into(),
            fn_ptr: map_get_style_source_type as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_get_style_source_info".into(),
            sig: "(JLjava/lang/String;[I[Z[J)I".into(),
            fn_ptr: map_get_style_source_info as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_copy_style_source_attribution".into(),
            sig: "(JLjava/lang/String;[B[J[Z)I".into(),
            fn_ptr: map_copy_style_source_attribution as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_list_style_source_ids".into(),
            sig: "(J[Ljava/lang/Object;)I".into(),
            fn_ptr: map_list_style_source_ids as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_geojson_source_url".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;)I".into(),
            fn_ptr: map_add_geojson_source_url as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_set_geojson_source_url".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;)I".into(),
            fn_ptr: map_set_geojson_source_url as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_vector_source_url".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;[Z[DLjava/lang/String;)I".into(),
            fn_ptr: map_add_vector_source_url as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_raster_source_url".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;[Z[DLjava/lang/String;)I".into(),
            fn_ptr: map_add_raster_source_url as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_raster_dem_source_url".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;[Z[DLjava/lang/String;)I".into(),
            fn_ptr: map_add_raster_dem_source_url as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_vector_source_tiles".into(),
            sig: "(JLjava/lang/String;[Ljava/lang/String;[Z[DLjava/lang/String;)I".into(),
            fn_ptr: map_add_vector_source_tiles as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_raster_source_tiles".into(),
            sig: "(JLjava/lang/String;[Ljava/lang/String;[Z[DLjava/lang/String;)I".into(),
            fn_ptr: map_add_raster_source_tiles as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_raster_dem_source_tiles".into(),
            sig: "(JLjava/lang/String;[Ljava/lang/String;[Z[DLjava/lang/String;)I".into(),
            fn_ptr: map_add_raster_dem_source_tiles as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_set_style_image".into(),
            sig: "(JLjava/lang/String;III[BZDZZ)I".into(),
            fn_ptr: map_set_style_image as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_remove_style_image".into(),
            sig: "(JLjava/lang/String;[Z)I".into(),
            fn_ptr: map_remove_style_image as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_style_image_exists".into(),
            sig: "(JLjava/lang/String;[Z)I".into(),
            fn_ptr: map_style_image_exists as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_get_style_image_info".into(),
            sig: "(JLjava/lang/String;[I[J[D[Z)I".into(),
            fn_ptr: map_get_style_image_info as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_copy_style_image_premultiplied_rgba8".into(),
            sig: "(JLjava/lang/String;[B[J[Z)I".into(),
            fn_ptr: map_copy_style_image_premultiplied_rgba8 as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_image_source_url".into(),
            sig: "(JLjava/lang/String;[DLjava/lang/String;)I".into(),
            fn_ptr: map_add_image_source_url as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_image_source_image".into(),
            sig: "(JLjava/lang/String;[DIII[B)I".into(),
            fn_ptr: map_add_image_source_image as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_set_image_source_url".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;)I".into(),
            fn_ptr: map_set_image_source_url as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_set_image_source_image".into(),
            sig: "(JLjava/lang/String;III[B)I".into(),
            fn_ptr: map_set_image_source_image as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_set_image_source_coordinates".into(),
            sig: "(JLjava/lang/String;[D)I".into(),
            fn_ptr: map_set_image_source_coordinates as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_get_image_source_coordinates".into(),
            sig: "(JLjava/lang/String;[D[J[Z)I".into(),
            fn_ptr: map_get_image_source_coordinates as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_hillshade_layer".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I".into(),
            fn_ptr: map_add_hillshade_layer as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_color_relief_layer".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;Ljava/lang/String;)I".into(),
            fn_ptr: map_add_color_relief_layer as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_add_location_indicator_layer".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;)I".into(),
            fn_ptr: map_add_location_indicator_layer as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_set_location_indicator_location".into(),
            sig: "(JLjava/lang/String;DDD)I".into(),
            fn_ptr: map_set_location_indicator_location as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_set_location_indicator_bearing".into(),
            sig: "(JLjava/lang/String;D)I".into(),
            fn_ptr: map_set_location_indicator_bearing as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_set_location_indicator_accuracy_radius".into(),
            sig: "(JLjava/lang/String;D)I".into(),
            fn_ptr: map_set_location_indicator_accuracy_radius as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_set_location_indicator_image_name".into(),
            sig: "(JLjava/lang/String;ILjava/lang/String;)I".into(),
            fn_ptr: map_set_location_indicator_image_name as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_remove_style_layer".into(),
            sig: "(JLjava/lang/String;[Z)I".into(),
            fn_ptr: map_remove_style_layer as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_style_layer_exists".into(),
            sig: "(JLjava/lang/String;[Z)I".into(),
            fn_ptr: map_style_layer_exists as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_get_style_layer_type".into(),
            sig: "(JLjava/lang/String;[Ljava/lang/String;[Z)I".into(),
            fn_ptr: map_get_style_layer_type as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_list_style_layer_ids".into(),
            sig: "(J[Ljava/lang/Object;)I".into(),
            fn_ptr: map_list_style_layer_ids as *mut c_void,
        });
        style_methods.push(NativeMethod {
            name: "mln_map_move_style_layer".into(),
            sig: "(JLjava/lang/String;Ljava/lang/String;)I".into(),
            fn_ptr: map_move_style_layer as *mut c_void,
        });
        register_methods(
            vm,
            "org/maplibre/nativejni/internal/bridge/StyleNative",
            style_methods,
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
            "mln_runtime_set_resource_provider",
            "mln_resource_request_complete",
            "mln_resource_request_cancelled",
            "mln_resource_request_release",
            "mln_runtime_set_resource_transform",
            "mln_runtime_clear_resource_transform",
        ]);
        methods.push(NativeMethod {
            name: "mln_runtime_create".into(),
            sig: "([J)I".into(),
            fn_ptr: runtime_create as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_destroy".into(),
            sig: "(J)I".into(),
            fn_ptr: runtime_destroy as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_run_once".into(),
            sig: "(J)I".into(),
            fn_ptr: runtime_run_once as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_poll_event".into(),
            sig: "(J[J[I[Z[D[Ljava/lang/String;)I".into(),
            fn_ptr: runtime_poll_event as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_run_ambient_cache_operation_start".into(),
            sig: "(JI[J)I".into(),
            fn_ptr: runtime_run_ambient_cache_operation_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_operation_discard".into(),
            sig: "(JJ)I".into(),
            fn_ptr: runtime_offline_operation_discard as *mut c_void,
        });
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

    fn register_offline(vm: &JavaVM) -> jni::errors::Result<()> {
        let mut methods = no_arg_status_methods(&[
            "mln_runtime_offline_region_create_take_result",
            "mln_runtime_offline_region_get_take_result",
            "mln_runtime_offline_regions_list_take_result",
            "mln_runtime_offline_regions_merge_database_take_result",
            "mln_runtime_offline_region_update_metadata_take_result",
            "mln_offline_region_snapshot_get",
            "mln_offline_region_snapshot_destroy",
            "mln_offline_region_list_count",
            "mln_offline_region_list_get",
            "mln_offline_region_list_destroy",
        ]);
        methods.push(NativeMethod {
            name: "mln_runtime_offline_region_create_start".into(),
            sig: "(JLjava/lang/String;DDDDDDDZ[B[J)I".into(),
            fn_ptr: offline_region_create_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_region_get_start".into(),
            sig: "(JJ[J)I".into(),
            fn_ptr: offline_region_get_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_regions_list_start".into(),
            sig: "(J[J)I".into(),
            fn_ptr: offline_regions_list_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_regions_merge_database_start".into(),
            sig: "(JLjava/lang/String;[J)I".into(),
            fn_ptr: offline_regions_merge_database_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_region_update_metadata_start".into(),
            sig: "(JJ[B[J)I".into(),
            fn_ptr: offline_region_update_metadata_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_region_get_status_start".into(),
            sig: "(JJ[J)I".into(),
            fn_ptr: offline_region_get_status_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_region_set_observed_start".into(),
            sig: "(JJZ[J)I".into(),
            fn_ptr: offline_region_set_observed_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_region_set_download_state_start".into(),
            sig: "(JJI[J)I".into(),
            fn_ptr: offline_region_set_download_state_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_region_invalidate_start".into(),
            sig: "(JJ[J)I".into(),
            fn_ptr: offline_region_invalidate_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_region_delete_start".into(),
            sig: "(JJ[J)I".into(),
            fn_ptr: offline_region_delete_start as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_runtime_offline_region_get_status_take_result".into(),
            sig: "(JJ[J[I[Z)I".into(),
            fn_ptr: offline_region_get_status_take_result as *mut c_void,
        });
        register_methods(
            vm,
            "org/maplibre/nativejni/internal/bridge/OfflineNative",
            methods,
        )
    }

    fn register_map(vm: &JavaVM) -> jni::errors::Result<()> {
        let mut methods = no_arg_status_methods(&["mln_map_options_default"]);
        methods.push(NativeMethod {
            name: "mln_map_create".into(),
            sig: "(JIIDI[J)I".into(),
            fn_ptr: map_create as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_destroy".into(),
            sig: "(J)I".into(),
            fn_ptr: map_destroy as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_request_repaint".into(),
            sig: "(J)I".into(),
            fn_ptr: map_request_repaint as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_request_still_image".into(),
            sig: "(J)I".into(),
            fn_ptr: map_request_still_image as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_set_style_url".into(),
            sig: "(JLjava/lang/String;)I".into(),
            fn_ptr: map_set_style_url as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_set_style_json".into(),
            sig: "(JLjava/lang/String;)I".into(),
            fn_ptr: map_set_style_json as *mut c_void,
        });
        register_methods(
            vm,
            "org/maplibre/nativejni/internal/bridge/MapNative",
            methods,
        )
    }

    fn register_camera(vm: &JavaVM) -> jni::errors::Result<()> {
        let mut methods = no_arg_status_methods(&[
            "mln_camera_options_default",
            "mln_animation_options_default",
            "mln_camera_fit_options_default",
            "mln_bound_options_default",
            "mln_free_camera_options_default",
            "mln_projection_mode_default",
            "mln_map_viewport_options_default",
            "mln_map_tile_options_default",
            "mln_map_camera_for_geometry",
        ]);
        methods.push(NativeMethod {
            name: "mln_map_set_debug_options".into(),
            sig: "(JI)I".into(),
            fn_ptr: map_set_debug_options as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_get_debug_options".into(),
            sig: "(J[I)I".into(),
            fn_ptr: map_get_debug_options as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_set_rendering_stats_view_enabled".into(),
            sig: "(JZ)I".into(),
            fn_ptr: map_set_rendering_stats_view_enabled as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_get_rendering_stats_view_enabled".into(),
            sig: "(J[Z)I".into(),
            fn_ptr: map_get_rendering_stats_view_enabled as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_is_fully_loaded".into(),
            sig: "(J[Z)I".into(),
            fn_ptr: map_is_fully_loaded as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_dump_debug_logs".into(),
            sig: "(J)I".into(),
            fn_ptr: map_dump_debug_logs as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_get_viewport_options".into(),
            sig: "(J[Z[I[D)I".into(),
            fn_ptr: map_get_viewport_options as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_set_viewport_options".into(),
            sig: "(J[Z[I[D)I".into(),
            fn_ptr: map_set_viewport_options as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_get_tile_options".into(),
            sig: "(J[Z[I[D)I".into(),
            fn_ptr: map_get_tile_options as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_set_tile_options".into(),
            sig: "(J[Z[I[D)I".into(),
            fn_ptr: map_set_tile_options as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_camera_for_lat_lng_bounds".into(),
            sig: "(JDDDDZ[Z[D[Z[D)I".into(),
            fn_ptr: map_camera_for_lat_lng_bounds as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_camera_for_lat_lngs".into(),
            sig: "(J[DZ[Z[D[Z[D)I".into(),
            fn_ptr: map_camera_for_lat_lngs as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_lat_lng_bounds_for_camera".into(),
            sig: "(J[Z[D[D)I".into(),
            fn_ptr: map_lat_lng_bounds_for_camera as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_lat_lng_bounds_for_camera_unwrapped".into(),
            sig: "(J[Z[D[D)I".into(),
            fn_ptr: map_lat_lng_bounds_for_camera_unwrapped as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_get_free_camera_options".into(),
            sig: "(J[Z[D)I".into(),
            fn_ptr: map_get_free_camera_options as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_set_free_camera_options".into(),
            sig: "(J[Z[D)I".into(),
            fn_ptr: map_set_free_camera_options as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_get_projection_mode".into(),
            sig: "(J[Z[Z[D)I".into(),
            fn_ptr: map_get_projection_mode as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_set_projection_mode".into(),
            sig: "(J[Z[Z[D)I".into(),
            fn_ptr: map_set_projection_mode as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_get_bounds".into(),
            sig: "(J[Z[D)I".into(),
            fn_ptr: map_get_bounds as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_set_bounds".into(),
            sig: "(J[Z[D)I".into(),
            fn_ptr: map_set_bounds as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_get_camera".into(),
            sig: "(J[Z[D)I".into(),
            fn_ptr: map_get_camera as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_jump_to".into(),
            sig: "(J[Z[D)I".into(),
            fn_ptr: map_jump_to as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_ease_to".into(),
            sig: "(J[Z[DZ[Z[D)I".into(),
            fn_ptr: map_ease_to as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_fly_to".into(),
            sig: "(J[Z[DZ[Z[D)I".into(),
            fn_ptr: map_fly_to as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_move_by".into(),
            sig: "(JDD)I".into(),
            fn_ptr: map_move_by as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_move_by_animated".into(),
            sig: "(JDDZ[Z[D)I".into(),
            fn_ptr: map_move_by_animated as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_scale_by".into(),
            sig: "(JDZDD)I".into(),
            fn_ptr: map_scale_by as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_scale_by_animated".into(),
            sig: "(JDZDDZ[Z[D)I".into(),
            fn_ptr: map_scale_by_animated as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_rotate_by".into(),
            sig: "(JDDDD)I".into(),
            fn_ptr: map_rotate_by as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_rotate_by_animated".into(),
            sig: "(JDDDDZ[Z[D)I".into(),
            fn_ptr: map_rotate_by_animated as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_pitch_by".into(),
            sig: "(JD)I".into(),
            fn_ptr: map_pitch_by as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_pitch_by_animated".into(),
            sig: "(JDZ[Z[D)I".into(),
            fn_ptr: map_pitch_by_animated as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_cancel_transitions".into(),
            sig: "(J)I".into(),
            fn_ptr: map_cancel_transitions as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_pixel_for_lat_lng".into(),
            sig: "(JDD[D)I".into(),
            fn_ptr: map_pixel_for_lat_lng as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_lat_lng_for_pixel".into(),
            sig: "(JDD[D)I".into(),
            fn_ptr: map_lat_lng_for_pixel as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_pixels_for_lat_lngs".into(),
            sig: "(J[D[D)I".into(),
            fn_ptr: map_pixels_for_lat_lngs as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_lat_lngs_for_pixels".into(),
            sig: "(J[D[D)I".into(),
            fn_ptr: map_lat_lngs_for_pixels as *mut c_void,
        });
        register_methods(
            vm,
            "org/maplibre/nativejni/internal/bridge/CameraNative",
            methods,
        )
    }

    fn register_projection(vm: &JavaVM) -> jni::errors::Result<()> {
        let mut methods = no_arg_status_methods(&["mln_map_projection_set_visible_geometry"]);
        methods.push(NativeMethod {
            name: "mln_map_projection_create".into(),
            sig: "(J[J)I".into(),
            fn_ptr: projection_create as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_projection_destroy".into(),
            sig: "(J)I".into(),
            fn_ptr: projection_destroy as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_projection_get_camera".into(),
            sig: "(J[Z[D)I".into(),
            fn_ptr: projection_get_camera as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_projection_set_camera".into(),
            sig: "(J[Z[D)I".into(),
            fn_ptr: projection_set_camera as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_projection_set_visible_coordinates".into(),
            sig: "(J[D[D)I".into(),
            fn_ptr: projection_set_visible_coordinates as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_projection_pixel_for_lat_lng".into(),
            sig: "(JDD[D)I".into(),
            fn_ptr: projection_pixel_for_lat_lng as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_map_projection_lat_lng_for_pixel".into(),
            sig: "(JDD[D)I".into(),
            fn_ptr: projection_lat_lng_for_pixel as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_projected_meters_for_lat_lng".into(),
            sig: "(DD[D)I".into(),
            fn_ptr: projected_meters_for_lat_lng as *mut c_void,
        });
        methods.push(NativeMethod {
            name: "mln_lat_lng_for_projected_meters".into(),
            sig: "(DD[D)I".into(),
            fn_ptr: lat_lng_for_projected_meters as *mut c_void,
        });
        register_methods(
            vm,
            "org/maplibre/nativejni/internal/bridge/ProjectionNative",
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

extern "system" fn runtime_create(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    out_runtime: JLongArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_runtime.is_null() || env.get_array_length(&out_runtime).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }

        let options = unsafe { sys::mln_runtime_options_default() };
        let mut runtime: *mut sys::mln_runtime = std::ptr::null_mut();
        let result = unsafe { sys::mln_runtime_create(&options, &mut runtime) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_long_array_region(&out_runtime, 0, &[runtime as jlong])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn runtime_destroy(_env: JNIEnv<'_>, _class: JClass<'_>, runtime: jlong) -> jint {
    catch_unwind(|| unsafe { sys::mln_runtime_destroy(runtime as *mut sys::mln_runtime) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn runtime_run_once(_env: JNIEnv<'_>, _class: JClass<'_>, runtime: jlong) -> jint {
    catch_unwind(|| unsafe { sys::mln_runtime_run_once(runtime as *mut sys::mln_runtime) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn runtime_run_ambient_cache_operation_start(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    operation: jint,
    out_operation_id: JLongArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_operation_id.is_null() || env.get_array_length(&out_operation_id).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut operation_id: sys::mln_offline_operation_id = 0;
        let result = unsafe {
            sys::mln_runtime_run_ambient_cache_operation_start(
                runtime as *mut sys::mln_runtime,
                operation as u32,
                &mut operation_id,
            )
        };
        if result == sys::MLN_STATUS_OK
            && env
                .set_long_array_region(&out_operation_id, 0, &[operation_id as jlong])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn runtime_offline_operation_discard(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    operation_id: jlong,
) -> jint {
    catch_unwind(|| unsafe {
        sys::mln_runtime_offline_operation_discard(
            runtime as *mut sys::mln_runtime,
            operation_id as sys::mln_offline_operation_id,
        )
    })
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn offline_region_create_start(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    style_url: JString<'_>,
    southwest_latitude: f64,
    southwest_longitude: f64,
    northeast_latitude: f64,
    northeast_longitude: f64,
    min_zoom: f64,
    max_zoom: f64,
    pixel_ratio: f64,
    include_ideographs: jboolean,
    metadata: JByteArray<'_>,
    out_operation_id: JLongArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if metadata.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let style_url = match jstring_to_cstring(&mut env, &style_url) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let metadata = match env.convert_byte_array(&metadata) {
            Ok(value) => value,
            Err(_) => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        let tile_pyramid = sys::mln_offline_tile_pyramid_region_definition {
            size: std::mem::size_of::<sys::mln_offline_tile_pyramid_region_definition>() as u32,
            style_url: style_url.as_ptr(),
            bounds: sys::mln_lat_lng_bounds {
                southwest: sys::mln_lat_lng {
                    latitude: southwest_latitude,
                    longitude: southwest_longitude,
                },
                northeast: sys::mln_lat_lng {
                    latitude: northeast_latitude,
                    longitude: northeast_longitude,
                },
            },
            min_zoom,
            max_zoom,
            pixel_ratio: pixel_ratio as f32,
            include_ideographs: include_ideographs != 0,
        };
        let definition = sys::mln_offline_region_definition {
            size: std::mem::size_of::<sys::mln_offline_region_definition>() as u32,
            type_: sys::MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
            data: sys::mln_offline_region_definition__bindgen_ty_1 { tile_pyramid },
        };
        offline_start_with_out(env, out_operation_id, |out| unsafe {
            sys::mln_runtime_offline_region_create_start(
                runtime as *mut sys::mln_runtime,
                &definition,
                if metadata.is_empty() {
                    std::ptr::null()
                } else {
                    metadata.as_ptr()
                },
                metadata.len(),
                out,
            )
        })
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn offline_region_get_start(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    region_id: jlong,
    out_operation_id: JLongArray<'_>,
) -> jint {
    offline_start_with_out(env, out_operation_id, |out| unsafe {
        sys::mln_runtime_offline_region_get_start(
            runtime as *mut sys::mln_runtime,
            region_id as sys::mln_offline_region_id,
            out,
        )
    })
}

extern "system" fn offline_regions_list_start(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    out_operation_id: JLongArray<'_>,
) -> jint {
    offline_start_with_out(env, out_operation_id, |out| unsafe {
        sys::mln_runtime_offline_regions_list_start(runtime as *mut sys::mln_runtime, out)
    })
}

extern "system" fn offline_regions_merge_database_start(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    side_database_path: JString<'_>,
    out_operation_id: JLongArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let path = match jstring_to_cstring(&mut env, &side_database_path) {
            Ok(path) => path,
            Err(status) => return status,
        };
        offline_start_with_out(env, out_operation_id, |out| unsafe {
            sys::mln_runtime_offline_regions_merge_database_start(
                runtime as *mut sys::mln_runtime,
                path.as_ptr(),
                out,
            )
        })
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn offline_region_update_metadata_start(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    region_id: jlong,
    metadata: JByteArray<'_>,
    out_operation_id: JLongArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if metadata.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let metadata = match env.convert_byte_array(&metadata) {
            Ok(metadata) => metadata,
            Err(_) => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        offline_start_with_out(env, out_operation_id, |out| unsafe {
            sys::mln_runtime_offline_region_update_metadata_start(
                runtime as *mut sys::mln_runtime,
                region_id as sys::mln_offline_region_id,
                if metadata.is_empty() {
                    std::ptr::null()
                } else {
                    metadata.as_ptr()
                },
                metadata.len(),
                out,
            )
        })
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn offline_region_get_status_start(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    region_id: jlong,
    out_operation_id: JLongArray<'_>,
) -> jint {
    offline_start_with_out(env, out_operation_id, |out| unsafe {
        sys::mln_runtime_offline_region_get_status_start(
            runtime as *mut sys::mln_runtime,
            region_id as sys::mln_offline_region_id,
            out,
        )
    })
}

extern "system" fn offline_region_set_observed_start(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    region_id: jlong,
    observed: jboolean,
    out_operation_id: JLongArray<'_>,
) -> jint {
    offline_start_with_out(env, out_operation_id, |out| unsafe {
        sys::mln_runtime_offline_region_set_observed_start(
            runtime as *mut sys::mln_runtime,
            region_id as sys::mln_offline_region_id,
            observed != 0,
            out,
        )
    })
}

extern "system" fn offline_region_set_download_state_start(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    region_id: jlong,
    state: jint,
    out_operation_id: JLongArray<'_>,
) -> jint {
    offline_start_with_out(env, out_operation_id, |out| unsafe {
        sys::mln_runtime_offline_region_set_download_state_start(
            runtime as *mut sys::mln_runtime,
            region_id as sys::mln_offline_region_id,
            state as u32,
            out,
        )
    })
}

extern "system" fn offline_region_invalidate_start(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    region_id: jlong,
    out_operation_id: JLongArray<'_>,
) -> jint {
    offline_start_with_out(env, out_operation_id, |out| unsafe {
        sys::mln_runtime_offline_region_invalidate_start(
            runtime as *mut sys::mln_runtime,
            region_id as sys::mln_offline_region_id,
            out,
        )
    })
}

extern "system" fn offline_region_delete_start(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    region_id: jlong,
    out_operation_id: JLongArray<'_>,
) -> jint {
    offline_start_with_out(env, out_operation_id, |out| unsafe {
        sys::mln_runtime_offline_region_delete_start(
            runtime as *mut sys::mln_runtime,
            region_id as sys::mln_offline_region_id,
            out,
        )
    })
}

extern "system" fn offline_region_get_status_take_result(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    operation_id: jlong,
    longs: JLongArray<'_>,
    ints: JIntArray<'_>,
    booleans: JBooleanArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if longs.is_null()
            || ints.is_null()
            || booleans.is_null()
            || env.get_array_length(&longs).unwrap_or(0) < 6
            || env.get_array_length(&ints).unwrap_or(0) < 1
            || env.get_array_length(&booleans).unwrap_or(0) < 2
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut status = sys::mln_offline_region_status {
            size: std::mem::size_of::<sys::mln_offline_region_status>() as u32,
            download_state: 0,
            completed_resource_count: 0,
            completed_resource_size: 0,
            completed_tile_count: 0,
            required_tile_count: 0,
            completed_tile_size: 0,
            required_resource_count: 0,
            required_resource_count_is_precise: false,
            complete: false,
        };
        let result = unsafe {
            sys::mln_runtime_offline_region_get_status_take_result(
                runtime as *mut sys::mln_runtime,
                operation_id as sys::mln_offline_operation_id,
                &mut status,
            )
        };
        if result == sys::MLN_STATUS_OK
            && (env
                .set_long_array_region(
                    &longs,
                    0,
                    &[
                        status.completed_resource_count as jlong,
                        status.completed_resource_size as jlong,
                        status.completed_tile_count as jlong,
                        status.required_tile_count as jlong,
                        status.completed_tile_size as jlong,
                        status.required_resource_count as jlong,
                    ],
                )
                .is_err()
                || env
                    .set_int_array_region(&ints, 0, &[status.download_state as jint])
                    .is_err()
                || env
                    .set_boolean_array_region(
                        &booleans,
                        0,
                        &[
                            jboolean::from(status.required_resource_count_is_precise),
                            jboolean::from(status.complete),
                        ],
                    )
                    .is_err())
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn offline_start_with_out(
    env: JNIEnv<'_>,
    out_operation_id: JLongArray<'_>,
    start: impl FnOnce(*mut sys::mln_offline_operation_id) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_operation_id.is_null() || env.get_array_length(&out_operation_id).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut operation_id: sys::mln_offline_operation_id = 0;
        let result = start(&mut operation_id);
        if result == sys::MLN_STATUS_OK
            && env
                .set_long_array_region(&out_operation_id, 0, &[operation_id as jlong])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn runtime_poll_event(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    longs: JLongArray<'_>,
    ints: JIntArray<'_>,
    booleans: JBooleanArray<'_>,
    doubles: JDoubleArray<'_>,
    strings: JObjectArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !runtime_event_arrays_are_valid(&env, &longs, &ints, &booleans, &doubles, &strings) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }

        let mut event: sys::mln_runtime_event = unsafe { std::mem::zeroed() };
        event.size = std::mem::size_of::<sys::mln_runtime_event>() as u32;
        let mut has_event = false;
        let result = unsafe {
            sys::mln_runtime_poll_event(
                runtime as *mut sys::mln_runtime,
                &mut event,
                &mut has_event,
            )
        };
        if result != sys::MLN_STATUS_OK {
            return result;
        }

        let mut long_values = [0 as jlong; LONG_COUNT];
        let mut int_values = [0 as jint; INT_COUNT];
        let mut boolean_values = [0 as jboolean; BOOLEAN_COUNT];
        let mut double_values = [0.0_f64; DOUBLE_COUNT];
        boolean_values[BOOLEAN_HAS_EVENT] = jboolean::from(has_event);
        if !has_event {
            return if env
                .set_boolean_array_region(&booleans, 0, &boolean_values)
                .is_ok()
            {
                sys::MLN_STATUS_OK
            } else {
                sys::MLN_STATUS_INVALID_ARGUMENT
            };
        }

        long_values[LONG_SOURCE_ADDRESS] = event.source as jlong;
        long_values[LONG_PAYLOAD_SIZE] = event.payload_size as jlong;
        int_values[INT_EVENT_TYPE] = event.type_ as jint;
        int_values[INT_SOURCE_TYPE] = event.source_type as jint;
        int_values[INT_CODE] = event.code as jint;
        int_values[INT_PAYLOAD_TYPE] = event.payload_type as jint;

        fill_runtime_event_payload(
            &event,
            &mut long_values,
            &mut int_values,
            &mut boolean_values,
            &mut double_values,
        );

        if env.set_long_array_region(&longs, 0, &long_values).is_err()
            || env.set_int_array_region(&ints, 0, &int_values).is_err()
            || env
                .set_boolean_array_region(&booleans, 0, &boolean_values)
                .is_err()
            || env
                .set_double_array_region(&doubles, 0, &double_values)
                .is_err()
            || set_string_array_element(&env, &strings, STRING_MESSAGE, unsafe {
                copy_string(event.message, event.message_size)
            })
            .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }

        let payload_string = payload_string(&event);
        if set_string_array_element(&env, &strings, STRING_PAYLOAD, payload_string).is_err() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        sys::MLN_STATUS_OK
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn runtime_event_arrays_are_valid(
    env: &JNIEnv<'_>,
    longs: &JLongArray<'_>,
    ints: &JIntArray<'_>,
    booleans: &JBooleanArray<'_>,
    doubles: &JDoubleArray<'_>,
    strings: &JObjectArray<'_>,
) -> bool {
    !longs.is_null()
        && !ints.is_null()
        && !booleans.is_null()
        && !doubles.is_null()
        && !strings.is_null()
        && env.get_array_length(longs).unwrap_or(0) >= LONG_COUNT as i32
        && env.get_array_length(ints).unwrap_or(0) >= INT_COUNT as i32
        && env.get_array_length(booleans).unwrap_or(0) >= BOOLEAN_COUNT as i32
        && env.get_array_length(doubles).unwrap_or(0) >= DOUBLE_COUNT as i32
        && env.get_array_length(strings).unwrap_or(0) >= STRING_COUNT
}

fn fill_runtime_event_payload(
    event: &sys::mln_runtime_event,
    longs: &mut [jlong; LONG_COUNT],
    ints: &mut [jint; INT_COUNT],
    booleans: &mut [jboolean; BOOLEAN_COUNT],
    doubles: &mut [f64; DOUBLE_COUNT],
) {
    if event.payload_type == sys::MLN_RUNTIME_EVENT_PAYLOAD_NONE {
        ints[INT_PAYLOAD_AVAILABLE] = 1;
        return;
    }
    if event.payload.is_null() {
        return;
    }

    unsafe {
        match event.payload_type {
            sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME
                if event.payload_size
                    >= std::mem::size_of::<sys::mln_runtime_event_render_frame>() =>
            {
                let payload = &*(event.payload as *const sys::mln_runtime_event_render_frame);
                ints[INT_PAYLOAD_AVAILABLE] = 1;
                ints[INT_RENDER_MODE] = payload.mode as jint;
                booleans[BOOLEAN_NEEDS_REPAINT] = jboolean::from(payload.needs_repaint);
                booleans[BOOLEAN_PLACEMENT_CHANGED] = jboolean::from(payload.placement_changed);
                doubles[DOUBLE_ENCODING_TIME] = payload.stats.encoding_time;
                doubles[DOUBLE_RENDERING_TIME] = payload.stats.rendering_time;
                longs[LONG_FRAME_COUNT] = payload.stats.frame_count as jlong;
                longs[LONG_DRAW_CALL_COUNT] = payload.stats.draw_call_count as jlong;
                longs[LONG_TOTAL_DRAW_CALL_COUNT] = payload.stats.total_draw_call_count as jlong;
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP
                if event.payload_size
                    >= std::mem::size_of::<sys::mln_runtime_event_render_map>() =>
            {
                let payload = &*(event.payload as *const sys::mln_runtime_event_render_map);
                ints[INT_PAYLOAD_AVAILABLE] = 1;
                ints[INT_RENDER_MODE] = payload.mode as jint;
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING
                if event.payload_size
                    >= std::mem::size_of::<sys::mln_runtime_event_style_image_missing>() =>
            {
                ints[INT_PAYLOAD_AVAILABLE] = 1;
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION
                if event.payload_size
                    >= std::mem::size_of::<sys::mln_runtime_event_tile_action>() =>
            {
                let payload = &*(event.payload as *const sys::mln_runtime_event_tile_action);
                ints[INT_PAYLOAD_AVAILABLE] = 1;
                ints[INT_TILE_OPERATION] = payload.operation as jint;
                ints[INT_TILE_WRAP] = payload.tile_id.wrap as jint;
                longs[LONG_TILE_OVERSCALED_Z] = payload.tile_id.overscaled_z as jlong;
                longs[LONG_TILE_CANONICAL_Z] = payload.tile_id.canonical_z as jlong;
                longs[LONG_TILE_CANONICAL_X] = payload.tile_id.canonical_x as jlong;
                longs[LONG_TILE_CANONICAL_Y] = payload.tile_id.canonical_y as jlong;
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS
                if event.payload_size
                    >= std::mem::size_of::<sys::mln_runtime_event_offline_region_status>() =>
            {
                let payload =
                    &*(event.payload as *const sys::mln_runtime_event_offline_region_status);
                ints[INT_PAYLOAD_AVAILABLE] = 1;
                ints[INT_OFFLINE_DOWNLOAD_STATE] = payload.status.download_state as jint;
                longs[LONG_REGION_ID] = payload.region_id as jlong;
                longs[LONG_COMPLETED_RESOURCE_COUNT] =
                    payload.status.completed_resource_count as jlong;
                longs[LONG_COMPLETED_RESOURCE_SIZE] =
                    payload.status.completed_resource_size as jlong;
                longs[LONG_COMPLETED_TILE_COUNT] = payload.status.completed_tile_count as jlong;
                longs[LONG_REQUIRED_TILE_COUNT] = payload.status.required_tile_count as jlong;
                longs[LONG_COMPLETED_TILE_SIZE] = payload.status.completed_tile_size as jlong;
                longs[LONG_REQUIRED_RESOURCE_COUNT] =
                    payload.status.required_resource_count as jlong;
                booleans[BOOLEAN_REQUIRED_RESOURCE_COUNT_IS_PRECISE] =
                    jboolean::from(payload.status.required_resource_count_is_precise);
                booleans[BOOLEAN_COMPLETE] = jboolean::from(payload.status.complete);
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR
                if event.payload_size
                    >= std::mem::size_of::<sys::mln_runtime_event_offline_region_response_error>(
                    ) =>
            {
                let payload = &*(event.payload
                    as *const sys::mln_runtime_event_offline_region_response_error);
                ints[INT_PAYLOAD_AVAILABLE] = 1;
                ints[INT_RESOURCE_ERROR_REASON] = payload.reason as jint;
                longs[LONG_REGION_ID] = payload.region_id as jlong;
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT
                if event.payload_size
                    >= std::mem::size_of::<
                        sys::mln_runtime_event_offline_region_tile_count_limit,
                    >() =>
            {
                let payload = &*(event.payload
                    as *const sys::mln_runtime_event_offline_region_tile_count_limit);
                ints[INT_PAYLOAD_AVAILABLE] = 1;
                longs[LONG_REGION_ID] = payload.region_id as jlong;
                longs[LONG_LIMIT] = payload.limit as jlong;
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED
                if event.payload_size
                    >= std::mem::size_of::<sys::mln_runtime_event_offline_operation_completed>(
                    ) =>
            {
                let payload =
                    &*(event.payload as *const sys::mln_runtime_event_offline_operation_completed);
                ints[INT_PAYLOAD_AVAILABLE] = 1;
                ints[INT_OFFLINE_OPERATION_KIND] = payload.operation_kind as jint;
                ints[INT_OFFLINE_RESULT_KIND] = payload.result_kind as jint;
                ints[INT_OFFLINE_RESULT_STATUS] = payload.result_status as jint;
                longs[LONG_OPERATION_ID] = payload.operation_id as jlong;
                booleans[BOOLEAN_FOUND] = jboolean::from(payload.found);
            }
            _ => {}
        }
    }
}

fn payload_string(event: &sys::mln_runtime_event) -> String {
    if event.payload.is_null() {
        return String::new();
    }
    unsafe {
        match event.payload_type {
            sys::MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING
                if event.payload_size
                    >= std::mem::size_of::<sys::mln_runtime_event_style_image_missing>() =>
            {
                let payload =
                    &*(event.payload as *const sys::mln_runtime_event_style_image_missing);
                copy_string(payload.image_id, payload.image_id_size)
            }
            sys::MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION
                if event.payload_size
                    >= std::mem::size_of::<sys::mln_runtime_event_tile_action>() =>
            {
                let payload = &*(event.payload as *const sys::mln_runtime_event_tile_action);
                copy_string(payload.source_id, payload.source_id_size)
            }
            _ => String::new(),
        }
    }
}

unsafe fn copy_string(ptr: *const std::os::raw::c_char, len: usize) -> String {
    if ptr.is_null() || len == 0 {
        return String::new();
    }
    let bytes = unsafe { std::slice::from_raw_parts(ptr.cast::<u8>(), len) };
    String::from_utf8_lossy(bytes).into_owned()
}

fn set_string_array_element(
    env: &JNIEnv<'_>,
    strings: &JObjectArray<'_>,
    index: i32,
    value: String,
) -> jni::errors::Result<()> {
    let java_string = env.new_string(value)?;
    env.set_object_array_element(strings, index, &java_string)
}

extern "system" fn map_create(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    runtime: jlong,
    width: jint,
    height: jint,
    scale_factor: f64,
    map_mode: jint,
    out_map: JLongArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_map.is_null() || env.get_array_length(&out_map).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        if width < 0 || height < 0 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }

        let mut options = unsafe { sys::mln_map_options_default() };
        options.width = width as u32;
        options.height = height as u32;
        options.scale_factor = scale_factor;
        options.map_mode = map_mode as u32;
        let mut map: *mut sys::mln_map = std::ptr::null_mut();
        let result =
            unsafe { sys::mln_map_create(runtime as *mut sys::mln_runtime, &options, &mut map) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_long_array_region(&out_map, 0, &[map as jlong])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_destroy(_env: JNIEnv<'_>, _class: JClass<'_>, map: jlong) -> jint {
    catch_unwind(|| unsafe { sys::mln_map_destroy(map as *mut sys::mln_map) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_request_repaint(_env: JNIEnv<'_>, _class: JClass<'_>, map: jlong) -> jint {
    catch_unwind(|| unsafe { sys::mln_map_request_repaint(map as *mut sys::mln_map) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_request_still_image(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
) -> jint {
    catch_unwind(|| unsafe { sys::mln_map_request_still_image(map as *mut sys::mln_map) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_style_url(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    url: JString<'_>,
) -> jint {
    map_set_style_string(env, map, url, sys::mln_map_set_style_url)
}

extern "system" fn map_set_style_json(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    json: JString<'_>,
) -> jint {
    map_set_style_string(env, map, json, sys::mln_map_set_style_json)
}

fn map_set_style_string(
    mut env: JNIEnv<'_>,
    map: jlong,
    value: JString<'_>,
    setter: unsafe extern "C" fn(*mut sys::mln_map, *const std::os::raw::c_char) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if value.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let java_string = match env.get_string(&value) {
            Ok(value) => value,
            Err(_) => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        let string = match CString::new(String::from(java_string)) {
            Ok(value) => value,
            Err(_) => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        unsafe { setter(map as *mut sys::mln_map, string.as_ptr()) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_list_style_source_ids(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_source_ids: JObjectArray<'_>,
) -> jint {
    map_list_style_ids(env, map, out_source_ids, sys::mln_map_list_style_source_ids)
}

extern "system" fn map_add_geojson_source_url(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    url: JString<'_>,
) -> jint {
    map_style_source_url(
        env,
        map,
        source_id,
        url,
        sys::mln_map_add_geojson_source_url,
    )
}

extern "system" fn map_set_geojson_source_url(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    url: JString<'_>,
) -> jint {
    map_style_source_url(
        env,
        map,
        source_id,
        url,
        sys::mln_map_set_geojson_source_url,
    )
}

extern "system" fn map_add_vector_source_url(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    url: JString<'_>,
    option_fields: JBooleanArray<'_>,
    option_values: JDoubleArray<'_>,
    attribution: JString<'_>,
) -> jint {
    map_tile_source_url(
        env,
        map,
        source_id,
        url,
        option_fields,
        option_values,
        attribution,
        sys::mln_map_add_vector_source_url,
    )
}

extern "system" fn map_add_raster_source_url(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    url: JString<'_>,
    option_fields: JBooleanArray<'_>,
    option_values: JDoubleArray<'_>,
    attribution: JString<'_>,
) -> jint {
    map_tile_source_url(
        env,
        map,
        source_id,
        url,
        option_fields,
        option_values,
        attribution,
        sys::mln_map_add_raster_source_url,
    )
}

extern "system" fn map_add_raster_dem_source_url(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    url: JString<'_>,
    option_fields: JBooleanArray<'_>,
    option_values: JDoubleArray<'_>,
    attribution: JString<'_>,
) -> jint {
    map_tile_source_url(
        env,
        map,
        source_id,
        url,
        option_fields,
        option_values,
        attribution,
        sys::mln_map_add_raster_dem_source_url,
    )
}

extern "system" fn map_add_vector_source_tiles(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    tiles: JObjectArray<'_>,
    option_fields: JBooleanArray<'_>,
    option_values: JDoubleArray<'_>,
    attribution: JString<'_>,
) -> jint {
    map_tile_source_tiles(
        env,
        map,
        source_id,
        tiles,
        option_fields,
        option_values,
        attribution,
        sys::mln_map_add_vector_source_tiles,
    )
}

extern "system" fn map_add_raster_source_tiles(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    tiles: JObjectArray<'_>,
    option_fields: JBooleanArray<'_>,
    option_values: JDoubleArray<'_>,
    attribution: JString<'_>,
) -> jint {
    map_tile_source_tiles(
        env,
        map,
        source_id,
        tiles,
        option_fields,
        option_values,
        attribution,
        sys::mln_map_add_raster_source_tiles,
    )
}

extern "system" fn map_add_raster_dem_source_tiles(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    tiles: JObjectArray<'_>,
    option_fields: JBooleanArray<'_>,
    option_values: JDoubleArray<'_>,
    attribution: JString<'_>,
) -> jint {
    map_tile_source_tiles(
        env,
        map,
        source_id,
        tiles,
        option_fields,
        option_values,
        attribution,
        sys::mln_map_add_raster_dem_source_tiles,
    )
}

fn map_tile_source_tiles(
    mut env: JNIEnv<'_>,
    map: jlong,
    source_id: JString<'_>,
    tiles: JObjectArray<'_>,
    option_fields: JBooleanArray<'_>,
    option_values: JDoubleArray<'_>,
    attribution: JString<'_>,
    operation: unsafe extern "C" fn(
        *mut sys::mln_map,
        sys::mln_string_view,
        *const sys::mln_string_view,
        usize,
        *const sys::mln_style_tile_source_options,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if tiles.is_null() {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let tile_count = match env.get_array_length(&tiles) {
            Ok(value) if value >= 0 => value as usize,
            _ => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        let (source_id, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (options, attribution_storage) = match read_tile_source_options(
            &mut env,
            &option_fields,
            &option_values,
            &attribution,
        ) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let mut tile_storage = Vec::with_capacity(tile_count);
        let mut tile_views = Vec::with_capacity(tile_count);
        for index in 0..tile_count {
            let tile = match env.get_object_array_element(&tiles, index as i32) {
                Ok(value) => JString::from(value),
                Err(_) => return sys::MLN_STATUS_INVALID_ARGUMENT,
            };
            let (tile_storage_value, tile_view) = match string_view(&mut env, &tile) {
                Ok(value) => value,
                Err(status) => return status,
            };
            tile_storage.push(tile_storage_value);
            tile_views.push(tile_view);
        }
        let _keep_alive = (source_id, attribution_storage, tile_storage);
        let tile_ptr = if tile_views.is_empty() {
            std::ptr::null()
        } else {
            tile_views.as_ptr()
        };
        unsafe {
            operation(
                map as *mut sys::mln_map,
                source_id_view,
                tile_ptr,
                tile_views.len(),
                &options,
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn map_tile_source_url(
    mut env: JNIEnv<'_>,
    map: jlong,
    source_id: JString<'_>,
    url: JString<'_>,
    option_fields: JBooleanArray<'_>,
    option_values: JDoubleArray<'_>,
    attribution: JString<'_>,
    operation: unsafe extern "C" fn(
        *mut sys::mln_map,
        sys::mln_string_view,
        sys::mln_string_view,
        *const sys::mln_style_tile_source_options,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (source_id, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (url, url_view) = match string_view(&mut env, &url) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (options, attribution_storage) = match read_tile_source_options(
            &mut env,
            &option_fields,
            &option_values,
            &attribution,
        ) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _keep_alive = (source_id, url, attribution_storage);
        unsafe { operation(map as *mut sys::mln_map, source_id_view, url_view, &options) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn read_tile_source_options(
    env: &mut JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
    attribution: &JString<'_>,
) -> Result<(sys::mln_style_tile_source_options, Option<CString>), jint> {
    if fields.is_null()
        || values.is_null()
        || env.get_array_length(fields).unwrap_or(0) < 8
        || env.get_array_length(values).unwrap_or(0) < 10
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut field_values = [0 as jboolean; 8];
    let mut option_values = [0.0_f64; 10];
    if env
        .get_boolean_array_region(fields, 0, &mut field_values)
        .is_err()
        || env
            .get_double_array_region(values, 0, &mut option_values)
            .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut options = unsafe { sys::mln_style_tile_source_options_default() };
    let mut attribution_storage = None;
    if field_values[0] != 0 {
        options.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM;
        options.min_zoom = option_values[0];
    }
    if field_values[1] != 0 {
        options.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM;
        options.max_zoom = option_values[1];
    }
    if field_values[2] != 0 {
        let (storage, view) = string_view(env, attribution)?;
        options.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION;
        options.attribution = view;
        attribution_storage = Some(storage);
    }
    if field_values[3] != 0 {
        options.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_SCHEME;
        options.scheme = option_values[6] as u32;
    }
    if field_values[4] != 0 {
        options.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS;
        options.bounds = sys::mln_lat_lng_bounds {
            southwest: sys::mln_lat_lng {
                latitude: option_values[2],
                longitude: option_values[3],
            },
            northeast: sys::mln_lat_lng {
                latitude: option_values[4],
                longitude: option_values[5],
            },
        };
    }
    if field_values[5] != 0 {
        options.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE;
        options.tile_size = option_values[7] as u32;
    }
    if field_values[6] != 0 {
        options.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
        options.vector_encoding = option_values[8] as u32;
    }
    if field_values[7] != 0 {
        options.fields |= sys::MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
        options.raster_encoding = option_values[9] as u32;
    }
    Ok((options, attribution_storage))
}

fn map_style_source_url(
    mut env: JNIEnv<'_>,
    map: jlong,
    source_id: JString<'_>,
    url: JString<'_>,
    operation: unsafe extern "C" fn(
        *mut sys::mln_map,
        sys::mln_string_view,
        sys::mln_string_view,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (source_id, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (url, url_view) = match string_view(&mut env, &url) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _source_id = source_id;
        let _url = url;
        unsafe { operation(map as *mut sys::mln_map, source_id_view, url_view) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_remove_style_source(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    out_removed: JBooleanArray<'_>,
) -> jint {
    map_style_source_bool(
        env,
        map,
        source_id,
        out_removed,
        sys::mln_map_remove_style_source,
    )
}

extern "system" fn map_style_source_exists(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    out_exists: JBooleanArray<'_>,
) -> jint {
    map_style_source_bool(
        env,
        map,
        source_id,
        out_exists,
        sys::mln_map_style_source_exists,
    )
}

extern "system" fn map_get_style_source_type(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    out_source_type: JIntArray<'_>,
    out_found: JBooleanArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_source_type.is_null()
            || out_found.is_null()
            || env.get_array_length(&out_source_type).unwrap_or(0) < 1
            || env.get_array_length(&out_found).unwrap_or(0) < 1
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let (source_id, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _source_id = source_id;
        let mut source_type = 0_u32;
        let mut found = false;
        let result = unsafe {
            sys::mln_map_get_style_source_type(
                map as *mut sys::mln_map,
                source_id_view,
                &mut source_type,
                &mut found,
            )
        };
        if result == sys::MLN_STATUS_OK
            && (env
                .set_int_array_region(&out_source_type, 0, &[source_type as jint])
                .is_err()
                || env
                    .set_boolean_array_region(&out_found, 0, &[jboolean::from(found)])
                    .is_err())
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_style_source_info(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    out_info: JIntArray<'_>,
    out_flags: JBooleanArray<'_>,
    out_sizes: JLongArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_info.is_null()
            || out_flags.is_null()
            || out_sizes.is_null()
            || env.get_array_length(&out_info).unwrap_or(0) < 1
            || env.get_array_length(&out_flags).unwrap_or(0) < 3
            || env.get_array_length(&out_sizes).unwrap_or(0) < 1
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let (source_id, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _source_id = source_id;
        let mut info = sys::mln_style_source_info {
            size: std::mem::size_of::<sys::mln_style_source_info>() as u32,
            type_: 0,
            id_size: 0,
            is_volatile: false,
            has_attribution: false,
            attribution_size: 0,
        };
        let mut found = false;
        let result = unsafe {
            sys::mln_map_get_style_source_info(
                map as *mut sys::mln_map,
                source_id_view,
                &mut info,
                &mut found,
            )
        };
        if result == sys::MLN_STATUS_OK
            && (env
                .set_int_array_region(&out_info, 0, &[info.type_ as jint])
                .is_err()
                || env
                    .set_boolean_array_region(
                        &out_flags,
                        0,
                        &[
                            jboolean::from(found),
                            jboolean::from(info.is_volatile),
                            jboolean::from(info.has_attribution),
                        ],
                    )
                    .is_err()
                || env
                    .set_long_array_region(&out_sizes, 0, &[info.attribution_size as jlong])
                    .is_err())
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_copy_style_source_attribution(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    out_attribution: JByteArray<'_>,
    out_attribution_size: JLongArray<'_>,
    out_found: JBooleanArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_attribution.is_null()
            || out_attribution_size.is_null()
            || out_found.is_null()
            || env.get_array_length(&out_attribution_size).unwrap_or(0) < 1
            || env.get_array_length(&out_found).unwrap_or(0) < 1
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let capacity = env.get_array_length(&out_attribution).unwrap_or(-1);
        if capacity < 0 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let (source_id, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _source_id = source_id;
        let mut buffer = vec![0_i8; capacity as usize];
        let mut attribution_size = 0_usize;
        let mut found = false;
        let out_ptr = if buffer.is_empty() {
            std::ptr::null_mut()
        } else {
            buffer.as_mut_ptr() as *mut c_char
        };
        let result = unsafe {
            sys::mln_map_copy_style_source_attribution(
                map as *mut sys::mln_map,
                source_id_view,
                out_ptr,
                buffer.len(),
                &mut attribution_size,
                &mut found,
            )
        };
        if result == sys::MLN_STATUS_OK {
            if attribution_size > buffer.len() {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
            if attribution_size > 0
                && env
                    .set_byte_array_region(&out_attribution, 0, &buffer[..attribution_size])
                    .is_err()
            {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
            if env
                .set_long_array_region(&out_attribution_size, 0, &[attribution_size as jlong])
                .is_err()
                || env
                    .set_boolean_array_region(&out_found, 0, &[jboolean::from(found)])
                    .is_err()
            {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn map_style_source_bool(
    mut env: JNIEnv<'_>,
    map: jlong,
    source_id: JString<'_>,
    out_value: JBooleanArray<'_>,
    operation: unsafe extern "C" fn(
        *mut sys::mln_map,
        sys::mln_string_view,
        *mut bool,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_value.is_null() || env.get_array_length(&out_value).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let (source_id, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _source_id = source_id;
        let mut value = false;
        let result = unsafe { operation(map as *mut sys::mln_map, source_id_view, &mut value) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_boolean_array_region(&out_value, 0, &[jboolean::from(value)])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_list_style_layer_ids(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_layer_ids: JObjectArray<'_>,
) -> jint {
    map_list_style_ids(env, map, out_layer_ids, sys::mln_map_list_style_layer_ids)
}

extern "system" fn map_set_style_image(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    image_id: JString<'_>,
    width: jint,
    height: jint,
    stride: jint,
    pixels: JByteArray<'_>,
    has_pixel_ratio: jboolean,
    pixel_ratio: f64,
    has_sdf: jboolean,
    sdf: jboolean,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if pixels.is_null() || width < 0 || height < 0 || stride < 0 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let pixel_count = match env.get_array_length(&pixels) {
            Ok(value) if value >= 0 => value as usize,
            _ => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        let mut pixel_values = vec![0_i8; pixel_count];
        if env
            .get_byte_array_region(&pixels, 0, &mut pixel_values)
            .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let (image_id_storage, image_id_view) = match string_view(&mut env, &image_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let mut options = unsafe { sys::mln_style_image_options_default() };
        if has_pixel_ratio != 0 {
            options.fields |= sys::MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO;
            options.pixel_ratio = pixel_ratio as f32;
        }
        if has_sdf != 0 {
            options.fields |= sys::MLN_STYLE_IMAGE_OPTION_SDF;
            options.sdf = sdf != 0;
        }
        let image = sys::mln_premultiplied_rgba8_image {
            size: std::mem::size_of::<sys::mln_premultiplied_rgba8_image>() as u32,
            width: width as u32,
            height: height as u32,
            stride: stride as u32,
            pixels: if pixel_values.is_empty() {
                std::ptr::null()
            } else {
                pixel_values.as_ptr() as *const u8
            },
            byte_length: pixel_values.len(),
        };
        let _keep_alive = (image_id_storage, pixel_values);
        unsafe {
            sys::mln_map_set_style_image(map as *mut sys::mln_map, image_id_view, &image, &options)
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn premultiplied_rgba8_image(
    env: &JNIEnv<'_>,
    pixels: &JByteArray<'_>,
    width: jint,
    height: jint,
    stride: jint,
) -> Result<(sys::mln_premultiplied_rgba8_image, Vec<i8>), jint> {
    if pixels.is_null() || width < 0 || height < 0 || stride < 0 {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let pixel_count = env
        .get_array_length(pixels)
        .map_err(|_| sys::MLN_STATUS_INVALID_ARGUMENT)?;
    if pixel_count < 0 {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut pixel_values = vec![0_i8; pixel_count as usize];
    if env
        .get_byte_array_region(pixels, 0, &mut pixel_values)
        .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let image = sys::mln_premultiplied_rgba8_image {
        size: std::mem::size_of::<sys::mln_premultiplied_rgba8_image>() as u32,
        width: width as u32,
        height: height as u32,
        stride: stride as u32,
        pixels: if pixel_values.is_empty() {
            std::ptr::null()
        } else {
            pixel_values.as_ptr() as *const u8
        },
        byte_length: pixel_values.len(),
    };
    Ok((image, pixel_values))
}

extern "system" fn map_remove_style_image(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    image_id: JString<'_>,
    out_removed: JBooleanArray<'_>,
) -> jint {
    map_style_image_bool(
        env,
        map,
        image_id,
        out_removed,
        sys::mln_map_remove_style_image,
    )
}

extern "system" fn map_style_image_exists(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    image_id: JString<'_>,
    out_exists: JBooleanArray<'_>,
) -> jint {
    map_style_image_bool(
        env,
        map,
        image_id,
        out_exists,
        sys::mln_map_style_image_exists,
    )
}

fn map_style_image_bool(
    mut env: JNIEnv<'_>,
    map: jlong,
    image_id: JString<'_>,
    out_value: JBooleanArray<'_>,
    operation: unsafe extern "C" fn(
        *mut sys::mln_map,
        sys::mln_string_view,
        *mut bool,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_value.is_null() || env.get_array_length(&out_value).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let (image_id_storage, image_id_view) = match string_view(&mut env, &image_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _keep_alive = image_id_storage;
        let mut value = false;
        let result = unsafe { operation(map as *mut sys::mln_map, image_id_view, &mut value) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_boolean_array_region(&out_value, 0, &[jboolean::from(value)])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_style_image_info(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    image_id: JString<'_>,
    out_info: JIntArray<'_>,
    out_byte_length: JLongArray<'_>,
    out_pixel_ratio: JDoubleArray<'_>,
    out_flags: JBooleanArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_info.is_null()
            || out_byte_length.is_null()
            || out_pixel_ratio.is_null()
            || out_flags.is_null()
            || env.get_array_length(&out_info).unwrap_or(0) < 3
            || env.get_array_length(&out_byte_length).unwrap_or(0) < 1
            || env.get_array_length(&out_pixel_ratio).unwrap_or(0) < 1
            || env.get_array_length(&out_flags).unwrap_or(0) < 2
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let (image_id_storage, image_id_view) = match string_view(&mut env, &image_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _keep_alive = image_id_storage;
        let mut info = unsafe { sys::mln_style_image_info_default() };
        let mut found = false;
        let result = unsafe {
            sys::mln_map_get_style_image_info(
                map as *mut sys::mln_map,
                image_id_view,
                &mut info,
                &mut found,
            )
        };
        if result == sys::MLN_STATUS_OK
            && (env
                .set_int_array_region(
                    &out_info,
                    0,
                    &[info.width as jint, info.height as jint, info.stride as jint],
                )
                .is_err()
                || env
                    .set_long_array_region(&out_byte_length, 0, &[info.byte_length as jlong])
                    .is_err()
                || env
                    .set_double_array_region(&out_pixel_ratio, 0, &[info.pixel_ratio as f64])
                    .is_err()
                || env
                    .set_boolean_array_region(
                        &out_flags,
                        0,
                        &[jboolean::from(found), jboolean::from(info.sdf)],
                    )
                    .is_err())
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_copy_style_image_premultiplied_rgba8(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    image_id: JString<'_>,
    out_pixels: JByteArray<'_>,
    out_byte_length: JLongArray<'_>,
    out_found: JBooleanArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_pixels.is_null()
            || out_byte_length.is_null()
            || out_found.is_null()
            || env.get_array_length(&out_byte_length).unwrap_or(0) < 1
            || env.get_array_length(&out_found).unwrap_or(0) < 1
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let capacity = match env.get_array_length(&out_pixels) {
            Ok(value) if value >= 0 => value as usize,
            _ => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        let (image_id_storage, image_id_view) = match string_view(&mut env, &image_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _keep_alive = image_id_storage;
        let mut buffer = vec![0_u8; capacity];
        let mut byte_length = 0_usize;
        let mut found = false;
        let result = unsafe {
            sys::mln_map_copy_style_image_premultiplied_rgba8(
                map as *mut sys::mln_map,
                image_id_view,
                if buffer.is_empty() {
                    std::ptr::null_mut()
                } else {
                    buffer.as_mut_ptr()
                },
                buffer.len(),
                &mut byte_length,
                &mut found,
            )
        };
        if result == sys::MLN_STATUS_OK {
            if byte_length > buffer.len() {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
            if byte_length > 0 {
                let signed_pixels: Vec<i8> = buffer[..byte_length]
                    .iter()
                    .copied()
                    .map(|value| value as i8)
                    .collect();
                if env
                    .set_byte_array_region(&out_pixels, 0, &signed_pixels)
                    .is_err()
                {
                    return sys::MLN_STATUS_INVALID_ARGUMENT;
                }
            }
            if env
                .set_long_array_region(&out_byte_length, 0, &[byte_length as jlong])
                .is_err()
                || env
                    .set_boolean_array_region(&out_found, 0, &[jboolean::from(found)])
                    .is_err()
            {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_add_image_source_url(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    coordinates: JDoubleArray<'_>,
    url: JString<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (source_id_storage, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let coordinate_values = match read_nonempty_coordinate_pairs(&env, &coordinates) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let coordinates: Vec<_> = coordinate_values
            .chunks_exact(2)
            .map(|pair| sys::mln_lat_lng {
                latitude: pair[0],
                longitude: pair[1],
            })
            .collect();
        let (url_storage, url_view) = match string_view(&mut env, &url) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _keep_alive = (source_id_storage, url_storage);
        unsafe {
            sys::mln_map_add_image_source_url(
                map as *mut sys::mln_map,
                source_id_view,
                coordinates.as_ptr(),
                coordinates.len(),
                url_view,
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_add_image_source_image(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    coordinates: JDoubleArray<'_>,
    width: jint,
    height: jint,
    stride: jint,
    pixels: JByteArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (source_id_storage, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let coordinate_values = match read_nonempty_coordinate_pairs(&env, &coordinates) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let coordinates: Vec<_> = coordinate_values
            .chunks_exact(2)
            .map(|pair| sys::mln_lat_lng {
                latitude: pair[0],
                longitude: pair[1],
            })
            .collect();
        let (image, image_pixels) =
            match premultiplied_rgba8_image(&env, &pixels, width, height, stride) {
                Ok(value) => value,
                Err(status) => return status,
            };
        let _keep_alive = (source_id_storage, image_pixels);
        unsafe {
            sys::mln_map_add_image_source_image(
                map as *mut sys::mln_map,
                source_id_view,
                coordinates.as_ptr(),
                coordinates.len(),
                &image,
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_image_source_url(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    url: JString<'_>,
) -> jint {
    map_style_source_url(env, map, source_id, url, sys::mln_map_set_image_source_url)
}

extern "system" fn map_set_image_source_image(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    width: jint,
    height: jint,
    stride: jint,
    pixels: JByteArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let mut env = env;
        let (source_id_storage, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (image, image_pixels) =
            match premultiplied_rgba8_image(&env, &pixels, width, height, stride) {
                Ok(value) => value,
                Err(status) => return status,
            };
        let _keep_alive = (source_id_storage, image_pixels);
        unsafe {
            sys::mln_map_set_image_source_image(map as *mut sys::mln_map, source_id_view, &image)
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_image_source_coordinates(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    coordinates: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (source_id_storage, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let coordinate_values = match read_nonempty_coordinate_pairs(&env, &coordinates) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let coordinates: Vec<_> = coordinate_values
            .chunks_exact(2)
            .map(|pair| sys::mln_lat_lng {
                latitude: pair[0],
                longitude: pair[1],
            })
            .collect();
        let _keep_alive = source_id_storage;
        unsafe {
            sys::mln_map_set_image_source_coordinates(
                map as *mut sys::mln_map,
                source_id_view,
                coordinates.as_ptr(),
                coordinates.len(),
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_image_source_coordinates(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    source_id: JString<'_>,
    out_coordinates: JDoubleArray<'_>,
    out_coordinate_count: JLongArray<'_>,
    out_found: JBooleanArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_coordinates.is_null()
            || out_coordinate_count.is_null()
            || out_found.is_null()
            || env.get_array_length(&out_coordinate_count).unwrap_or(0) < 1
            || env.get_array_length(&out_found).unwrap_or(0) < 1
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let capacity_values = match env.get_array_length(&out_coordinates) {
            Ok(value) if value >= 0 && value % 2 == 0 => value as usize,
            _ => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        let coordinate_capacity = capacity_values / 2;
        let (source_id_storage, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _keep_alive = source_id_storage;
        let mut coordinates = vec![
            sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            };
            coordinate_capacity
        ];
        let mut coordinate_count = 0_usize;
        let mut found = false;
        let result = unsafe {
            sys::mln_map_get_image_source_coordinates(
                map as *mut sys::mln_map,
                source_id_view,
                if coordinates.is_empty() {
                    std::ptr::null_mut()
                } else {
                    coordinates.as_mut_ptr()
                },
                coordinates.len(),
                &mut coordinate_count,
                &mut found,
            )
        };
        if result == sys::MLN_STATUS_OK {
            if coordinate_count > coordinates.len() {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
            let mut values = vec![0.0_f64; coordinate_count * 2];
            for (index, coordinate) in coordinates.iter().take(coordinate_count).enumerate() {
                values[index * 2] = coordinate.latitude;
                values[index * 2 + 1] = coordinate.longitude;
            }
            if (!values.is_empty()
                && env
                    .set_double_array_region(&out_coordinates, 0, &values)
                    .is_err())
                || env
                    .set_long_array_region(&out_coordinate_count, 0, &[coordinate_count as jlong])
                    .is_err()
                || env
                    .set_boolean_array_region(&out_found, 0, &[jboolean::from(found)])
                    .is_err()
            {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_add_hillshade_layer(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    source_id: JString<'_>,
    before_layer_id: JString<'_>,
) -> jint {
    map_add_source_layer(
        env,
        map,
        layer_id,
        source_id,
        before_layer_id,
        sys::mln_map_add_hillshade_layer,
    )
}

extern "system" fn map_add_color_relief_layer(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    source_id: JString<'_>,
    before_layer_id: JString<'_>,
) -> jint {
    map_add_source_layer(
        env,
        map,
        layer_id,
        source_id,
        before_layer_id,
        sys::mln_map_add_color_relief_layer,
    )
}

fn map_add_source_layer(
    mut env: JNIEnv<'_>,
    map: jlong,
    layer_id: JString<'_>,
    source_id: JString<'_>,
    before_layer_id: JString<'_>,
    native: unsafe extern "C" fn(
        *mut sys::mln_map,
        sys::mln_string_view,
        sys::mln_string_view,
        sys::mln_string_view,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (layer_id_storage, layer_id_view) = match string_view(&mut env, &layer_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (source_id_storage, source_id_view) = match string_view(&mut env, &source_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (before_layer_id_storage, before_layer_id_view) =
            match string_view(&mut env, &before_layer_id) {
                Ok(value) => value,
                Err(status) => return status,
            };
        let _keep_alive = (layer_id_storage, source_id_storage, before_layer_id_storage);
        unsafe {
            native(
                map as *mut sys::mln_map,
                layer_id_view,
                source_id_view,
                before_layer_id_view,
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_add_location_indicator_layer(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    before_layer_id: JString<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (layer_id_storage, layer_id_view) = match string_view(&mut env, &layer_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (before_layer_id_storage, before_layer_id_view) =
            match string_view(&mut env, &before_layer_id) {
                Ok(value) => value,
                Err(status) => return status,
            };
        let _keep_alive = (layer_id_storage, before_layer_id_storage);
        unsafe {
            sys::mln_map_add_location_indicator_layer(
                map as *mut sys::mln_map,
                layer_id_view,
                before_layer_id_view,
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_location_indicator_location(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    latitude: f64,
    longitude: f64,
    altitude: f64,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (layer_id_storage, layer_id_view) = match string_view(&mut env, &layer_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _keep_alive = layer_id_storage;
        unsafe {
            sys::mln_map_set_location_indicator_location(
                map as *mut sys::mln_map,
                layer_id_view,
                sys::mln_lat_lng {
                    latitude,
                    longitude,
                },
                altitude,
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_location_indicator_bearing(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    bearing: f64,
) -> jint {
    map_set_location_indicator_double(
        env,
        map,
        layer_id,
        bearing,
        sys::mln_map_set_location_indicator_bearing,
    )
}

extern "system" fn map_set_location_indicator_accuracy_radius(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    radius: f64,
) -> jint {
    map_set_location_indicator_double(
        env,
        map,
        layer_id,
        radius,
        sys::mln_map_set_location_indicator_accuracy_radius,
    )
}

fn map_set_location_indicator_double(
    mut env: JNIEnv<'_>,
    map: jlong,
    layer_id: JString<'_>,
    value: f64,
    native: unsafe extern "C" fn(*mut sys::mln_map, sys::mln_string_view, f64) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (layer_id_storage, layer_id_view) = match string_view(&mut env, &layer_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _keep_alive = layer_id_storage;
        unsafe { native(map as *mut sys::mln_map, layer_id_view, value) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_location_indicator_image_name(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    image_kind: jint,
    image_id: JString<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (layer_id_storage, layer_id_view) = match string_view(&mut env, &layer_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (image_id_storage, image_id_view) = match string_view(&mut env, &image_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _keep_alive = (layer_id_storage, image_id_storage);
        unsafe {
            sys::mln_map_set_location_indicator_image_name(
                map as *mut sys::mln_map,
                layer_id_view,
                image_kind as u32,
                image_id_view,
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_move_style_layer(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    before_layer_id: JString<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let (layer_id_storage, layer_id_view) = match string_view(&mut env, &layer_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let (before_layer_id_storage, before_layer_id_view) =
            match string_view(&mut env, &before_layer_id) {
                Ok(value) => value,
                Err(status) => return status,
            };
        let _keep_alive = (layer_id_storage, before_layer_id_storage);
        unsafe {
            sys::mln_map_move_style_layer(
                map as *mut sys::mln_map,
                layer_id_view,
                before_layer_id_view,
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_remove_style_layer(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    out_removed: JBooleanArray<'_>,
) -> jint {
    map_style_layer_bool(
        env,
        map,
        layer_id,
        out_removed,
        sys::mln_map_remove_style_layer,
    )
}

extern "system" fn map_style_layer_exists(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    out_exists: JBooleanArray<'_>,
) -> jint {
    map_style_layer_bool(
        env,
        map,
        layer_id,
        out_exists,
        sys::mln_map_style_layer_exists,
    )
}

fn map_style_layer_bool(
    mut env: JNIEnv<'_>,
    map: jlong,
    layer_id: JString<'_>,
    out_value: JBooleanArray<'_>,
    operation: unsafe extern "C" fn(
        *mut sys::mln_map,
        sys::mln_string_view,
        *mut bool,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_value.is_null() || env.get_array_length(&out_value).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let (layer_id, layer_id_view) = match string_view(&mut env, &layer_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _layer_id = layer_id;
        let mut value = false;
        let result = unsafe { operation(map as *mut sys::mln_map, layer_id_view, &mut value) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_boolean_array_region(&out_value, 0, &[jboolean::from(value)])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_style_layer_type(
    mut env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    layer_id: JString<'_>,
    out_layer_type: JObjectArray<'_>,
    out_found: JBooleanArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_layer_type.is_null()
            || out_found.is_null()
            || env.get_array_length(&out_layer_type).unwrap_or(0) < 1
            || env.get_array_length(&out_found).unwrap_or(0) < 1
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let (layer_id, layer_id_view) = match string_view(&mut env, &layer_id) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let _layer_id = layer_id;
        let mut layer_type = sys::mln_string_view {
            data: std::ptr::null(),
            size: 0,
        };
        let mut found = false;
        let result = unsafe {
            sys::mln_map_get_style_layer_type(
                map as *mut sys::mln_map,
                layer_id_view,
                &mut layer_type,
                &mut found,
            )
        };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        if env
            .set_boolean_array_region(&out_found, 0, &[jboolean::from(found)])
            .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        if found {
            let layer_type = unsafe { copy_string(layer_type.data, layer_type.size) };
            if set_string_array_element(&env, &out_layer_type, 0, layer_type).is_err() {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
        }
        sys::MLN_STATUS_OK
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn map_list_style_ids(
    mut env: JNIEnv<'_>,
    map: jlong,
    out_ids: JObjectArray<'_>,
    list_function: unsafe extern "C" fn(
        *mut sys::mln_map,
        *mut *mut sys::mln_style_id_list,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_ids.is_null() || env.get_array_length(&out_ids).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut list: *mut sys::mln_style_id_list = std::ptr::null_mut();
        let result = unsafe { list_function(map as *mut sys::mln_map, &mut list) };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        let list_guard = StyleIdListGuard(list);
        let mut count = 0_usize;
        let result = unsafe { sys::mln_style_id_list_count(list_guard.0, &mut count) };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        let string_class = match env.find_class("java/lang/String") {
            Ok(value) => value,
            Err(_) => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        let ids = match env.new_object_array(count as i32, string_class, JObject::null()) {
            Ok(value) => value,
            Err(_) => return sys::MLN_STATUS_INVALID_ARGUMENT,
        };
        for index in 0..count {
            let mut id = sys::mln_string_view {
                data: std::ptr::null(),
                size: 0,
            };
            let result = unsafe { sys::mln_style_id_list_get(list_guard.0, index, &mut id) };
            if result != sys::MLN_STATUS_OK {
                return result;
            }
            if set_string_array_element(&env, &ids, index as i32, unsafe {
                copy_string(id.data, id.size)
            })
            .is_err()
            {
                return sys::MLN_STATUS_INVALID_ARGUMENT;
            }
        }
        if env.set_object_array_element(&out_ids, 0, &ids).is_err() {
            sys::MLN_STATUS_INVALID_ARGUMENT
        } else {
            sys::MLN_STATUS_OK
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

struct StyleIdListGuard(*mut sys::mln_style_id_list);

impl Drop for StyleIdListGuard {
    fn drop(&mut self) {
        unsafe { sys::mln_style_id_list_destroy(self.0) }
    }
}

fn jstring_to_cstring(env: &mut JNIEnv<'_>, value: &JString<'_>) -> Result<CString, jint> {
    if value.is_null() {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let string = env
        .get_string(value)
        .map_err(|_| sys::MLN_STATUS_INVALID_ARGUMENT)?;
    CString::new(string.to_bytes()).map_err(|_| sys::MLN_STATUS_INVALID_ARGUMENT)
}

fn string_view(
    env: &mut JNIEnv<'_>,
    value: &JString<'_>,
) -> Result<(CString, sys::mln_string_view), jint> {
    if value.is_null() {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let java_string = env
        .get_string(value)
        .map_err(|_| sys::MLN_STATUS_INVALID_ARGUMENT)?;
    let string =
        CString::new(String::from(java_string)).map_err(|_| sys::MLN_STATUS_INVALID_ARGUMENT)?;
    let view = sys::mln_string_view {
        data: string.as_ptr(),
        size: string.as_bytes().len(),
    };
    Ok((string, view))
}

extern "system" fn map_set_debug_options(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    options: jint,
) -> jint {
    catch_unwind(|| unsafe {
        sys::mln_map_set_debug_options(map as *mut sys::mln_map, options as u32)
    })
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_debug_options(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_options: JIntArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_options.is_null() || env.get_array_length(&out_options).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut options = 0_u32;
        let result =
            unsafe { sys::mln_map_get_debug_options(map as *mut sys::mln_map, &mut options) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_int_array_region(&out_options, 0, &[options as jint])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_rendering_stats_view_enabled(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    enabled: jboolean,
) -> jint {
    catch_unwind(|| unsafe {
        sys::mln_map_set_rendering_stats_view_enabled(map as *mut sys::mln_map, enabled != 0)
    })
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_rendering_stats_view_enabled(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_enabled: JBooleanArray<'_>,
) -> jint {
    map_get_bool(
        env,
        map,
        out_enabled,
        sys::mln_map_get_rendering_stats_view_enabled,
    )
}

extern "system" fn map_is_fully_loaded(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_loaded: JBooleanArray<'_>,
) -> jint {
    map_get_bool(env, map, out_loaded, sys::mln_map_is_fully_loaded)
}

extern "system" fn map_dump_debug_logs(_env: JNIEnv<'_>, _class: JClass<'_>, map: jlong) -> jint {
    catch_unwind(|| unsafe { sys::mln_map_dump_debug_logs(map as *mut sys::mln_map) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_viewport_options(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_fields: JBooleanArray<'_>,
    out_ints: JIntArray<'_>,
    out_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !viewport_arrays_are_valid(&env, &out_fields, &out_ints, &out_values) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut options = unsafe { sys::mln_map_viewport_options_default() };
        let result =
            unsafe { sys::mln_map_get_viewport_options(map as *mut sys::mln_map, &mut options) };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        write_viewport_arrays(&env, &out_fields, &out_ints, &out_values, &options)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_viewport_options(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    fields: JBooleanArray<'_>,
    ints: JIntArray<'_>,
    values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let options = match read_viewport_options(&env, &fields, &ints, &values) {
            Ok(value) => value,
            Err(status) => return status,
        };
        unsafe { sys::mln_map_set_viewport_options(map as *mut sys::mln_map, &options) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_tile_options(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_fields: JBooleanArray<'_>,
    out_ints: JIntArray<'_>,
    out_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !tile_arrays_are_valid(&env, &out_fields, &out_ints, &out_values) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut options = unsafe { sys::mln_map_tile_options_default() };
        let result =
            unsafe { sys::mln_map_get_tile_options(map as *mut sys::mln_map, &mut options) };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        write_tile_arrays(&env, &out_fields, &out_ints, &out_values, &options)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_tile_options(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    fields: JBooleanArray<'_>,
    ints: JIntArray<'_>,
    values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let options = match read_tile_options(&env, &fields, &ints, &values) {
            Ok(value) => value,
            Err(status) => return status,
        };
        unsafe { sys::mln_map_set_tile_options(map as *mut sys::mln_map, &options) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

#[allow(clippy::too_many_arguments)]
extern "system" fn map_camera_for_lat_lng_bounds(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    southwest_latitude: f64,
    southwest_longitude: f64,
    northeast_latitude: f64,
    northeast_longitude: f64,
    has_fit_options: jboolean,
    fit_fields: JBooleanArray<'_>,
    fit_values: JDoubleArray<'_>,
    out_camera_fields: JBooleanArray<'_>,
    out_camera_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !camera_arrays_are_valid(&env, &out_camera_fields, &out_camera_values) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let fit_options =
            match optional_camera_fit_options(&env, has_fit_options, &fit_fields, &fit_values) {
                Ok(value) => value,
                Err(status) => return status,
            };
        let mut camera = unsafe { sys::mln_camera_options_default() };
        let result = unsafe {
            sys::mln_map_camera_for_lat_lng_bounds(
                map as *mut sys::mln_map,
                sys::mln_lat_lng_bounds {
                    southwest: sys::mln_lat_lng {
                        latitude: southwest_latitude,
                        longitude: southwest_longitude,
                    },
                    northeast: sys::mln_lat_lng {
                        latitude: northeast_latitude,
                        longitude: northeast_longitude,
                    },
                },
                camera_fit_ptr(&fit_options),
                &mut camera,
            )
        };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        write_camera_arrays(&env, &out_camera_fields, &out_camera_values, &camera)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_camera_for_lat_lngs(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    coordinates: JDoubleArray<'_>,
    has_fit_options: jboolean,
    fit_fields: JBooleanArray<'_>,
    fit_values: JDoubleArray<'_>,
    out_camera_fields: JBooleanArray<'_>,
    out_camera_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !camera_arrays_are_valid(&env, &out_camera_fields, &out_camera_values) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let coordinate_values = match read_nonempty_coordinate_pairs(&env, &coordinates) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let coordinates: Vec<_> = coordinate_values
            .chunks_exact(2)
            .map(|pair| sys::mln_lat_lng {
                latitude: pair[0],
                longitude: pair[1],
            })
            .collect();
        let fit_options =
            match optional_camera_fit_options(&env, has_fit_options, &fit_fields, &fit_values) {
                Ok(value) => value,
                Err(status) => return status,
            };
        let mut camera = unsafe { sys::mln_camera_options_default() };
        let result = unsafe {
            sys::mln_map_camera_for_lat_lngs(
                map as *mut sys::mln_map,
                coordinates.as_ptr(),
                coordinates.len(),
                camera_fit_ptr(&fit_options),
                &mut camera,
            )
        };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        write_camera_arrays(&env, &out_camera_fields, &out_camera_values, &camera)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_lat_lng_bounds_for_camera(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    camera_fields: JBooleanArray<'_>,
    camera_values: JDoubleArray<'_>,
    out_bounds: JDoubleArray<'_>,
) -> jint {
    map_lat_lng_bounds_for_camera_impl(
        env,
        map,
        camera_fields,
        camera_values,
        out_bounds,
        sys::mln_map_lat_lng_bounds_for_camera,
    )
}

extern "system" fn map_lat_lng_bounds_for_camera_unwrapped(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    camera_fields: JBooleanArray<'_>,
    camera_values: JDoubleArray<'_>,
    out_bounds: JDoubleArray<'_>,
) -> jint {
    map_lat_lng_bounds_for_camera_impl(
        env,
        map,
        camera_fields,
        camera_values,
        out_bounds,
        sys::mln_map_lat_lng_bounds_for_camera_unwrapped,
    )
}

fn map_lat_lng_bounds_for_camera_impl(
    env: JNIEnv<'_>,
    map: jlong,
    camera_fields: JBooleanArray<'_>,
    camera_values: JDoubleArray<'_>,
    out_bounds: JDoubleArray<'_>,
    bounds_function: unsafe extern "C" fn(
        *mut sys::mln_map,
        *const sys::mln_camera_options,
        *mut sys::mln_lat_lng_bounds,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_bounds.is_null() || env.get_array_length(&out_bounds).unwrap_or(0) < 4 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let camera = match read_camera_options(&env, &camera_fields, &camera_values) {
            Ok(camera) => camera,
            Err(status) => return status,
        };
        let mut bounds = sys::mln_lat_lng_bounds {
            southwest: sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            },
            northeast: sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            },
        };
        let result = unsafe { bounds_function(map as *mut sys::mln_map, &camera, &mut bounds) };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        if env
            .set_double_array_region(
                &out_bounds,
                0,
                &[
                    bounds.southwest.latitude,
                    bounds.southwest.longitude,
                    bounds.northeast.latitude,
                    bounds.northeast.longitude,
                ],
            )
            .is_err()
        {
            sys::MLN_STATUS_INVALID_ARGUMENT
        } else {
            sys::MLN_STATUS_OK
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_free_camera_options(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_fields: JBooleanArray<'_>,
    out_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !free_camera_arrays_are_valid(&env, &out_fields, &out_values) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut options = unsafe { sys::mln_free_camera_options_default() };
        let result =
            unsafe { sys::mln_map_get_free_camera_options(map as *mut sys::mln_map, &mut options) };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        write_free_camera_arrays(&env, &out_fields, &out_values, &options)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_free_camera_options(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    fields: JBooleanArray<'_>,
    values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let options = match read_free_camera_options(&env, &fields, &values) {
            Ok(options) => options,
            Err(status) => return status,
        };
        unsafe { sys::mln_map_set_free_camera_options(map as *mut sys::mln_map, &options) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_projection_mode(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_fields: JBooleanArray<'_>,
    out_booleans: JBooleanArray<'_>,
    out_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !projection_mode_arrays_are_valid(&env, &out_fields, &out_booleans, &out_values) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut mode = unsafe { sys::mln_projection_mode_default() };
        let result =
            unsafe { sys::mln_map_get_projection_mode(map as *mut sys::mln_map, &mut mode) };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        write_projection_mode_arrays(&env, &out_fields, &out_booleans, &out_values, &mode)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_projection_mode(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    fields: JBooleanArray<'_>,
    booleans: JBooleanArray<'_>,
    values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let mode = match read_projection_mode(&env, &fields, &booleans, &values) {
            Ok(mode) => mode,
            Err(status) => return status,
        };
        unsafe { sys::mln_map_set_projection_mode(map as *mut sys::mln_map, &mode) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_bounds(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_fields: JBooleanArray<'_>,
    out_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !bound_arrays_are_valid(&env, &out_fields, &out_values) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut bounds = unsafe { sys::mln_bound_options_default() };
        let result = unsafe { sys::mln_map_get_bounds(map as *mut sys::mln_map, &mut bounds) };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        write_bound_arrays(&env, &out_fields, &out_values, &bounds)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_set_bounds(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    fields: JBooleanArray<'_>,
    values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let bounds = match read_bound_options(&env, &fields, &values) {
            Ok(bounds) => bounds,
            Err(status) => return status,
        };
        unsafe { sys::mln_map_set_bounds(map as *mut sys::mln_map, &bounds) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_get_camera(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_fields: JBooleanArray<'_>,
    out_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !camera_arrays_are_valid(&env, &out_fields, &out_values) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut camera = unsafe { sys::mln_camera_options_default() };
        let result = unsafe { sys::mln_map_get_camera(map as *mut sys::mln_map, &mut camera) };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        write_camera_arrays(&env, &out_fields, &out_values, &camera)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_jump_to(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    fields: JBooleanArray<'_>,
    values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let camera = match read_camera_options(&env, &fields, &values) {
            Ok(camera) => camera,
            Err(status) => return status,
        };
        unsafe { sys::mln_map_jump_to(map as *mut sys::mln_map, &camera) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_ease_to(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    camera_fields: JBooleanArray<'_>,
    camera_values: JDoubleArray<'_>,
    has_animation: jboolean,
    animation_fields: JBooleanArray<'_>,
    animation_values: JDoubleArray<'_>,
) -> jint {
    map_camera_transition(
        env,
        map,
        camera_fields,
        camera_values,
        has_animation,
        animation_fields,
        animation_values,
        sys::mln_map_ease_to,
    )
}

extern "system" fn map_fly_to(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    camera_fields: JBooleanArray<'_>,
    camera_values: JDoubleArray<'_>,
    has_animation: jboolean,
    animation_fields: JBooleanArray<'_>,
    animation_values: JDoubleArray<'_>,
) -> jint {
    map_camera_transition(
        env,
        map,
        camera_fields,
        camera_values,
        has_animation,
        animation_fields,
        animation_values,
        sys::mln_map_fly_to,
    )
}

#[allow(clippy::too_many_arguments)]
fn map_camera_transition(
    env: JNIEnv<'_>,
    map: jlong,
    camera_fields: JBooleanArray<'_>,
    camera_values: JDoubleArray<'_>,
    has_animation: jboolean,
    animation_fields: JBooleanArray<'_>,
    animation_values: JDoubleArray<'_>,
    transition: unsafe extern "C" fn(
        *mut sys::mln_map,
        *const sys::mln_camera_options,
        *const sys::mln_animation_options,
    ) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let camera = match read_camera_options(&env, &camera_fields, &camera_values) {
            Ok(camera) => camera,
            Err(status) => return status,
        };
        let animation = if has_animation != 0 {
            match read_animation_options(&env, &animation_fields, &animation_values) {
                Ok(animation) => Some(animation),
                Err(status) => return status,
            }
        } else {
            None
        };
        let animation_ptr = animation
            .as_ref()
            .map_or(std::ptr::null(), |animation| animation as *const _);
        unsafe { transition(map as *mut sys::mln_map, &camera, animation_ptr) }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn viewport_arrays_are_valid(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    ints: &JIntArray<'_>,
    values: &JDoubleArray<'_>,
) -> bool {
    !fields.is_null()
        && !ints.is_null()
        && !values.is_null()
        && env.get_array_length(fields).unwrap_or(0) >= VIEWPORT_FIELD_COUNT as i32
        && env.get_array_length(ints).unwrap_or(0) >= VIEWPORT_INT_COUNT as i32
        && env.get_array_length(values).unwrap_or(0) >= VIEWPORT_VALUE_COUNT as i32
}

fn tile_arrays_are_valid(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    ints: &JIntArray<'_>,
    values: &JDoubleArray<'_>,
) -> bool {
    !fields.is_null()
        && !ints.is_null()
        && !values.is_null()
        && env.get_array_length(fields).unwrap_or(0) >= TILE_FIELD_COUNT as i32
        && env.get_array_length(ints).unwrap_or(0) >= TILE_INT_COUNT as i32
        && env.get_array_length(values).unwrap_or(0) >= TILE_VALUE_COUNT as i32
}

fn camera_arrays_are_valid(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> bool {
    !fields.is_null()
        && !values.is_null()
        && env.get_array_length(fields).unwrap_or(0) >= CAMERA_FIELD_COUNT as i32
        && env.get_array_length(values).unwrap_or(0) >= CAMERA_VALUE_COUNT as i32
}

fn camera_fit_arrays_are_valid(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> bool {
    !fields.is_null()
        && !values.is_null()
        && env.get_array_length(fields).unwrap_or(0) >= FIT_FIELD_COUNT as i32
        && env.get_array_length(values).unwrap_or(0) >= FIT_VALUE_COUNT as i32
}

fn free_camera_arrays_are_valid(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> bool {
    !fields.is_null()
        && !values.is_null()
        && env.get_array_length(fields).unwrap_or(0) >= FREE_CAMERA_FIELD_COUNT as i32
        && env.get_array_length(values).unwrap_or(0) >= FREE_CAMERA_VALUE_COUNT as i32
}

fn projection_mode_arrays_are_valid(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    booleans: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> bool {
    !fields.is_null()
        && !booleans.is_null()
        && !values.is_null()
        && env.get_array_length(fields).unwrap_or(0) >= PROJECTION_MODE_FIELD_COUNT as i32
        && env.get_array_length(booleans).unwrap_or(0) >= PROJECTION_MODE_BOOLEAN_COUNT as i32
        && env.get_array_length(values).unwrap_or(0) >= PROJECTION_MODE_VALUE_COUNT as i32
}

fn bound_arrays_are_valid(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> bool {
    !fields.is_null()
        && !values.is_null()
        && env.get_array_length(fields).unwrap_or(0) >= BOUND_FIELD_COUNT as i32
        && env.get_array_length(values).unwrap_or(0) >= BOUND_VALUE_COUNT as i32
}

fn animation_arrays_are_valid(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> bool {
    !fields.is_null()
        && !values.is_null()
        && env.get_array_length(fields).unwrap_or(0) >= ANIMATION_FIELD_COUNT as i32
        && env.get_array_length(values).unwrap_or(0) >= ANIMATION_VALUE_COUNT as i32
}

fn optional_camera_fit_options(
    env: &JNIEnv<'_>,
    has_fit_options: jboolean,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<Option<sys::mln_camera_fit_options>, jint> {
    if has_fit_options != 0 {
        read_camera_fit_options(env, fields, values).map(Some)
    } else {
        Ok(None)
    }
}

fn camera_fit_ptr(
    fit_options: &Option<sys::mln_camera_fit_options>,
) -> *const sys::mln_camera_fit_options {
    fit_options
        .as_ref()
        .map_or(std::ptr::null(), |fit_options| fit_options as *const _)
}

fn read_viewport_options(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    ints: &JIntArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<sys::mln_map_viewport_options, jint> {
    if !viewport_arrays_are_valid(env, fields, ints, values) {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut field_values = [0 as jboolean; VIEWPORT_FIELD_COUNT];
    let mut int_values = [0 as jint; VIEWPORT_INT_COUNT];
    let mut option_values = [0.0_f64; VIEWPORT_VALUE_COUNT];
    if env
        .get_boolean_array_region(fields, 0, &mut field_values)
        .is_err()
        || env.get_int_array_region(ints, 0, &mut int_values).is_err()
        || env
            .get_double_array_region(values, 0, &mut option_values)
            .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut options = unsafe { sys::mln_map_viewport_options_default() };
    if field_values[VIEWPORT_FIELD_NORTH_ORIENTATION] != 0 {
        options.fields |= sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;
        options.north_orientation = int_values[VIEWPORT_INT_NORTH_ORIENTATION] as u32;
    }
    if field_values[VIEWPORT_FIELD_CONSTRAIN_MODE] != 0 {
        options.fields |= sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
        options.constrain_mode = int_values[VIEWPORT_INT_CONSTRAIN_MODE] as u32;
    }
    if field_values[VIEWPORT_FIELD_VIEWPORT_MODE] != 0 {
        options.fields |= sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;
        options.viewport_mode = int_values[VIEWPORT_INT_VIEWPORT_MODE] as u32;
    }
    if field_values[VIEWPORT_FIELD_FRUSTUM_OFFSET] != 0 {
        options.fields |= sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET;
        options.frustum_offset.top = option_values[VIEWPORT_VALUE_FRUSTUM_TOP];
        options.frustum_offset.left = option_values[VIEWPORT_VALUE_FRUSTUM_LEFT];
        options.frustum_offset.bottom = option_values[VIEWPORT_VALUE_FRUSTUM_BOTTOM];
        options.frustum_offset.right = option_values[VIEWPORT_VALUE_FRUSTUM_RIGHT];
    }
    Ok(options)
}

fn write_viewport_arrays(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    ints: &JIntArray<'_>,
    values: &JDoubleArray<'_>,
    options: &sys::mln_map_viewport_options,
) -> jint {
    let field_values = [
        jboolean::from(options.fields & sys::MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION != 0),
        jboolean::from(options.fields & sys::MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE != 0),
        jboolean::from(options.fields & sys::MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE != 0),
        jboolean::from(options.fields & sys::MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET != 0),
    ];
    let int_values = [
        options.north_orientation as jint,
        options.constrain_mode as jint,
        options.viewport_mode as jint,
    ];
    let option_values = [
        options.frustum_offset.top,
        options.frustum_offset.left,
        options.frustum_offset.bottom,
        options.frustum_offset.right,
    ];
    if env
        .set_boolean_array_region(fields, 0, &field_values)
        .is_err()
        || env.set_int_array_region(ints, 0, &int_values).is_err()
        || env
            .set_double_array_region(values, 0, &option_values)
            .is_err()
    {
        sys::MLN_STATUS_INVALID_ARGUMENT
    } else {
        sys::MLN_STATUS_OK
    }
}

fn read_tile_options(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    ints: &JIntArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<sys::mln_map_tile_options, jint> {
    if !tile_arrays_are_valid(env, fields, ints, values) {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut field_values = [0 as jboolean; TILE_FIELD_COUNT];
    let mut int_values = [0 as jint; TILE_INT_COUNT];
    let mut option_values = [0.0_f64; TILE_VALUE_COUNT];
    if env
        .get_boolean_array_region(fields, 0, &mut field_values)
        .is_err()
        || env.get_int_array_region(ints, 0, &mut int_values).is_err()
        || env
            .get_double_array_region(values, 0, &mut option_values)
            .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut options = unsafe { sys::mln_map_tile_options_default() };
    if field_values[TILE_FIELD_PREFETCH_ZOOM_DELTA] != 0 {
        options.fields |= sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA;
        options.prefetch_zoom_delta = int_values[TILE_INT_PREFETCH_ZOOM_DELTA] as u32;
    }
    if field_values[TILE_FIELD_LOD_MIN_RADIUS] != 0 {
        options.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS;
        options.lod_min_radius = option_values[TILE_VALUE_LOD_MIN_RADIUS];
    }
    if field_values[TILE_FIELD_LOD_SCALE] != 0 {
        options.fields |= sys::MLN_MAP_TILE_OPTION_LOD_SCALE;
        options.lod_scale = option_values[TILE_VALUE_LOD_SCALE];
    }
    if field_values[TILE_FIELD_LOD_PITCH_THRESHOLD] != 0 {
        options.fields |= sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD;
        options.lod_pitch_threshold = option_values[TILE_VALUE_LOD_PITCH_THRESHOLD];
    }
    if field_values[TILE_FIELD_LOD_ZOOM_SHIFT] != 0 {
        options.fields |= sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT;
        options.lod_zoom_shift = option_values[TILE_VALUE_LOD_ZOOM_SHIFT];
    }
    if field_values[TILE_FIELD_LOD_MODE] != 0 {
        options.fields |= sys::MLN_MAP_TILE_OPTION_LOD_MODE;
        options.lod_mode = int_values[TILE_INT_LOD_MODE] as u32;
    }
    Ok(options)
}

fn write_tile_arrays(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    ints: &JIntArray<'_>,
    values: &JDoubleArray<'_>,
    options: &sys::mln_map_tile_options,
) -> jint {
    let field_values = [
        jboolean::from(options.fields & sys::MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA != 0),
        jboolean::from(options.fields & sys::MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS != 0),
        jboolean::from(options.fields & sys::MLN_MAP_TILE_OPTION_LOD_SCALE != 0),
        jboolean::from(options.fields & sys::MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD != 0),
        jboolean::from(options.fields & sys::MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT != 0),
        jboolean::from(options.fields & sys::MLN_MAP_TILE_OPTION_LOD_MODE != 0),
    ];
    let int_values = [
        options.prefetch_zoom_delta as jint,
        options.lod_mode as jint,
    ];
    let option_values = [
        options.lod_min_radius,
        options.lod_scale,
        options.lod_pitch_threshold,
        options.lod_zoom_shift,
    ];
    if env
        .set_boolean_array_region(fields, 0, &field_values)
        .is_err()
        || env.set_int_array_region(ints, 0, &int_values).is_err()
        || env
            .set_double_array_region(values, 0, &option_values)
            .is_err()
    {
        sys::MLN_STATUS_INVALID_ARGUMENT
    } else {
        sys::MLN_STATUS_OK
    }
}

fn read_camera_fit_options(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<sys::mln_camera_fit_options, jint> {
    if !camera_fit_arrays_are_valid(env, fields, values) {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut field_values = [0 as jboolean; FIT_FIELD_COUNT];
    let mut option_values = [0.0_f64; FIT_VALUE_COUNT];
    if env
        .get_boolean_array_region(fields, 0, &mut field_values)
        .is_err()
        || env
            .get_double_array_region(values, 0, &mut option_values)
            .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut options = unsafe { sys::mln_camera_fit_options_default() };
    if field_values[FIT_FIELD_PADDING] != 0 {
        options.fields |= sys::MLN_CAMERA_FIT_OPTION_PADDING;
        options.padding.top = option_values[FIT_VALUE_PADDING_TOP];
        options.padding.left = option_values[FIT_VALUE_PADDING_LEFT];
        options.padding.bottom = option_values[FIT_VALUE_PADDING_BOTTOM];
        options.padding.right = option_values[FIT_VALUE_PADDING_RIGHT];
    }
    if field_values[FIT_FIELD_BEARING] != 0 {
        options.fields |= sys::MLN_CAMERA_FIT_OPTION_BEARING;
        options.bearing = option_values[FIT_VALUE_BEARING];
    }
    if field_values[FIT_FIELD_PITCH] != 0 {
        options.fields |= sys::MLN_CAMERA_FIT_OPTION_PITCH;
        options.pitch = option_values[FIT_VALUE_PITCH];
    }
    Ok(options)
}

fn read_free_camera_options(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<sys::mln_free_camera_options, jint> {
    if !free_camera_arrays_are_valid(env, fields, values) {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut field_values = [0 as jboolean; FREE_CAMERA_FIELD_COUNT];
    let mut option_values = [0.0_f64; FREE_CAMERA_VALUE_COUNT];
    if env
        .get_boolean_array_region(fields, 0, &mut field_values)
        .is_err()
        || env
            .get_double_array_region(values, 0, &mut option_values)
            .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut options = unsafe { sys::mln_free_camera_options_default() };
    if field_values[FREE_CAMERA_FIELD_POSITION] != 0 {
        options.fields |= sys::MLN_FREE_CAMERA_OPTION_POSITION;
        options.position.x = option_values[FREE_CAMERA_VALUE_POSITION_X];
        options.position.y = option_values[FREE_CAMERA_VALUE_POSITION_Y];
        options.position.z = option_values[FREE_CAMERA_VALUE_POSITION_Z];
    }
    if field_values[FREE_CAMERA_FIELD_ORIENTATION] != 0 {
        options.fields |= sys::MLN_FREE_CAMERA_OPTION_ORIENTATION;
        options.orientation.x = option_values[FREE_CAMERA_VALUE_ORIENTATION_X];
        options.orientation.y = option_values[FREE_CAMERA_VALUE_ORIENTATION_Y];
        options.orientation.z = option_values[FREE_CAMERA_VALUE_ORIENTATION_Z];
        options.orientation.w = option_values[FREE_CAMERA_VALUE_ORIENTATION_W];
    }
    Ok(options)
}

fn write_free_camera_arrays(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
    options: &sys::mln_free_camera_options,
) -> jint {
    let field_values = [
        jboolean::from(options.fields & sys::MLN_FREE_CAMERA_OPTION_POSITION != 0),
        jboolean::from(options.fields & sys::MLN_FREE_CAMERA_OPTION_ORIENTATION != 0),
    ];
    let option_values = [
        options.position.x,
        options.position.y,
        options.position.z,
        options.orientation.x,
        options.orientation.y,
        options.orientation.z,
        options.orientation.w,
    ];
    if env
        .set_boolean_array_region(fields, 0, &field_values)
        .is_err()
        || env
            .set_double_array_region(values, 0, &option_values)
            .is_err()
    {
        sys::MLN_STATUS_INVALID_ARGUMENT
    } else {
        sys::MLN_STATUS_OK
    }
}

fn read_projection_mode(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    booleans: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<sys::mln_projection_mode, jint> {
    if !projection_mode_arrays_are_valid(env, fields, booleans, values) {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut field_values = [0 as jboolean; PROJECTION_MODE_FIELD_COUNT];
    let mut boolean_values = [0 as jboolean; PROJECTION_MODE_BOOLEAN_COUNT];
    let mut option_values = [0.0_f64; PROJECTION_MODE_VALUE_COUNT];
    if env
        .get_boolean_array_region(fields, 0, &mut field_values)
        .is_err()
        || env
            .get_boolean_array_region(booleans, 0, &mut boolean_values)
            .is_err()
        || env
            .get_double_array_region(values, 0, &mut option_values)
            .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut mode = unsafe { sys::mln_projection_mode_default() };
    if field_values[PROJECTION_MODE_FIELD_AXONOMETRIC] != 0 {
        mode.fields |= sys::MLN_PROJECTION_MODE_AXONOMETRIC;
        mode.axonometric = boolean_values[PROJECTION_MODE_BOOLEAN_AXONOMETRIC] != 0;
    }
    if field_values[PROJECTION_MODE_FIELD_X_SKEW] != 0 {
        mode.fields |= sys::MLN_PROJECTION_MODE_X_SKEW;
        mode.x_skew = option_values[PROJECTION_MODE_VALUE_X_SKEW];
    }
    if field_values[PROJECTION_MODE_FIELD_Y_SKEW] != 0 {
        mode.fields |= sys::MLN_PROJECTION_MODE_Y_SKEW;
        mode.y_skew = option_values[PROJECTION_MODE_VALUE_Y_SKEW];
    }
    Ok(mode)
}

fn write_projection_mode_arrays(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    booleans: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
    mode: &sys::mln_projection_mode,
) -> jint {
    let field_values = [
        jboolean::from(mode.fields & sys::MLN_PROJECTION_MODE_AXONOMETRIC != 0),
        jboolean::from(mode.fields & sys::MLN_PROJECTION_MODE_X_SKEW != 0),
        jboolean::from(mode.fields & sys::MLN_PROJECTION_MODE_Y_SKEW != 0),
    ];
    let boolean_values = [jboolean::from(mode.axonometric)];
    let option_values = [mode.x_skew, mode.y_skew];
    if env
        .set_boolean_array_region(fields, 0, &field_values)
        .is_err()
        || env
            .set_boolean_array_region(booleans, 0, &boolean_values)
            .is_err()
        || env
            .set_double_array_region(values, 0, &option_values)
            .is_err()
    {
        sys::MLN_STATUS_INVALID_ARGUMENT
    } else {
        sys::MLN_STATUS_OK
    }
}

fn read_bound_options(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<sys::mln_bound_options, jint> {
    if !bound_arrays_are_valid(env, fields, values) {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut field_values = [0 as jboolean; BOUND_FIELD_COUNT];
    let mut option_values = [0.0_f64; BOUND_VALUE_COUNT];
    if env
        .get_boolean_array_region(fields, 0, &mut field_values)
        .is_err()
        || env
            .get_double_array_region(values, 0, &mut option_values)
            .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }

    let mut bounds = unsafe { sys::mln_bound_options_default() };
    if field_values[BOUND_FIELD_BOUNDS] != 0 {
        bounds.fields |= sys::MLN_BOUND_OPTION_BOUNDS;
        bounds.bounds.southwest.latitude = option_values[BOUND_VALUE_SW_LATITUDE];
        bounds.bounds.southwest.longitude = option_values[BOUND_VALUE_SW_LONGITUDE];
        bounds.bounds.northeast.latitude = option_values[BOUND_VALUE_NE_LATITUDE];
        bounds.bounds.northeast.longitude = option_values[BOUND_VALUE_NE_LONGITUDE];
    }
    if field_values[BOUND_FIELD_MIN_ZOOM] != 0 {
        bounds.fields |= sys::MLN_BOUND_OPTION_MIN_ZOOM;
        bounds.min_zoom = option_values[BOUND_VALUE_MIN_ZOOM];
    }
    if field_values[BOUND_FIELD_MAX_ZOOM] != 0 {
        bounds.fields |= sys::MLN_BOUND_OPTION_MAX_ZOOM;
        bounds.max_zoom = option_values[BOUND_VALUE_MAX_ZOOM];
    }
    if field_values[BOUND_FIELD_MIN_PITCH] != 0 {
        bounds.fields |= sys::MLN_BOUND_OPTION_MIN_PITCH;
        bounds.min_pitch = option_values[BOUND_VALUE_MIN_PITCH];
    }
    if field_values[BOUND_FIELD_MAX_PITCH] != 0 {
        bounds.fields |= sys::MLN_BOUND_OPTION_MAX_PITCH;
        bounds.max_pitch = option_values[BOUND_VALUE_MAX_PITCH];
    }
    Ok(bounds)
}

fn write_bound_arrays(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
    bounds: &sys::mln_bound_options,
) -> jint {
    let mut field_values = [0 as jboolean; BOUND_FIELD_COUNT];
    let mut option_values = [0.0_f64; BOUND_VALUE_COUNT];
    field_values[BOUND_FIELD_BOUNDS] =
        jboolean::from(bounds.fields & sys::MLN_BOUND_OPTION_BOUNDS != 0);
    field_values[BOUND_FIELD_MIN_ZOOM] =
        jboolean::from(bounds.fields & sys::MLN_BOUND_OPTION_MIN_ZOOM != 0);
    field_values[BOUND_FIELD_MAX_ZOOM] =
        jboolean::from(bounds.fields & sys::MLN_BOUND_OPTION_MAX_ZOOM != 0);
    field_values[BOUND_FIELD_MIN_PITCH] =
        jboolean::from(bounds.fields & sys::MLN_BOUND_OPTION_MIN_PITCH != 0);
    field_values[BOUND_FIELD_MAX_PITCH] =
        jboolean::from(bounds.fields & sys::MLN_BOUND_OPTION_MAX_PITCH != 0);
    option_values[BOUND_VALUE_SW_LATITUDE] = bounds.bounds.southwest.latitude;
    option_values[BOUND_VALUE_SW_LONGITUDE] = bounds.bounds.southwest.longitude;
    option_values[BOUND_VALUE_NE_LATITUDE] = bounds.bounds.northeast.latitude;
    option_values[BOUND_VALUE_NE_LONGITUDE] = bounds.bounds.northeast.longitude;
    option_values[BOUND_VALUE_MIN_ZOOM] = bounds.min_zoom;
    option_values[BOUND_VALUE_MAX_ZOOM] = bounds.max_zoom;
    option_values[BOUND_VALUE_MIN_PITCH] = bounds.min_pitch;
    option_values[BOUND_VALUE_MAX_PITCH] = bounds.max_pitch;

    if env
        .set_boolean_array_region(fields, 0, &field_values)
        .is_err()
        || env
            .set_double_array_region(values, 0, &option_values)
            .is_err()
    {
        sys::MLN_STATUS_INVALID_ARGUMENT
    } else {
        sys::MLN_STATUS_OK
    }
}

fn read_camera_options(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<sys::mln_camera_options, jint> {
    if !camera_arrays_are_valid(env, fields, values) {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut field_values = [0 as jboolean; CAMERA_FIELD_COUNT];
    let mut option_values = [0.0_f64; CAMERA_VALUE_COUNT];
    if env
        .get_boolean_array_region(fields, 0, &mut field_values)
        .is_err()
        || env
            .get_double_array_region(values, 0, &mut option_values)
            .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }

    let mut camera = unsafe { sys::mln_camera_options_default() };
    if field_values[CAMERA_FIELD_CENTER] != 0 {
        camera.fields |= sys::MLN_CAMERA_OPTION_CENTER;
        camera.latitude = option_values[CAMERA_VALUE_LATITUDE];
        camera.longitude = option_values[CAMERA_VALUE_LONGITUDE];
    }
    if field_values[CAMERA_FIELD_CENTER_ALTITUDE] != 0 {
        camera.fields |= sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE;
        camera.center_altitude = option_values[CAMERA_VALUE_CENTER_ALTITUDE];
    }
    if field_values[CAMERA_FIELD_PADDING] != 0 {
        camera.fields |= sys::MLN_CAMERA_OPTION_PADDING;
        camera.padding.top = option_values[CAMERA_VALUE_PADDING_TOP];
        camera.padding.left = option_values[CAMERA_VALUE_PADDING_LEFT];
        camera.padding.bottom = option_values[CAMERA_VALUE_PADDING_BOTTOM];
        camera.padding.right = option_values[CAMERA_VALUE_PADDING_RIGHT];
    }
    if field_values[CAMERA_FIELD_ANCHOR] != 0 {
        camera.fields |= sys::MLN_CAMERA_OPTION_ANCHOR;
        camera.anchor.x = option_values[CAMERA_VALUE_ANCHOR_X];
        camera.anchor.y = option_values[CAMERA_VALUE_ANCHOR_Y];
    }
    if field_values[CAMERA_FIELD_ZOOM] != 0 {
        camera.fields |= sys::MLN_CAMERA_OPTION_ZOOM;
        camera.zoom = option_values[CAMERA_VALUE_ZOOM];
    }
    if field_values[CAMERA_FIELD_BEARING] != 0 {
        camera.fields |= sys::MLN_CAMERA_OPTION_BEARING;
        camera.bearing = option_values[CAMERA_VALUE_BEARING];
    }
    if field_values[CAMERA_FIELD_PITCH] != 0 {
        camera.fields |= sys::MLN_CAMERA_OPTION_PITCH;
        camera.pitch = option_values[CAMERA_VALUE_PITCH];
    }
    if field_values[CAMERA_FIELD_ROLL] != 0 {
        camera.fields |= sys::MLN_CAMERA_OPTION_ROLL;
        camera.roll = option_values[CAMERA_VALUE_ROLL];
    }
    if field_values[CAMERA_FIELD_FOV] != 0 {
        camera.fields |= sys::MLN_CAMERA_OPTION_FOV;
        camera.field_of_view = option_values[CAMERA_VALUE_FOV];
    }
    Ok(camera)
}

fn write_camera_arrays(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
    camera: &sys::mln_camera_options,
) -> jint {
    let mut field_values = [0 as jboolean; CAMERA_FIELD_COUNT];
    let mut option_values = [0.0_f64; CAMERA_VALUE_COUNT];
    field_values[CAMERA_FIELD_CENTER] =
        jboolean::from(camera.fields & sys::MLN_CAMERA_OPTION_CENTER != 0);
    field_values[CAMERA_FIELD_CENTER_ALTITUDE] =
        jboolean::from(camera.fields & sys::MLN_CAMERA_OPTION_CENTER_ALTITUDE != 0);
    field_values[CAMERA_FIELD_PADDING] =
        jboolean::from(camera.fields & sys::MLN_CAMERA_OPTION_PADDING != 0);
    field_values[CAMERA_FIELD_ANCHOR] =
        jboolean::from(camera.fields & sys::MLN_CAMERA_OPTION_ANCHOR != 0);
    field_values[CAMERA_FIELD_ZOOM] =
        jboolean::from(camera.fields & sys::MLN_CAMERA_OPTION_ZOOM != 0);
    field_values[CAMERA_FIELD_BEARING] =
        jboolean::from(camera.fields & sys::MLN_CAMERA_OPTION_BEARING != 0);
    field_values[CAMERA_FIELD_PITCH] =
        jboolean::from(camera.fields & sys::MLN_CAMERA_OPTION_PITCH != 0);
    field_values[CAMERA_FIELD_ROLL] =
        jboolean::from(camera.fields & sys::MLN_CAMERA_OPTION_ROLL != 0);
    field_values[CAMERA_FIELD_FOV] =
        jboolean::from(camera.fields & sys::MLN_CAMERA_OPTION_FOV != 0);
    option_values[CAMERA_VALUE_LATITUDE] = camera.latitude;
    option_values[CAMERA_VALUE_LONGITUDE] = camera.longitude;
    option_values[CAMERA_VALUE_CENTER_ALTITUDE] = camera.center_altitude;
    option_values[CAMERA_VALUE_PADDING_TOP] = camera.padding.top;
    option_values[CAMERA_VALUE_PADDING_LEFT] = camera.padding.left;
    option_values[CAMERA_VALUE_PADDING_BOTTOM] = camera.padding.bottom;
    option_values[CAMERA_VALUE_PADDING_RIGHT] = camera.padding.right;
    option_values[CAMERA_VALUE_ANCHOR_X] = camera.anchor.x;
    option_values[CAMERA_VALUE_ANCHOR_Y] = camera.anchor.y;
    option_values[CAMERA_VALUE_ZOOM] = camera.zoom;
    option_values[CAMERA_VALUE_BEARING] = camera.bearing;
    option_values[CAMERA_VALUE_PITCH] = camera.pitch;
    option_values[CAMERA_VALUE_ROLL] = camera.roll;
    option_values[CAMERA_VALUE_FOV] = camera.field_of_view;

    if env
        .set_boolean_array_region(fields, 0, &field_values)
        .is_err()
        || env
            .set_double_array_region(values, 0, &option_values)
            .is_err()
    {
        sys::MLN_STATUS_INVALID_ARGUMENT
    } else {
        sys::MLN_STATUS_OK
    }
}

fn optional_animation(
    env: &JNIEnv<'_>,
    has_animation: jboolean,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<Option<sys::mln_animation_options>, jint> {
    if has_animation != 0 {
        read_animation_options(env, fields, values).map(Some)
    } else {
        Ok(None)
    }
}

fn animation_ptr(
    animation: &Option<sys::mln_animation_options>,
) -> *const sys::mln_animation_options {
    animation
        .as_ref()
        .map_or(std::ptr::null(), |animation| animation as *const _)
}

fn read_animation_options(
    env: &JNIEnv<'_>,
    fields: &JBooleanArray<'_>,
    values: &JDoubleArray<'_>,
) -> Result<sys::mln_animation_options, jint> {
    if !animation_arrays_are_valid(env, fields, values) {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut field_values = [0 as jboolean; ANIMATION_FIELD_COUNT];
    let mut option_values = [0.0_f64; ANIMATION_VALUE_COUNT];
    if env
        .get_boolean_array_region(fields, 0, &mut field_values)
        .is_err()
        || env
            .get_double_array_region(values, 0, &mut option_values)
            .is_err()
    {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }

    let mut animation = unsafe { sys::mln_animation_options_default() };
    if field_values[ANIMATION_FIELD_DURATION] != 0 {
        animation.fields |= sys::MLN_ANIMATION_OPTION_DURATION;
        animation.duration_ms = option_values[ANIMATION_VALUE_DURATION];
    }
    if field_values[ANIMATION_FIELD_VELOCITY] != 0 {
        animation.fields |= sys::MLN_ANIMATION_OPTION_VELOCITY;
        animation.velocity = option_values[ANIMATION_VALUE_VELOCITY];
    }
    if field_values[ANIMATION_FIELD_MIN_ZOOM] != 0 {
        animation.fields |= sys::MLN_ANIMATION_OPTION_MIN_ZOOM;
        animation.min_zoom = option_values[ANIMATION_VALUE_MIN_ZOOM];
    }
    if field_values[ANIMATION_FIELD_EASING] != 0 {
        animation.fields |= sys::MLN_ANIMATION_OPTION_EASING;
        animation.easing.x1 = option_values[ANIMATION_VALUE_EASING_X1];
        animation.easing.y1 = option_values[ANIMATION_VALUE_EASING_Y1];
        animation.easing.x2 = option_values[ANIMATION_VALUE_EASING_X2];
        animation.easing.y2 = option_values[ANIMATION_VALUE_EASING_Y2];
    }
    Ok(animation)
}

fn map_get_bool(
    env: JNIEnv<'_>,
    map: jlong,
    out_value: JBooleanArray<'_>,
    getter: unsafe extern "C" fn(*mut sys::mln_map, *mut bool) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_value.is_null() || env.get_array_length(&out_value).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut value = false;
        let result = unsafe { getter(map as *mut sys::mln_map, &mut value) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_boolean_array_region(&out_value, 0, &[jboolean::from(value)])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_move_by(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    delta_x: f64,
    delta_y: f64,
) -> jint {
    catch_unwind(|| unsafe { sys::mln_map_move_by(map as *mut sys::mln_map, delta_x, delta_y) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_move_by_animated(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    delta_x: f64,
    delta_y: f64,
    has_animation: jboolean,
    animation_fields: JBooleanArray<'_>,
    animation_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let animation =
            match optional_animation(&env, has_animation, &animation_fields, &animation_values) {
                Ok(animation) => animation,
                Err(status) => return status,
            };
        unsafe {
            sys::mln_map_move_by_animated(
                map as *mut sys::mln_map,
                delta_x,
                delta_y,
                animation_ptr(&animation),
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_scale_by(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    scale: f64,
    has_anchor: jboolean,
    anchor_x: f64,
    anchor_y: f64,
) -> jint {
    catch_unwind(|| unsafe {
        let anchor = sys::mln_screen_point {
            x: anchor_x,
            y: anchor_y,
        };
        let anchor_ptr = if has_anchor != 0 {
            &anchor as *const sys::mln_screen_point
        } else {
            std::ptr::null()
        };
        sys::mln_map_scale_by(map as *mut sys::mln_map, scale, anchor_ptr)
    })
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_scale_by_animated(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    scale: f64,
    has_anchor: jboolean,
    anchor_x: f64,
    anchor_y: f64,
    has_animation: jboolean,
    animation_fields: JBooleanArray<'_>,
    animation_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let animation =
            match optional_animation(&env, has_animation, &animation_fields, &animation_values) {
                Ok(animation) => animation,
                Err(status) => return status,
            };
        unsafe {
            let anchor = sys::mln_screen_point {
                x: anchor_x,
                y: anchor_y,
            };
            let anchor_ptr = if has_anchor != 0 {
                &anchor as *const sys::mln_screen_point
            } else {
                std::ptr::null()
            };
            sys::mln_map_scale_by_animated(
                map as *mut sys::mln_map,
                scale,
                anchor_ptr,
                animation_ptr(&animation),
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_rotate_by(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    first_x: f64,
    first_y: f64,
    second_x: f64,
    second_y: f64,
) -> jint {
    catch_unwind(|| unsafe {
        sys::mln_map_rotate_by(
            map as *mut sys::mln_map,
            sys::mln_screen_point {
                x: first_x,
                y: first_y,
            },
            sys::mln_screen_point {
                x: second_x,
                y: second_y,
            },
        )
    })
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_rotate_by_animated(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    first_x: f64,
    first_y: f64,
    second_x: f64,
    second_y: f64,
    has_animation: jboolean,
    animation_fields: JBooleanArray<'_>,
    animation_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let animation =
            match optional_animation(&env, has_animation, &animation_fields, &animation_values) {
                Ok(animation) => animation,
                Err(status) => return status,
            };
        unsafe {
            sys::mln_map_rotate_by_animated(
                map as *mut sys::mln_map,
                sys::mln_screen_point {
                    x: first_x,
                    y: first_y,
                },
                sys::mln_screen_point {
                    x: second_x,
                    y: second_y,
                },
                animation_ptr(&animation),
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_pitch_by(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    pitch: f64,
) -> jint {
    catch_unwind(|| unsafe { sys::mln_map_pitch_by(map as *mut sys::mln_map, pitch) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_pitch_by_animated(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    pitch: f64,
    has_animation: jboolean,
    animation_fields: JBooleanArray<'_>,
    animation_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let animation =
            match optional_animation(&env, has_animation, &animation_fields, &animation_values) {
                Ok(animation) => animation,
                Err(status) => return status,
            };
        unsafe {
            sys::mln_map_pitch_by_animated(
                map as *mut sys::mln_map,
                pitch,
                animation_ptr(&animation),
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_cancel_transitions(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
) -> jint {
    catch_unwind(|| unsafe { sys::mln_map_cancel_transitions(map as *mut sys::mln_map) })
        .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_pixel_for_lat_lng(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    latitude: f64,
    longitude: f64,
    out_point: JDoubleArray<'_>,
) -> jint {
    projection_get_double_pair(env, out_point, |out| unsafe {
        let mut point = sys::mln_screen_point { x: 0.0, y: 0.0 };
        let result = sys::mln_map_pixel_for_lat_lng(
            map as *mut sys::mln_map,
            sys::mln_lat_lng {
                latitude,
                longitude,
            },
            &mut point,
        );
        out[0] = point.x;
        out[1] = point.y;
        result
    })
}

extern "system" fn map_lat_lng_for_pixel(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    x: f64,
    y: f64,
    out_coordinate: JDoubleArray<'_>,
) -> jint {
    projection_get_double_pair(env, out_coordinate, |out| unsafe {
        let mut coordinate = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        let result = sys::mln_map_lat_lng_for_pixel(
            map as *mut sys::mln_map,
            sys::mln_screen_point { x, y },
            &mut coordinate,
        );
        out[0] = coordinate.latitude;
        out[1] = coordinate.longitude;
        result
    })
}

extern "system" fn map_pixels_for_lat_lngs(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    coordinates: JDoubleArray<'_>,
    out_points: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let coordinate_values = match read_double_pairs(&env, &coordinates, &out_points) {
            Ok(values) => values,
            Err(status) => return status,
        };
        let coordinate_count = coordinate_values.len() / 2;
        let coordinates: Vec<_> = coordinate_values
            .chunks_exact(2)
            .map(|pair| sys::mln_lat_lng {
                latitude: pair[0],
                longitude: pair[1],
            })
            .collect();
        let mut points = vec![sys::mln_screen_point { x: 0.0, y: 0.0 }; coordinate_count];
        let result = unsafe {
            sys::mln_map_pixels_for_lat_lngs(
                map as *mut sys::mln_map,
                if coordinates.is_empty() {
                    std::ptr::null()
                } else {
                    coordinates.as_ptr()
                },
                coordinate_count,
                if points.is_empty() {
                    std::ptr::null_mut()
                } else {
                    points.as_mut_ptr()
                },
            )
        };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        let mut point_values = vec![0.0_f64; coordinate_values.len()];
        for (index, point) in points.iter().enumerate() {
            point_values[index * 2] = point.x;
            point_values[index * 2 + 1] = point.y;
        }
        if env
            .set_double_array_region(&out_points, 0, &point_values)
            .is_err()
        {
            sys::MLN_STATUS_INVALID_ARGUMENT
        } else {
            sys::MLN_STATUS_OK
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn map_lat_lngs_for_pixels(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    points: JDoubleArray<'_>,
    out_coordinates: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let point_values = match read_double_pairs(&env, &points, &out_coordinates) {
            Ok(values) => values,
            Err(status) => return status,
        };
        let point_count = point_values.len() / 2;
        let points: Vec<_> = point_values
            .chunks_exact(2)
            .map(|pair| sys::mln_screen_point {
                x: pair[0],
                y: pair[1],
            })
            .collect();
        let mut coordinates = vec![
            sys::mln_lat_lng {
                latitude: 0.0,
                longitude: 0.0,
            };
            point_count
        ];
        let result = unsafe {
            sys::mln_map_lat_lngs_for_pixels(
                map as *mut sys::mln_map,
                if points.is_empty() {
                    std::ptr::null()
                } else {
                    points.as_ptr()
                },
                point_count,
                if coordinates.is_empty() {
                    std::ptr::null_mut()
                } else {
                    coordinates.as_mut_ptr()
                },
            )
        };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        let mut coordinate_values = vec![0.0_f64; point_values.len()];
        for (index, coordinate) in coordinates.iter().enumerate() {
            coordinate_values[index * 2] = coordinate.latitude;
            coordinate_values[index * 2 + 1] = coordinate.longitude;
        }
        if env
            .set_double_array_region(&out_coordinates, 0, &coordinate_values)
            .is_err()
        {
            sys::MLN_STATUS_INVALID_ARGUMENT
        } else {
            sys::MLN_STATUS_OK
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

fn read_nonempty_coordinate_pairs(
    env: &JNIEnv<'_>,
    input: &JDoubleArray<'_>,
) -> Result<Vec<f64>, jint> {
    if input.is_null() {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let input_length = env
        .get_array_length(input)
        .map_err(|_| sys::MLN_STATUS_INVALID_ARGUMENT)?;
    if input_length <= 0 || input_length % 2 != 0 {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut values = vec![0.0_f64; input_length as usize];
    if env.get_double_array_region(input, 0, &mut values).is_err() {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    Ok(values)
}

fn read_edge_insets(
    env: &JNIEnv<'_>,
    input: &JDoubleArray<'_>,
) -> Result<sys::mln_edge_insets, jint> {
    if input.is_null() || env.get_array_length(input).unwrap_or(0) < 4 {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut values = [0.0_f64; 4];
    if env.get_double_array_region(input, 0, &mut values).is_err() {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    Ok(sys::mln_edge_insets {
        top: values[0],
        left: values[1],
        bottom: values[2],
        right: values[3],
    })
}

fn read_double_pairs(
    env: &JNIEnv<'_>,
    input: &JDoubleArray<'_>,
    output: &JDoubleArray<'_>,
) -> Result<Vec<f64>, jint> {
    if input.is_null() || output.is_null() {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let input_length = env
        .get_array_length(input)
        .map_err(|_| sys::MLN_STATUS_INVALID_ARGUMENT)?;
    let output_length = env
        .get_array_length(output)
        .map_err(|_| sys::MLN_STATUS_INVALID_ARGUMENT)?;
    if input_length < 0 || input_length % 2 != 0 || output_length < input_length {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    let mut values = vec![0.0_f64; input_length as usize];
    if env.get_double_array_region(input, 0, &mut values).is_err() {
        return Err(sys::MLN_STATUS_INVALID_ARGUMENT);
    }
    Ok(values)
}

extern "system" fn projection_create(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    map: jlong,
    out_projection: JLongArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_projection.is_null() || env.get_array_length(&out_projection).unwrap_or(0) < 1 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }

        let mut projection: *mut sys::mln_map_projection = std::ptr::null_mut();
        let result =
            unsafe { sys::mln_map_projection_create(map as *mut sys::mln_map, &mut projection) };
        if result == sys::MLN_STATUS_OK
            && env
                .set_long_array_region(&out_projection, 0, &[projection as jlong])
                .is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn projection_destroy(
    _env: JNIEnv<'_>,
    _class: JClass<'_>,
    projection: jlong,
) -> jint {
    catch_unwind(|| unsafe {
        sys::mln_map_projection_destroy(projection as *mut sys::mln_map_projection)
    })
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn projection_get_camera(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    projection: jlong,
    out_fields: JBooleanArray<'_>,
    out_values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if !camera_arrays_are_valid(&env, &out_fields, &out_values) {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut camera = unsafe { sys::mln_camera_options_default() };
        let result = unsafe {
            sys::mln_map_projection_get_camera(
                projection as *mut sys::mln_map_projection,
                &mut camera,
            )
        };
        if result != sys::MLN_STATUS_OK {
            return result;
        }
        write_camera_arrays(&env, &out_fields, &out_values, &camera)
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn projection_set_camera(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    projection: jlong,
    fields: JBooleanArray<'_>,
    values: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let camera = match read_camera_options(&env, &fields, &values) {
            Ok(camera) => camera,
            Err(status) => return status,
        };
        unsafe {
            sys::mln_map_projection_set_camera(projection as *mut sys::mln_map_projection, &camera)
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn projection_set_visible_coordinates(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    projection: jlong,
    coordinates: JDoubleArray<'_>,
    padding: JDoubleArray<'_>,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        let coordinate_values = match read_nonempty_coordinate_pairs(&env, &coordinates) {
            Ok(values) => values,
            Err(status) => return status,
        };
        let padding = match read_edge_insets(&env, &padding) {
            Ok(value) => value,
            Err(status) => return status,
        };
        let coordinates: Vec<_> = coordinate_values
            .chunks_exact(2)
            .map(|pair| sys::mln_lat_lng {
                latitude: pair[0],
                longitude: pair[1],
            })
            .collect();
        unsafe {
            sys::mln_map_projection_set_visible_coordinates(
                projection as *mut sys::mln_map_projection,
                coordinates.as_ptr(),
                coordinates.len(),
                padding,
            )
        }
    }))
    .unwrap_or(sys::MLN_STATUS_NATIVE_ERROR)
}

extern "system" fn projection_pixel_for_lat_lng(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    projection: jlong,
    latitude: f64,
    longitude: f64,
    out_point: JDoubleArray<'_>,
) -> jint {
    projection_get_double_pair(env, out_point, |out| unsafe {
        let mut point = sys::mln_screen_point { x: 0.0, y: 0.0 };
        let result = sys::mln_map_projection_pixel_for_lat_lng(
            projection as *mut sys::mln_map_projection,
            sys::mln_lat_lng {
                latitude,
                longitude,
            },
            &mut point,
        );
        out[0] = point.x;
        out[1] = point.y;
        result
    })
}

extern "system" fn projection_lat_lng_for_pixel(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    projection: jlong,
    x: f64,
    y: f64,
    out_coordinate: JDoubleArray<'_>,
) -> jint {
    projection_get_double_pair(env, out_coordinate, |out| unsafe {
        let mut coordinate = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        let result = sys::mln_map_projection_lat_lng_for_pixel(
            projection as *mut sys::mln_map_projection,
            sys::mln_screen_point { x, y },
            &mut coordinate,
        );
        out[0] = coordinate.latitude;
        out[1] = coordinate.longitude;
        result
    })
}

extern "system" fn projected_meters_for_lat_lng(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    latitude: f64,
    longitude: f64,
    out_meters: JDoubleArray<'_>,
) -> jint {
    projection_get_double_pair(env, out_meters, |out| unsafe {
        let mut meters = sys::mln_projected_meters {
            northing: 0.0,
            easting: 0.0,
        };
        let result = sys::mln_projected_meters_for_lat_lng(
            sys::mln_lat_lng {
                latitude,
                longitude,
            },
            &mut meters,
        );
        out[0] = meters.northing;
        out[1] = meters.easting;
        result
    })
}

extern "system" fn lat_lng_for_projected_meters(
    env: JNIEnv<'_>,
    _class: JClass<'_>,
    northing: f64,
    easting: f64,
    out_coordinate: JDoubleArray<'_>,
) -> jint {
    projection_get_double_pair(env, out_coordinate, |out| unsafe {
        let mut coordinate = sys::mln_lat_lng {
            latitude: 0.0,
            longitude: 0.0,
        };
        let result = sys::mln_lat_lng_for_projected_meters(
            sys::mln_projected_meters { northing, easting },
            &mut coordinate,
        );
        out[0] = coordinate.latitude;
        out[1] = coordinate.longitude;
        result
    })
}

fn projection_get_double_pair(
    env: JNIEnv<'_>,
    out_array: JDoubleArray<'_>,
    fill: impl FnOnce(&mut [f64; 2]) -> sys::mln_status,
) -> jint {
    catch_unwind(AssertUnwindSafe(|| {
        if out_array.is_null() || env.get_array_length(&out_array).unwrap_or(0) < 2 {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        let mut out = [0.0_f64; 2];
        let result = fill(&mut out);
        if result == sys::MLN_STATUS_OK && env.set_double_array_region(&out_array, 0, &out).is_err()
        {
            return sys::MLN_STATUS_INVALID_ARGUMENT;
        }
        result
    }))
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
