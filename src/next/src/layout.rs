// The layouts of the `#[repr(C)]` mirrors in types.rs, for
// tests/c_api_test.c, which compares them with the C compiler's sizeof,
// _Alignof and offsetof of the header's types. Not part of the C API.

use crate::types::*;
use std::ffi::{CStr, c_char};
use std::mem::{align_of, offset_of, size_of};

macro_rules! layouts {
    ($($c_name:literal => $ty:ty { $($c_field:literal => $field:ident),* $(,)? }),* $(,)?) => {
        fn lookup(name: &str) -> Option<(usize, usize)> {
            match name {
                $(
                    $c_name => Some((size_of::<$ty>(), align_of::<$ty>())),
                    $( concat!($c_name, ".", $c_field) => Some((offset_of!($ty, $field), 0)), )*
                )*
                _ => None,
            }
        }
    };
}

layouts! {
    "mln_diagnostic" => MlnDiagnostic { "message" => message },
    "mln_buffer_view" => MlnBufferView { "size" => size },
    "mln_logical_extent" => MlnLogicalExtent { "scale_factor" => scale_factor },
    "mln_completion_result" => MlnCompletionResult {
        "generation" => generation, "diagnostic" => diagnostic, "value" => value, "value_count" => value_count,
    },
    "mln_completion" => MlnCompletion { "callback" => callback, "queue" => queue },
    "mln_wake" => MlnWake { "queue" => queue },
    "mln_delivery_queue_options" => MlnDeliveryQueueOptions {
        "wake" => wake, "release_abandoned_user_data" => release_abandoned_user_data,
    },
    "mln_runtime_options" => MlnRuntimeOptions { "asset_path" => asset_path, "cache_path" => cache_path },
    "mln_runtime_event" => MlnRuntimeEvent {
        "generation" => generation, "code" => code, "payload_type" => payload_type, "message" => message,
        "payload" => payload,
    },
    "mln_runtime_event_payload" => MlnRuntimeEventPayload {},
    "mln_runtime_event_render_frame" => MlnRuntimeEventRenderFrame { "stats" => stats },
    "mln_event_batch_view" => MlnEventBatchView { "events" => events, "event_count" => event_count },
    "mln_runtime_event_handler" => MlnRuntimeEventHandler { "callback" => callback, "queue" => queue },
    "mln_map_options" => MlnMapOptions {
        "initial_extent" => initial_extent, "map_mode" => map_mode, "fast_pfor_enabled" => fast_pfor_enabled,
        "event_mask" => event_mask, "tile_cache_enabled" => tile_cache_enabled,
    },
    "mln_camera_options" => MlnCameraOptions {
        "center" => center, "center_altitude" => center_altitude, "padding" => padding, "anchor" => anchor,
        "zoom" => zoom, "field_of_view" => field_of_view,
    },
    "mln_camera" => MlnCamera { "zoom" => zoom, "field_of_view" => field_of_view },
    "mln_animation_options" => MlnAnimationOptions {
        "duration_ms" => duration_ms, "easing" => easing, "transition_id" => transition_id,
        "end_handler" => end_handler,
    },
    "mln_camera_update" => MlnCameraUpdate {
        "camera" => camera, "animation" => animation, "gesture_phase" => gesture_phase,
    },
    "mln_map_view" => MlnMapView {
        "camera" => camera, "extent" => extent, "projection_mode" => projection_mode, "viewport" => viewport,
    },
    "mln_map_snapshot" => MlnMapSnapshot {
        "generation" => generation, "event_mask" => event_mask,
        "latest_render_update_generation" => latest_render_update_generation, "tile" => tile,
        "bounds" => bounds, "free_camera" => free_camera, "view" => view,
    },
    "mln_camera_query_result" => MlnCameraQueryResult { "camera" => camera },
    "mln_render_session_attach_options" => MlnRenderSessionAttachOptions {
        "frame_wake" => frame_wake, "driver_work_wake" => driver_work_wake, "queue_lock" => queue_lock,
    },
    "mln_render_session_capabilities" => MlnRenderSessionCapabilities { "consumer_sync_kinds" => consumer_sync_kinds },
    "mln_offscreen_descriptor" => MlnOffscreenDescriptor { "logical_size" => logical_size },
    "mln_offscreen_device_options" => MlnOffscreenDeviceOptions { "client_api" => client_api },
    "mln_texture_readback_result" => MlnTextureReadbackResult { "info" => info },
    "mln_texture_image_info" => MlnTextureImageInfo { "byte_length" => byte_length },
    "mln_frame_demand" => MlnFrameDemand { "token" => token },
    "mln_render_frame_result" => MlnRenderFrameResult {
        "token" => token, "frame_generation" => frame_generation, "needs_repaint" => needs_repaint,
        "view" => view, "mode" => mode,
    },
    "mln_render_frame_batch_view" => MlnRenderFrameBatchView { "results" => results, "result_count" => result_count },
    "mln_render_session_snapshot" => MlnRenderSessionSnapshot {
        "extent" => extent, "generation" => generation, "failure_status" => failure_status, "has_target" => has_target,
    },
    "mln_render_abandon_result" => MlnRenderAbandonResult { "reserved" => reserved },
}

/// Writes the size and alignment of a C type, or the offset of `type.field`
/// (alignment 0), as this crate lays it out. Returns false for a name it doesn't
/// mirror.
///
/// # Safety
/// `name` is a NUL-terminated string; the out pointers are writable.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn mln_next_ffi_layout(name: *const c_char, out_size: *mut usize, out_align: *mut usize) -> bool {
    // SAFETY: the caller's contract.
    let Ok(name) = (unsafe { CStr::from_ptr(name) }).to_str() else {
        return false;
    };
    match lookup(name) {
        Some((size, align)) => {
            // SAFETY: the caller's contract.
            unsafe {
                *out_size = size;
                *out_align = align;
            }
            true
        }
        None => false,
    }
}
