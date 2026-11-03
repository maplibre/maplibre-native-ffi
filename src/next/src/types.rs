// The C types of maplibre-native-ffi's headers (include/) that this crate
// reads or writes, as `#[repr(C)]` mirrors. C enums with a fixed underlying
// type are plain integers here, so an unknown value from the caller is never
// undefined behaviour. `layout.rs` exports their sizes, alignments and field
// offsets, and tests/c_api_test.c compares them with the C compiler's.

#![allow(dead_code)]

use std::ffi::{c_char, c_void};

pub type UserDataRelease = Option<unsafe extern "C" fn(*mut c_void)>;

pub const MLN_STATUS_OK: i32 = 0;
pub const MLN_STATUS_INVALID_ARGUMENT: i32 = -1;
pub const MLN_STATUS_INVALID_STATE: i32 = -2;
pub const MLN_STATUS_WRONG_THREAD: i32 = -3;
pub const MLN_STATUS_UNSUPPORTED: i32 = -4;
pub const MLN_STATUS_NATIVE_ERROR: i32 = -5;
pub const MLN_STATUS_CANCELLED: i32 = -6;
pub const MLN_STATUS_BUSY: i32 = -7;
pub const MLN_STATUS_TARGET_LOST: i32 = -8;
pub const MLN_STATUS_NOT_FOUND: i32 = -10;

pub const MLN_DIAGNOSTIC_MESSAGE_CAPACITY: usize = 4096;
pub const MLN_RENDER_BACKEND_FLAG_WEB_GPU: u32 = 1 << 3;

#[repr(C)]
pub struct MlnDiagnostic {
    pub size: u32,
    pub message: [c_char; MLN_DIAGNOSTIC_MESSAGE_CAPACITY],
}

/// Text that the library lends to a callback, NUL-terminated.
pub struct LentText(Vec<u8>);

impl LentText {
    pub fn new(text: impl Into<Vec<u8>>) -> LentText {
        let mut bytes = text.into();
        bytes.push(0);
        LentText(bytes)
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnBufferView {
    pub data: *const c_void,
    pub size: usize,
}

impl MlnBufferView {
    pub const EMPTY: MlnBufferView = MlnBufferView { data: std::ptr::null(), size: 0 };

    pub fn of(bytes: &[u8]) -> MlnBufferView {
        MlnBufferView { data: bytes.as_ptr().cast(), size: bytes.len() }
    }

    /// A view of lent text: its bytes, followed by a NUL that `size` leaves
    /// out. The header promises no terminator, but the C++ library lends
    /// every text with one and the ffi's tests check it.
    pub fn of_text(text: &LentText) -> MlnBufferView {
        MlnBufferView { data: text.0.as_ptr().cast(), size: text.0.len() - 1 }
    }

    /// The bytes, or `None` for a null `data` with a nonzero `size`.
    ///
    /// # Safety
    /// `data` points at `size` readable bytes, or `size` is 0.
    pub unsafe fn bytes<'a>(self) -> Option<&'a [u8]> {
        if self.size == 0 {
            Some(&[])
        } else if self.data.is_null() {
            None
        } else {
            // SAFETY: the caller's contract.
            Some(unsafe { std::slice::from_raw_parts(self.data.cast::<u8>(), self.size) })
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnLogicalSize {
    pub width: u32,
    pub height: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnLogicalExtent {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnCanonicalTileId {
    pub z: u32,
    pub x: u32,
    pub y: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnLatLng {
    pub latitude: f64,
    pub longitude: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnLatLngBounds {
    pub southwest: MlnLatLng,
    pub northeast: MlnLatLng,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnProjectedMeters {
    pub northing: f64,
    pub easting: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnScreenPoint {
    pub x: f64,
    pub y: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnEdgeInsets {
    pub top: f64,
    pub left: f64,
    pub bottom: f64,
    pub right: f64,
}

// completion.h

pub const MLN_COMMAND_DISPOSITION_COMMITTED: u32 = 0;
pub const MLN_COMMAND_DISPOSITION_FAILED: u32 = 1;
pub const MLN_COMMAND_DISPOSITION_CANCELLED: u32 = 2;

#[repr(C)]
pub struct MlnCompletionResult {
    pub size: u32,
    pub status: i32,
    pub disposition: u32,
    pub value_size: u32,
    pub generation: u64,
    pub render_update_generation: u64,
    pub diagnostic: MlnBufferView,
    pub value: *const c_void,
    pub value_count: usize,
}

pub type MlnCompletionCallback = Option<unsafe extern "C" fn(*mut c_void, *const MlnCompletionResult)>;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnCompletion {
    pub size: u32,
    pub callback: MlnCompletionCallback,
    pub user_data: *mut c_void,
    pub release_user_data: UserDataRelease,
    pub queue: u64,
}

// wake.h, delivery.h

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnWake {
    pub callback: Option<unsafe extern "C" fn(*mut c_void)>,
    pub user_data: *mut c_void,
    pub release_user_data: UserDataRelease,
    pub queue: u64,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnDeliveryQueueOptions {
    pub size: u32,
    pub wake: MlnWake,
    pub release_abandoned_user_data: UserDataRelease,
}

// runtime.h

pub const MLN_NETWORK_STATUS_ONLINE: u32 = 1;
pub const MLN_NETWORK_STATUS_OFFLINE: u32 = 2;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnRuntimeOptions {
    pub size: u32,
    pub asset_path: MlnBufferView,
    pub cache_path: MlnBufferView,
}

pub const MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE: u32 = 1;
pub const MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING: u32 = 2;
pub const MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE: u32 = 3;
pub const MLN_RUNTIME_EVENT_MAP_STYLE_LOADED: u32 = 4;
pub const MLN_RUNTIME_EVENT_MAP_LOADING_STARTED: u32 = 5;
pub const MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED: u32 = 6;
pub const MLN_RUNTIME_EVENT_MAP_LOADING_FAILED: u32 = 7;
pub const MLN_RUNTIME_EVENT_MAP_IDLE: u32 = 8;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_ERROR: u32 = 9;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED: u32 = 10;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED: u32 = 11;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED: u32 = 12;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED: u32 = 13;
pub const MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING: u32 = 14;
/// The highest event type; `MLN_RUNTIME_EVENT_FLAG_ALL` has a bit for each
/// type from 1 to this one.
pub const MLN_RUNTIME_EVENT_LAST: u32 = 25;
pub const MLN_RUNTIME_EVENT_FLAGS_ALL: u64 = ((1u64 << (MLN_RUNTIME_EVENT_LAST + 1)) - 1) & !1;
pub const MLN_RUNTIME_EVENT_FLAGS_RUNTIME: u64 = (1 << 17) | (1 << 18) | (1 << 19);
pub const MLN_RUNTIME_EVENT_FLAGS_MAP: u64 = MLN_RUNTIME_EVENT_FLAGS_ALL & !MLN_RUNTIME_EVENT_FLAGS_RUNTIME;

pub const MLN_RUNTIME_EVENT_SOURCE_RUNTIME: u32 = 0;
pub const MLN_RUNTIME_EVENT_SOURCE_MAP: u32 = 1;

pub const MLN_RUNTIME_EVENT_PAYLOAD_NONE: u32 = 0;
pub const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME: u32 = 1;
pub const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP: u32 = 2;

pub const MLN_RENDER_MODE_PARTIAL: u32 = 0;
pub const MLN_RENDER_MODE_FULL: u32 = 1;

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnRenderingStats {
    pub encoding_time_ms: f64,
    pub rendering_time_ms: f64,
    pub frame_count: i64,
    pub draw_call_count: i64,
    pub total_draw_call_count: i64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnRuntimeEventRenderFrame {
    pub mode: u32,
    pub needs_repaint: bool,
    pub placement_changed: bool,
    pub stats: MlnRenderingStats,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnRuntimeEventRenderMap {
    pub mode: u32,
}

/// `mln_runtime_event_payload`. Only the members this crate raises are named; the
/// padding member gives the union the C size (the offline region status
/// member is the largest, 72 bytes).
#[repr(C)]
#[derive(Clone, Copy)]
pub union MlnRuntimeEventPayload {
    pub render_frame: MlnRuntimeEventRenderFrame,
    pub render_map: MlnRuntimeEventRenderMap,
    pub padding: [u64; 9],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnRuntimeEvent {
    pub type_: u32,
    pub source_type: u32,
    pub source: u64,
    pub generation: u64,
    pub code: u32,
    pub payload_type: u32,
    pub message: MlnBufferView,
    pub payload: MlnRuntimeEventPayload,
}

#[repr(C)]
pub struct MlnEventBatchView {
    pub size: u32,
    pub event_size: u32,
    pub events: *const MlnRuntimeEvent,
    pub event_count: usize,
}

pub type MlnRuntimeEventCallback = Option<unsafe extern "C" fn(*mut c_void, *const MlnEventBatchView)>;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnRuntimeEventHandler {
    pub size: u32,
    pub callback: MlnRuntimeEventCallback,
    pub user_data: *mut c_void,
    pub release_user_data: UserDataRelease,
    pub queue: u64,
}

// map.h

pub const MLN_MAP_MODE_CONTINUOUS: u32 = 0;
pub const MLN_MAP_MODE_STATIC: u32 = 1;
pub const MLN_MAP_MODE_TILE: u32 = 2;

pub const MLN_MAP_OPTION_EVENT_MASK: u32 = 1 << 0;
pub const MLN_MAP_OPTION_TILE_CACHE_ENABLED: u32 = 1 << 1;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnMapOptions {
    pub size: u32,
    pub fields: u32,
    pub initial_extent: MlnLogicalExtent,
    pub map_mode: u32,
    pub fast_pfor_enabled: bool,
    pub event_mask: u64,
    pub tile_cache_enabled: bool,
}

pub const MLN_CAMERA_OPTION_CENTER: u32 = 1 << 0;
pub const MLN_CAMERA_OPTION_ZOOM: u32 = 1 << 1;
pub const MLN_CAMERA_OPTION_BEARING: u32 = 1 << 2;
pub const MLN_CAMERA_OPTION_PITCH: u32 = 1 << 3;
pub const MLN_CAMERA_OPTION_CENTER_ALTITUDE: u32 = 1 << 4;
pub const MLN_CAMERA_OPTION_PADDING: u32 = 1 << 5;
pub const MLN_CAMERA_OPTION_ANCHOR: u32 = 1 << 6;
pub const MLN_CAMERA_OPTION_ROLL: u32 = 1 << 7;
pub const MLN_CAMERA_OPTION_FOV: u32 = 1 << 8;
pub const MLN_CAMERA_OPTION_ALL: u32 = (1 << 9) - 1;

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnCameraOptions {
    pub size: u32,
    pub fields: u32,
    pub center: MlnLatLng,
    pub center_altitude: f64,
    pub padding: MlnEdgeInsets,
    pub anchor: MlnScreenPoint,
    pub zoom: f64,
    pub bearing: f64,
    pub pitch: f64,
    pub roll: f64,
    pub field_of_view: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnCamera {
    pub center: MlnLatLng,
    pub center_altitude: f64,
    pub padding: MlnEdgeInsets,
    pub zoom: f64,
    pub bearing: f64,
    pub pitch: f64,
    pub roll: f64,
    pub field_of_view: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnUnitBezier {
    pub x1: f64,
    pub y1: f64,
    pub x2: f64,
    pub y2: f64,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnCameraTransitionHandler {
    pub callback: Option<unsafe extern "C" fn(*mut c_void, *const c_void)>,
    pub user_data: *mut c_void,
    pub release_user_data: UserDataRelease,
    pub queue: u64,
}

pub const MLN_ANIMATION_OPTION_DURATION: u32 = 1 << 0;
pub const MLN_ANIMATION_OPTION_VELOCITY: u32 = 1 << 1;
pub const MLN_ANIMATION_OPTION_MIN_ZOOM: u32 = 1 << 2;
pub const MLN_ANIMATION_OPTION_EASING: u32 = 1 << 3;
pub const MLN_ANIMATION_OPTION_TRANSITION_ID: u32 = 1 << 4;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnAnimationOptions {
    pub fields: u32,
    pub duration_ms: f64,
    pub velocity: f64,
    pub min_zoom: f64,
    pub easing: MlnUnitBezier,
    pub transition_id: u64,
    pub end_handler: MlnCameraTransitionHandler,
}

pub const MLN_CAMERA_UPDATE_MODE_JUMP: u32 = 0;
pub const MLN_CAMERA_UPDATE_MODE_EASE: u32 = 1;
pub const MLN_CAMERA_UPDATE_MODE_FLY: u32 = 2;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnCameraUpdate {
    pub size: u32,
    pub mode: u32,
    pub camera: MlnCameraOptions,
    pub animation: MlnAnimationOptions,
    pub gesture_phase: u32,
    pub reserved: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnVec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnQuaternion {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnFreeCamera {
    pub position: MlnVec3,
    pub orientation: MlnQuaternion,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnCameraBounds {
    pub bounds: MlnLatLngBounds,
    pub unbounded: bool,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub min_pitch: f64,
    pub max_pitch: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnProjectionMode {
    pub axonometric: bool,
    pub x_skew: f64,
    pub y_skew: f64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnMapViewport {
    pub north_orientation: u32,
    pub constrain_mode: u32,
    pub viewport_mode: u32,
    pub frustum_offset: MlnEdgeInsets,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnMapView {
    pub size: u32,
    pub camera: MlnCamera,
    pub extent: MlnLogicalExtent,
    pub projection_mode: MlnProjectionMode,
    pub viewport: MlnMapViewport,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnMapTileSettings {
    pub prefetch_zoom_delta: u32,
    pub lod_min_radius: f64,
    pub lod_scale: f64,
    pub lod_pitch_threshold: f64,
    pub lod_zoom_shift: f64,
    pub lod_mode: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnMapSnapshot {
    pub size: u32,
    pub debug_flags: u32,
    pub generation: u64,
    pub fully_loaded: bool,
    pub rendering_stats_view_enabled: bool,
    pub gesture_in_progress: bool,
    pub transition_in_progress: bool,
    pub event_mask: u64,
    pub latest_render_update_generation: u64,
    pub tile: MlnMapTileSettings,
    pub bounds: MlnCameraBounds,
    pub free_camera: MlnFreeCamera,
    pub view: MlnMapView,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnCameraQueryResult {
    pub generation: u64,
    pub camera: MlnCamera,
}

// render_target.h, render_session.h, offscreen.h, texture.h

pub const MLN_RENDER_DRIVER_CORE_WORKER: u32 = 1;
pub const MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD: u32 = 2;

pub const MLN_RENDER_SESSION_CAPABILITY_READBACK: u32 = 1 << 1;
pub const MLN_GPU_SYNC_CPU_COMPLETE: u32 = 0;
pub const MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE: u32 = 1 << 0;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnQueueLock {
    pub lock: Option<unsafe extern "C" fn(*mut c_void)>,
    pub unlock: Option<unsafe extern "C" fn(*mut c_void)>,
    pub user_data: *mut c_void,
    pub release_user_data: UserDataRelease,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnRenderSessionAttachOptions {
    pub size: u32,
    pub driver: u32,
    pub producer_sync_kinds: u32,
    pub frame_wake: MlnWake,
    pub driver_work_wake: MlnWake,
    pub queue_lock: MlnQueueLock,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnRenderSessionCapabilities {
    pub size: u32,
    pub driver: u32,
    pub texture_ring_depth: u32,
    pub flags: u32,
    pub producer_sync_kind: u32,
    pub consumer_sync_kinds: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnOffscreenDescriptor {
    pub size: u32,
    pub logical_size: MlnLogicalSize,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnOffscreenDeviceOptions {
    pub size: u32,
    pub fields: u32,
    pub adapter_index: u32,
    pub single_context: bool,
    pub client_api: u32,
}

pub const MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX: u32 = 1 << 0;
pub const MLN_OFFSCREEN_DEVICE_OPTION_SINGLE_CONTEXT: u32 = 1 << 1;
pub const MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API: u32 = 1 << 2;
pub const MLN_OPEN_GL_CLIENT_API_UNSPECIFIED: u32 = 0;

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnTextureImageInfo {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct MlnTextureReadbackResult {
    pub data: MlnBufferView,
    pub info: MlnTextureImageInfo,
}

pub const MLN_RENDER_RESULT_RENDERED: u32 = 0;
pub const MLN_RENDER_RESULT_NO_UPDATE: u32 = 1;
pub const MLN_RENDER_RESULT_TARGET_NOT_READY: u32 = 2;
pub const MLN_RENDER_RESULT_SUPERSEDED: u32 = 3;
pub const MLN_RENDER_RESULT_FAILED: u32 = 4;

pub const MLN_RENDER_SESSION_STATE_ATTACHING: u32 = 1;
pub const MLN_RENDER_SESSION_STATE_ATTACHED: u32 = 2;
pub const MLN_RENDER_SESSION_STATE_DETACHING: u32 = 3;
pub const MLN_RENDER_SESSION_STATE_DETACHED: u32 = 4;
pub const MLN_RENDER_SESSION_STATE_FAILED: u32 = 5;
pub const MLN_RENDER_SESSION_STATE_ABANDONED: u32 = 6;

pub const MLN_FRAME_DEMAND_FORCE: u32 = 1 << 0;
pub const MLN_FRAME_DEMAND_WAIT_FOR_UPDATE: u32 = 1 << 2;
pub const MLN_FRAME_DEMAND_TOKEN_STILL_IMAGE: u64 = u64::MAX;

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnFrameDemand {
    pub size: u32,
    pub flags: u32,
    pub token: u64,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnRenderFrameResult {
    pub size: u32,
    pub disposition: u32,
    pub token: u64,
    pub map_update_generation: u64,
    pub generation: u64,
    pub frame_generation: u64,
    pub needs_repaint: bool,
    pub view: MlnMapView,
    pub mode: u32,
}

#[repr(C)]
pub struct MlnRenderFrameBatchView {
    pub size: u32,
    pub result_size: u32,
    pub results: *const MlnRenderFrameResult,
    pub result_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnRenderSessionSnapshot {
    pub size: u32,
    pub state: u32,
    pub extent: MlnLogicalExtent,
    pub generation: u64,
    pub frame_generation: u64,
    pub acquired_frame_count: u32,
    pub failure_status: i32,
    pub has_target: bool,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct MlnRenderAbandonResult {
    pub size: u32,
    pub disposition: u32,
    pub quarantined_resource_count: u32,
    pub reserved: u32,
}

/// Reads a caller's options struct whose first member is its `uint32_t size`.
///
/// Returns `None` for a null pointer or a `size` smaller than the struct
/// these headers declare; a larger `size` (a newer caller) reads the known
/// prefix.
///
/// # Safety
/// `ptr` is null or points at a readable `T` whose first field is `u32 size`.
pub unsafe fn read_sized<T: Copy>(ptr: *const T) -> Option<T> {
    if ptr.is_null() {
        return None;
    }
    // SAFETY: the caller's contract; `size` is the first field.
    let size = unsafe { *ptr.cast::<u32>() } as usize;
    if size < std::mem::size_of::<T>() {
        return None;
    }
    // SAFETY: as above.
    Some(unsafe { *ptr })
}

/// Writes `value` to a caller's out struct whose `size` field says how large
/// it is; fails like `read_sized` for a null pointer or a short struct.
///
/// # Safety
/// `ptr` is null or points at a writable `T` whose first field is `u32 size`.
pub unsafe fn write_sized<T: Copy>(ptr: *mut T, mut value: T) -> bool {
    if ptr.is_null() {
        return false;
    }
    // SAFETY: the caller's contract.
    let size = unsafe { *ptr.cast::<u32>() };
    if (size as usize) < std::mem::size_of::<T>() {
        return false;
    }
    // Keep the caller's size, as C++ does.
    // SAFETY: `value` is a `T` whose first field is the `u32 size`.
    unsafe { *(&mut value as *mut T).cast::<u32>() = size };
    // SAFETY: the caller's contract.
    unsafe { *ptr = value };
    true
}
