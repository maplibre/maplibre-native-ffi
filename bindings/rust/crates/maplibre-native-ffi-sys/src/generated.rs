// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

pub const MLN_C_VERSION: u32 = 0;
pub type mln_ambient_cache_operation = u32;
pub const MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE: mln_ambient_cache_operation = 1;
pub const MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE: mln_ambient_cache_operation = 2;
pub const MLN_AMBIENT_CACHE_OPERATION_INVALIDATE: mln_ambient_cache_operation = 3;
pub const MLN_AMBIENT_CACHE_OPERATION_CLEAR: mln_ambient_cache_operation = 4;
pub type mln_animation_option_field = u32;
pub const MLN_ANIMATION_OPTION_DURATION: mln_animation_option_field = 1;
pub const MLN_ANIMATION_OPTION_VELOCITY: mln_animation_option_field = 2;
pub const MLN_ANIMATION_OPTION_MIN_ZOOM: mln_animation_option_field = 4;
pub const MLN_ANIMATION_OPTION_EASING: mln_animation_option_field = 8;
pub const MLN_ANIMATION_OPTION_TRANSITION_ID: mln_animation_option_field = 16;
pub type mln_bound_option_field = u32;
pub const MLN_BOUND_OPTION_BOUNDS: mln_bound_option_field = 1;
pub const MLN_BOUND_OPTION_MIN_ZOOM: mln_bound_option_field = 2;
pub const MLN_BOUND_OPTION_MAX_ZOOM: mln_bound_option_field = 4;
pub const MLN_BOUND_OPTION_MIN_PITCH: mln_bound_option_field = 8;
pub const MLN_BOUND_OPTION_MAX_PITCH: mln_bound_option_field = 16;
pub const MLN_BOUND_OPTION_UNBOUNDED: mln_bound_option_field = 32;
pub type mln_camera_change_mode = u32;
pub const MLN_CAMERA_CHANGE_MODE_IMMEDIATE: mln_camera_change_mode = 0;
pub const MLN_CAMERA_CHANGE_MODE_ANIMATED: mln_camera_change_mode = 1;
pub type mln_camera_delta_field = u32;
pub const MLN_CAMERA_DELTA_OFFSET: mln_camera_delta_field = 1;
pub const MLN_CAMERA_DELTA_SCALE: mln_camera_delta_field = 2;
pub const MLN_CAMERA_DELTA_BEARING: mln_camera_delta_field = 4;
pub const MLN_CAMERA_DELTA_PITCH: mln_camera_delta_field = 8;
pub const MLN_CAMERA_DELTA_ANCHOR: mln_camera_delta_field = 16;
pub type mln_camera_fit_option_field = u32;
pub const MLN_CAMERA_FIT_OPTION_PADDING: mln_camera_fit_option_field = 1;
pub const MLN_CAMERA_FIT_OPTION_BEARING: mln_camera_fit_option_field = 2;
pub const MLN_CAMERA_FIT_OPTION_PITCH: mln_camera_fit_option_field = 4;
pub type mln_camera_option_field = u32;
pub const MLN_CAMERA_OPTION_CENTER: mln_camera_option_field = 1;
pub const MLN_CAMERA_OPTION_ZOOM: mln_camera_option_field = 2;
pub const MLN_CAMERA_OPTION_BEARING: mln_camera_option_field = 4;
pub const MLN_CAMERA_OPTION_PITCH: mln_camera_option_field = 8;
pub const MLN_CAMERA_OPTION_CENTER_ALTITUDE: mln_camera_option_field = 16;
pub const MLN_CAMERA_OPTION_PADDING: mln_camera_option_field = 32;
pub const MLN_CAMERA_OPTION_ANCHOR: mln_camera_option_field = 64;
pub const MLN_CAMERA_OPTION_ROLL: mln_camera_option_field = 128;
pub const MLN_CAMERA_OPTION_FOV: mln_camera_option_field = 256;
pub type mln_camera_transition_outcome = u32;
pub const MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED: mln_camera_transition_outcome = 0;
pub const MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED: mln_camera_transition_outcome = 1;
pub const MLN_CAMERA_TRANSITION_OUTCOME_CLOSED: mln_camera_transition_outcome = 2;
pub type mln_camera_update_mode = u32;
pub const MLN_CAMERA_UPDATE_MODE_JUMP: mln_camera_update_mode = 0;
pub const MLN_CAMERA_UPDATE_MODE_EASE: mln_camera_update_mode = 1;
pub const MLN_CAMERA_UPDATE_MODE_FLY: mln_camera_update_mode = 2;
pub type mln_command_disposition = u32;
pub const MLN_COMMAND_DISPOSITION_COMMITTED: mln_command_disposition = 0;
pub const MLN_COMMAND_DISPOSITION_FAILED: mln_command_disposition = 1;
pub const MLN_COMMAND_DISPOSITION_CANCELLED: mln_command_disposition = 2;
pub type mln_constrain_mode = u32;
pub const MLN_CONSTRAIN_MODE_NONE: mln_constrain_mode = 0;
pub const MLN_CONSTRAIN_MODE_HEIGHT_ONLY: mln_constrain_mode = 1;
pub const MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT: mln_constrain_mode = 2;
pub const MLN_CONSTRAIN_MODE_SCREEN: mln_constrain_mode = 3;
pub type mln_custom_geometry_source_option_field = u32;
pub const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM: mln_custom_geometry_source_option_field = 1;
pub const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM: mln_custom_geometry_source_option_field = 2;
pub const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE: mln_custom_geometry_source_option_field = 4;
pub const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE: mln_custom_geometry_source_option_field = 8;
pub const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER: mln_custom_geometry_source_option_field = 16;
pub const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP: mln_custom_geometry_source_option_field = 32;
pub const MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP: mln_custom_geometry_source_option_field = 64;
pub type mln_custom_mvt_vector_source_option_field = u32;
pub const MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM: mln_custom_mvt_vector_source_option_field =
    1;
pub const MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM: mln_custom_mvt_vector_source_option_field =
    2;
pub type mln_feature_state_selector_field = u32;
pub const MLN_FEATURE_STATE_SELECTOR_FEATURE_ID: mln_feature_state_selector_field = 1;
pub type mln_frame_demand_flag = u32;
pub const MLN_FRAME_DEMAND_IF_NEEDED: mln_frame_demand_flag = 1;
pub const MLN_FRAME_DEMAND_PRESENT: mln_frame_demand_flag = 2;
pub const MLN_FRAME_DEMAND_WAIT_FOR_UPDATE: mln_frame_demand_flag = 4;
pub type mln_free_camera_option_field = u32;
pub const MLN_FREE_CAMERA_OPTION_POSITION: mln_free_camera_option_field = 1;
pub const MLN_FREE_CAMERA_OPTION_ORIENTATION: mln_free_camera_option_field = 2;
pub type mln_geojson_source_option_field = u32;
pub const MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM: mln_geojson_source_option_field = 1;
pub const MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM: mln_geojson_source_option_field = 2;
pub const MLN_GEOJSON_SOURCE_OPTION_TOLERANCE: mln_geojson_source_option_field = 4;
pub const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM: mln_geojson_source_option_field = 8;
pub const MLN_GEOJSON_SOURCE_OPTION_BUFFER: mln_geojson_source_option_field = 16;
pub const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS: mln_geojson_source_option_field = 32;
pub const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS: mln_geojson_source_option_field = 64;
pub const MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS: mln_geojson_source_option_field = 128;
pub const MLN_GEOJSON_SOURCE_OPTION_CLUSTER: mln_geojson_source_option_field = 256;
pub const MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING: mln_geojson_source_option_field = 512;
pub type mln_gesture_phase = u32;
pub const MLN_GESTURE_PHASE_NONE: mln_gesture_phase = 0;
pub const MLN_GESTURE_PHASE_BEGIN: mln_gesture_phase = 1;
pub const MLN_GESTURE_PHASE_UPDATE: mln_gesture_phase = 2;
pub const MLN_GESTURE_PHASE_END: mln_gesture_phase = 3;
pub const MLN_GESTURE_PHASE_CANCEL: mln_gesture_phase = 4;
pub type mln_gpu_sync_kind = u32;
pub const MLN_GPU_SYNC_CPU_COMPLETE: mln_gpu_sync_kind = 0;
pub const MLN_GPU_SYNC_OPENGL_FENCE: mln_gpu_sync_kind = 1;
pub type mln_log_event = u32;
pub const MLN_LOG_EVENT_GENERAL: mln_log_event = 0;
pub const MLN_LOG_EVENT_SETUP: mln_log_event = 1;
pub const MLN_LOG_EVENT_SHADER: mln_log_event = 2;
pub const MLN_LOG_EVENT_PARSE_STYLE: mln_log_event = 3;
pub const MLN_LOG_EVENT_PARSE_TILE: mln_log_event = 4;
pub const MLN_LOG_EVENT_RENDER: mln_log_event = 5;
pub const MLN_LOG_EVENT_STYLE: mln_log_event = 6;
pub const MLN_LOG_EVENT_DATABASE: mln_log_event = 7;
pub const MLN_LOG_EVENT_HTTP_REQUEST: mln_log_event = 8;
pub const MLN_LOG_EVENT_SPRITE: mln_log_event = 9;
pub const MLN_LOG_EVENT_IMAGE: mln_log_event = 10;
pub const MLN_LOG_EVENT_GRAPHICS_BACKEND: mln_log_event = 11;
pub const MLN_LOG_EVENT_JNI: mln_log_event = 12;
pub const MLN_LOG_EVENT_ANDROID: mln_log_event = 13;
pub const MLN_LOG_EVENT_CRASH: mln_log_event = 14;
pub const MLN_LOG_EVENT_GLYPH: mln_log_event = 15;
pub const MLN_LOG_EVENT_TIMING: mln_log_event = 16;
pub type mln_log_severity = u32;
pub const MLN_LOG_SEVERITY_INFO: mln_log_severity = 1;
pub const MLN_LOG_SEVERITY_WARNING: mln_log_severity = 2;
pub const MLN_LOG_SEVERITY_ERROR: mln_log_severity = 3;
pub type mln_log_severity_flag = u32;
pub const MLN_LOG_SEVERITY_FLAG_INFO: mln_log_severity_flag = 2;
pub const MLN_LOG_SEVERITY_FLAG_WARNING: mln_log_severity_flag = 4;
pub const MLN_LOG_SEVERITY_FLAG_ERROR: mln_log_severity_flag = 8;
pub const MLN_LOG_SEVERITY_FLAG_DEFAULT: mln_log_severity_flag = 6;
pub const MLN_LOG_SEVERITY_FLAG_ALL: mln_log_severity_flag = 14;
pub type mln_map_debug_flag = u32;
pub const MLN_MAP_DEBUG_TILE_BORDERS: mln_map_debug_flag = 2;
pub const MLN_MAP_DEBUG_PARSE_STATUS: mln_map_debug_flag = 4;
pub const MLN_MAP_DEBUG_TIMESTAMPS: mln_map_debug_flag = 8;
pub const MLN_MAP_DEBUG_COLLISION: mln_map_debug_flag = 16;
pub const MLN_MAP_DEBUG_OVERDRAW: mln_map_debug_flag = 32;
pub const MLN_MAP_DEBUG_STENCIL_CLIP: mln_map_debug_flag = 64;
pub const MLN_MAP_DEBUG_DEPTH_BUFFER: mln_map_debug_flag = 128;
pub type mln_map_mode = u32;
pub const MLN_MAP_MODE_CONTINUOUS: mln_map_mode = 0;
pub const MLN_MAP_MODE_STATIC: mln_map_mode = 1;
pub const MLN_MAP_MODE_TILE: mln_map_mode = 2;
pub type mln_map_tile_option_field = u32;
pub const MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA: mln_map_tile_option_field = 1;
pub const MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS: mln_map_tile_option_field = 2;
pub const MLN_MAP_TILE_OPTION_LOD_SCALE: mln_map_tile_option_field = 4;
pub const MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD: mln_map_tile_option_field = 8;
pub const MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT: mln_map_tile_option_field = 16;
pub const MLN_MAP_TILE_OPTION_LOD_MODE: mln_map_tile_option_field = 32;
pub type mln_map_viewport_option_field = u32;
pub const MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION: mln_map_viewport_option_field = 1;
pub const MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE: mln_map_viewport_option_field = 2;
pub const MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE: mln_map_viewport_option_field = 4;
pub const MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET: mln_map_viewport_option_field = 8;
pub type mln_network_status = u32;
pub const MLN_NETWORK_STATUS_ONLINE: mln_network_status = 1;
pub const MLN_NETWORK_STATUS_OFFLINE: mln_network_status = 2;
pub type mln_north_orientation = u32;
pub const MLN_NORTH_ORIENTATION_UP: mln_north_orientation = 0;
pub const MLN_NORTH_ORIENTATION_RIGHT: mln_north_orientation = 1;
pub const MLN_NORTH_ORIENTATION_DOWN: mln_north_orientation = 2;
pub const MLN_NORTH_ORIENTATION_LEFT: mln_north_orientation = 3;
pub type mln_offline_region_definition_type = u32;
pub const MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID: mln_offline_region_definition_type = 1;
pub const MLN_OFFLINE_REGION_DEFINITION_GEOMETRY: mln_offline_region_definition_type = 2;
pub type mln_offline_region_download_state = u32;
pub const MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE: mln_offline_region_download_state = 0;
pub const MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE: mln_offline_region_download_state = 1;
pub type mln_opengl_client_api = u32;
pub const MLN_OPENGL_CLIENT_API_UNSPECIFIED: mln_opengl_client_api = 0;
pub const MLN_OPENGL_CLIENT_API_GL: mln_opengl_client_api = 1;
pub const MLN_OPENGL_CLIENT_API_GLES: mln_opengl_client_api = 2;
pub type mln_opengl_context_ownership = u32;
pub const MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED: mln_opengl_context_ownership = 0;
pub const MLN_OPENGL_CONTEXT_OWNERSHIP_DEDICATED: mln_opengl_context_ownership = 1;
pub type mln_opengl_context_platform = u32;
pub const MLN_OPENGL_CONTEXT_PLATFORM_UNSPECIFIED: mln_opengl_context_platform = 0;
pub const MLN_OPENGL_CONTEXT_PLATFORM_WGL: mln_opengl_context_platform = 1;
pub const MLN_OPENGL_CONTEXT_PLATFORM_EGL: mln_opengl_context_platform = 2;
pub const MLN_OPENGL_CONTEXT_PLATFORM_WEBGL: mln_opengl_context_platform = 3;
pub type mln_opengl_context_provider_flag = u32;
pub const MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WGL: mln_opengl_context_provider_flag = 1;
pub const MLN_OPENGL_CONTEXT_PROVIDER_FLAG_EGL: mln_opengl_context_provider_flag = 2;
pub const MLN_OPENGL_CONTEXT_PROVIDER_FLAG_WEBGL: mln_opengl_context_provider_flag = 4;
pub type mln_projection_mode_field = u32;
pub const MLN_PROJECTION_MODE_AXONOMETRIC: mln_projection_mode_field = 1;
pub const MLN_PROJECTION_MODE_X_SKEW: mln_projection_mode_field = 2;
pub const MLN_PROJECTION_MODE_Y_SKEW: mln_projection_mode_field = 4;
pub type mln_render_abandon_disposition = u32;
pub const MLN_RENDER_ABANDON_DISPOSITION_CLEAN: mln_render_abandon_disposition = 0;
pub const MLN_RENDER_ABANDON_DISPOSITION_QUARANTINED: mln_render_abandon_disposition = 1;
pub type mln_render_backend_flag = u32;
pub const MLN_RENDER_BACKEND_FLAG_METAL: mln_render_backend_flag = 1;
pub const MLN_RENDER_BACKEND_FLAG_VULKAN: mln_render_backend_flag = 2;
pub const MLN_RENDER_BACKEND_FLAG_OPENGL: mln_render_backend_flag = 4;
pub const MLN_RENDER_BACKEND_FLAG_WEBGPU: mln_render_backend_flag = 8;
pub type mln_render_driver_kind = u32;
pub const MLN_RENDER_DRIVER_CORE_WORKER: mln_render_driver_kind = 1;
pub const MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD: mln_render_driver_kind = 2;
pub type mln_render_mode = u32;
pub const MLN_RENDER_MODE_PARTIAL: mln_render_mode = 0;
pub const MLN_RENDER_MODE_FULL: mln_render_mode = 1;
pub type mln_render_result = u32;
pub const MLN_RENDER_RESULT_RENDERED: mln_render_result = 0;
pub const MLN_RENDER_RESULT_NO_UPDATE: mln_render_result = 1;
pub const MLN_RENDER_RESULT_TARGET_NOT_READY: mln_render_result = 2;
pub const MLN_RENDER_RESULT_SUPERSEDED: mln_render_result = 3;
pub type mln_render_session_capability_flag = u32;
pub const MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION: mln_render_session_capability_flag = 1;
pub const MLN_RENDER_SESSION_CAPABILITY_READBACK: mln_render_session_capability_flag = 2;
pub const MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC: mln_render_session_capability_flag = 4;
pub const MLN_RENDER_SESSION_CAPABILITY_PRESENTATION: mln_render_session_capability_flag = 8;
pub type mln_render_session_state = u32;
pub const MLN_RENDER_SESSION_STATE_ATTACHING: mln_render_session_state = 1;
pub const MLN_RENDER_SESSION_STATE_ATTACHED: mln_render_session_state = 2;
pub const MLN_RENDER_SESSION_STATE_DETACHING: mln_render_session_state = 3;
pub const MLN_RENDER_SESSION_STATE_DETACHED: mln_render_session_state = 4;
pub const MLN_RENDER_SESSION_STATE_TARGET_LOST: mln_render_session_state = 5;
pub const MLN_RENDER_SESSION_STATE_ABANDONED: mln_render_session_state = 6;
pub type mln_rendered_feature_query_option_field = u32;
pub const MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS: mln_rendered_feature_query_option_field = 1;
pub type mln_rendered_query_geometry_type = u32;
pub const MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT: mln_rendered_query_geometry_type = 1;
pub const MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX: mln_rendered_query_geometry_type = 2;
pub const MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING: mln_rendered_query_geometry_type = 3;
pub type mln_resource_error_reason = u32;
pub const MLN_RESOURCE_ERROR_REASON_NONE: mln_resource_error_reason = 0;
pub const MLN_RESOURCE_ERROR_REASON_NOT_FOUND: mln_resource_error_reason = 1;
pub const MLN_RESOURCE_ERROR_REASON_SERVER: mln_resource_error_reason = 2;
pub const MLN_RESOURCE_ERROR_REASON_CONNECTION: mln_resource_error_reason = 3;
pub const MLN_RESOURCE_ERROR_REASON_RATE_LIMIT: mln_resource_error_reason = 4;
pub const MLN_RESOURCE_ERROR_REASON_OTHER: mln_resource_error_reason = 5;
pub type mln_resource_kind = u32;
pub const MLN_RESOURCE_KIND_UNKNOWN: mln_resource_kind = 0;
pub const MLN_RESOURCE_KIND_STYLE: mln_resource_kind = 1;
pub const MLN_RESOURCE_KIND_SOURCE: mln_resource_kind = 2;
pub const MLN_RESOURCE_KIND_TILE: mln_resource_kind = 3;
pub const MLN_RESOURCE_KIND_GLYPHS: mln_resource_kind = 4;
pub const MLN_RESOURCE_KIND_SPRITE_IMAGE: mln_resource_kind = 5;
pub const MLN_RESOURCE_KIND_SPRITE_JSON: mln_resource_kind = 6;
pub const MLN_RESOURCE_KIND_IMAGE: mln_resource_kind = 7;
pub type mln_resource_kind_flag = u32;
pub const MLN_RESOURCE_KIND_FLAG_UNKNOWN: mln_resource_kind_flag = 1;
pub const MLN_RESOURCE_KIND_FLAG_STYLE: mln_resource_kind_flag = 2;
pub const MLN_RESOURCE_KIND_FLAG_SOURCE: mln_resource_kind_flag = 4;
pub const MLN_RESOURCE_KIND_FLAG_TILE: mln_resource_kind_flag = 8;
pub const MLN_RESOURCE_KIND_FLAG_GLYPHS: mln_resource_kind_flag = 16;
pub const MLN_RESOURCE_KIND_FLAG_SPRITE_IMAGE: mln_resource_kind_flag = 32;
pub const MLN_RESOURCE_KIND_FLAG_SPRITE_JSON: mln_resource_kind_flag = 64;
pub const MLN_RESOURCE_KIND_FLAG_IMAGE: mln_resource_kind_flag = 128;
pub const MLN_RESOURCE_KIND_FLAG_ALL: mln_resource_kind_flag = 255;
pub type mln_resource_loading_method = u32;
pub const MLN_RESOURCE_LOADING_METHOD_ALL: mln_resource_loading_method = 0;
pub const MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY: mln_resource_loading_method = 1;
pub const MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY: mln_resource_loading_method = 2;
pub type mln_resource_priority = u32;
pub const MLN_RESOURCE_PRIORITY_REGULAR: mln_resource_priority = 0;
pub const MLN_RESOURCE_PRIORITY_LOW: mln_resource_priority = 1;
pub type mln_resource_request_field = u32;
pub const MLN_RESOURCE_REQUEST_RANGE: mln_resource_request_field = 1;
pub const MLN_RESOURCE_REQUEST_PRIOR_MODIFIED: mln_resource_request_field = 2;
pub const MLN_RESOURCE_REQUEST_PRIOR_EXPIRES: mln_resource_request_field = 4;
pub type mln_resource_response_field = u32;
pub const MLN_RESOURCE_RESPONSE_MODIFIED: mln_resource_response_field = 1;
pub const MLN_RESOURCE_RESPONSE_EXPIRES: mln_resource_response_field = 2;
pub const MLN_RESOURCE_RESPONSE_RETRY_AFTER: mln_resource_response_field = 4;
pub type mln_resource_response_status = u32;
pub const MLN_RESOURCE_RESPONSE_STATUS_OK: mln_resource_response_status = 0;
pub const MLN_RESOURCE_RESPONSE_STATUS_ERROR: mln_resource_response_status = 1;
pub const MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT: mln_resource_response_status = 2;
pub const MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED: mln_resource_response_status = 3;
pub type mln_resource_storage_policy = u32;
pub const MLN_RESOURCE_STORAGE_POLICY_PERMANENT: mln_resource_storage_policy = 0;
pub const MLN_RESOURCE_STORAGE_POLICY_VOLATILE: mln_resource_storage_policy = 1;
pub type mln_resource_usage = u32;
pub const MLN_RESOURCE_USAGE_ONLINE: mln_resource_usage = 0;
pub const MLN_RESOURCE_USAGE_OFFLINE: mln_resource_usage = 1;
pub type mln_runtime_event_flag = u64;
pub const MLN_RUNTIME_EVENT_FLAG_NONE: mln_runtime_event_flag = 0;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_WILL_CHANGE: mln_runtime_event_flag = 2;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_IS_CHANGING: mln_runtime_event_flag = 4;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_DID_CHANGE: mln_runtime_event_flag = 8;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED: mln_runtime_event_flag = 16;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_STARTED: mln_runtime_event_flag = 32;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FINISHED: mln_runtime_event_flag = 64;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FAILED: mln_runtime_event_flag = 128;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_IDLE: mln_runtime_event_flag = 256;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_ERROR: mln_runtime_event_flag = 512;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_STARTED: mln_runtime_event_flag = 1024;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_FINISHED: mln_runtime_event_flag = 2048;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_STARTED: mln_runtime_event_flag = 4096;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_FINISHED: mln_runtime_event_flag = 8192;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_IMAGE_MISSING: mln_runtime_event_flag = 16384;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_TILE_ACTION: mln_runtime_event_flag = 32768;
pub const MLN_RUNTIME_EVENT_FLAG_MAP_SOURCE_CHANGED: mln_runtime_event_flag = 65536;
pub const MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_STATUS_CHANGED: mln_runtime_event_flag = 131072;
pub const MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_RESPONSE_ERROR: mln_runtime_event_flag = 262144;
pub const MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: mln_runtime_event_flag =
    524288;
pub const MLN_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS: mln_runtime_event_flag = 131070;
pub const MLN_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS: mln_runtime_event_flag = 917504;
pub const MLN_RUNTIME_EVENT_FLAG_ALL: mln_runtime_event_flag = 1048574;
pub type mln_runtime_event_payload_type = u32;
pub const MLN_RUNTIME_EVENT_PAYLOAD_NONE: mln_runtime_event_payload_type = 0;
pub const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME: mln_runtime_event_payload_type = 1;
pub const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP: mln_runtime_event_payload_type = 2;
pub const MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION: mln_runtime_event_payload_type = 3;
pub const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS: mln_runtime_event_payload_type = 4;
pub const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR: mln_runtime_event_payload_type =
    5;
pub const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT:
    mln_runtime_event_payload_type = 6;
pub type mln_runtime_event_source_type = u32;
pub const MLN_RUNTIME_EVENT_SOURCE_RUNTIME: mln_runtime_event_source_type = 0;
pub const MLN_RUNTIME_EVENT_SOURCE_MAP: mln_runtime_event_source_type = 1;
pub type mln_runtime_event_type = u32;
pub const MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE: mln_runtime_event_type = 1;
pub const MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING: mln_runtime_event_type = 2;
pub const MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE: mln_runtime_event_type = 3;
pub const MLN_RUNTIME_EVENT_MAP_STYLE_LOADED: mln_runtime_event_type = 4;
pub const MLN_RUNTIME_EVENT_MAP_LOADING_STARTED: mln_runtime_event_type = 5;
pub const MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED: mln_runtime_event_type = 6;
pub const MLN_RUNTIME_EVENT_MAP_LOADING_FAILED: mln_runtime_event_type = 7;
pub const MLN_RUNTIME_EVENT_MAP_IDLE: mln_runtime_event_type = 8;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_ERROR: mln_runtime_event_type = 9;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED: mln_runtime_event_type = 10;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED: mln_runtime_event_type = 11;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED: mln_runtime_event_type = 12;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED: mln_runtime_event_type = 13;
pub const MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING: mln_runtime_event_type = 14;
pub const MLN_RUNTIME_EVENT_MAP_TILE_ACTION: mln_runtime_event_type = 15;
pub const MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED: mln_runtime_event_type = 16;
pub const MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED: mln_runtime_event_type = 17;
pub const MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR: mln_runtime_event_type = 18;
pub const MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: mln_runtime_event_type = 19;
pub type mln_source_feature_query_option_field = u32;
pub const MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS: mln_source_feature_query_option_field =
    1;
pub type mln_status = i32;
pub const MLN_STATUS_OK: mln_status = 0;
pub const MLN_STATUS_INVALID_ARGUMENT: mln_status = -1;
pub const MLN_STATUS_INVALID_STATE: mln_status = -2;
pub const MLN_STATUS_WRONG_THREAD: mln_status = -3;
pub const MLN_STATUS_UNSUPPORTED: mln_status = -4;
pub const MLN_STATUS_NATIVE_ERROR: mln_status = -5;
pub const MLN_STATUS_CANCELLED: mln_status = -6;
pub const MLN_STATUS_BUSY: mln_status = -7;
pub const MLN_STATUS_TARGET_LOST: mln_status = -8;
pub const MLN_STATUS_NOT_READY: mln_status = -9;
pub const MLN_STATUS_NOT_FOUND: mln_status = -10;
pub type mln_style_expression_input_field = u32;
pub const MLN_STYLE_EXPRESSION_INPUT_ZOOM: mln_style_expression_input_field = 1;
pub const MLN_STYLE_EXPRESSION_INPUT_COLOR_RAMP_PARAMETER: mln_style_expression_input_field = 2;
pub const MLN_STYLE_EXPRESSION_INPUT_ELEVATION: mln_style_expression_input_field = 4;
pub const MLN_STYLE_EXPRESSION_INPUT_CANONICAL_TILE_ID: mln_style_expression_input_field = 8;
pub type mln_style_expression_option_field = u32;
pub const MLN_STYLE_EXPRESSION_OPTION_EXPECTED_TYPE: mln_style_expression_option_field = 1;
pub type mln_style_expression_type_kind = u32;
pub const MLN_STYLE_EXPRESSION_TYPE_VALUE: mln_style_expression_type_kind = 0;
pub const MLN_STYLE_EXPRESSION_TYPE_NUMBER: mln_style_expression_type_kind = 1;
pub const MLN_STYLE_EXPRESSION_TYPE_STRING: mln_style_expression_type_kind = 2;
pub const MLN_STYLE_EXPRESSION_TYPE_BOOLEAN: mln_style_expression_type_kind = 3;
pub const MLN_STYLE_EXPRESSION_TYPE_COLOR: mln_style_expression_type_kind = 4;
pub const MLN_STYLE_EXPRESSION_TYPE_OBJECT: mln_style_expression_type_kind = 5;
pub const MLN_STYLE_EXPRESSION_TYPE_FORMATTED: mln_style_expression_type_kind = 6;
pub const MLN_STYLE_EXPRESSION_TYPE_IMAGE: mln_style_expression_type_kind = 7;
pub const MLN_STYLE_EXPRESSION_TYPE_PADDING: mln_style_expression_type_kind = 8;
pub const MLN_STYLE_EXPRESSION_TYPE_VARIABLE_ANCHOR_OFFSET_COLLECTION:
    mln_style_expression_type_kind = 9;
pub const MLN_STYLE_EXPRESSION_TYPE_ARRAY: mln_style_expression_type_kind = 10;
pub type mln_style_image_info_field = u32;
pub const MLN_STYLE_IMAGE_INFO_CONTENT: mln_style_image_info_field = 1;
pub const MLN_STYLE_IMAGE_INFO_TEXT_FIT_WIDTH: mln_style_image_info_field = 2;
pub const MLN_STYLE_IMAGE_INFO_TEXT_FIT_HEIGHT: mln_style_image_info_field = 4;
pub type mln_style_image_option_field = u32;
pub const MLN_STYLE_IMAGE_OPTION_PIXEL_RATIO: mln_style_image_option_field = 1;
pub const MLN_STYLE_IMAGE_OPTION_SDF: mln_style_image_option_field = 2;
pub const MLN_STYLE_IMAGE_OPTION_STRETCH_X: mln_style_image_option_field = 4;
pub const MLN_STYLE_IMAGE_OPTION_STRETCH_Y: mln_style_image_option_field = 8;
pub const MLN_STYLE_IMAGE_OPTION_CONTENT: mln_style_image_option_field = 16;
pub const MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH: mln_style_image_option_field = 32;
pub const MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT: mln_style_image_option_field = 64;
pub type mln_style_image_text_fit = u32;
pub const MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK: mln_style_image_text_fit = 0;
pub const MLN_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY: mln_style_image_text_fit = 1;
pub const MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL: mln_style_image_text_fit = 2;
pub type mln_style_layer_visibility = u32;
pub const MLN_STYLE_LAYER_VISIBILITY_VISIBLE: mln_style_layer_visibility = 0;
pub const MLN_STYLE_LAYER_VISIBILITY_NONE: mln_style_layer_visibility = 1;
pub type mln_style_raster_dem_encoding = u32;
pub const MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX: mln_style_raster_dem_encoding = 0;
pub const MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM: mln_style_raster_dem_encoding = 1;
pub type mln_style_source_info_field = u32;
pub const MLN_STYLE_SOURCE_INFO_TILEJSON: mln_style_source_info_field = 1;
pub const MLN_STYLE_SOURCE_INFO_BOUNDS: mln_style_source_info_field = 2;
pub const MLN_STYLE_SOURCE_INFO_TILE_SIZE: mln_style_source_info_field = 4;
pub const MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING: mln_style_source_info_field = 8;
pub const MLN_STYLE_SOURCE_INFO_RASTER_ENCODING: mln_style_source_info_field = 16;
pub type mln_style_source_type = u32;
pub const MLN_STYLE_SOURCE_TYPE_UNKNOWN: mln_style_source_type = 0;
pub const MLN_STYLE_SOURCE_TYPE_VECTOR: mln_style_source_type = 1;
pub const MLN_STYLE_SOURCE_TYPE_RASTER: mln_style_source_type = 2;
pub const MLN_STYLE_SOURCE_TYPE_RASTER_DEM: mln_style_source_type = 3;
pub const MLN_STYLE_SOURCE_TYPE_GEOJSON: mln_style_source_type = 4;
pub const MLN_STYLE_SOURCE_TYPE_IMAGE: mln_style_source_type = 5;
pub const MLN_STYLE_SOURCE_TYPE_VIDEO: mln_style_source_type = 6;
pub const MLN_STYLE_SOURCE_TYPE_ANNOTATIONS: mln_style_source_type = 7;
pub const MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR: mln_style_source_type = 8;
pub const MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR: mln_style_source_type = 9;
pub type mln_style_tile_scheme = u32;
pub const MLN_STYLE_TILE_SCHEME_XYZ: mln_style_tile_scheme = 0;
pub const MLN_STYLE_TILE_SCHEME_TMS: mln_style_tile_scheme = 1;
pub type mln_style_transition_option_field = u32;
pub const MLN_STYLE_TRANSITION_OPTION_DURATION: mln_style_transition_option_field = 1;
pub const MLN_STYLE_TRANSITION_OPTION_DELAY: mln_style_transition_option_field = 2;
pub const MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS:
    mln_style_transition_option_field = 4;
pub type mln_style_vector_tile_encoding = u32;
pub const MLN_STYLE_VECTOR_TILE_ENCODING_MVT: mln_style_vector_tile_encoding = 0;
pub const MLN_STYLE_VECTOR_TILE_ENCODING_MLT: mln_style_vector_tile_encoding = 1;
pub type mln_tile_lod_mode = u32;
pub const MLN_TILE_LOD_MODE_DEFAULT: mln_tile_lod_mode = 0;
pub const MLN_TILE_LOD_MODE_DISTANCE: mln_tile_lod_mode = 1;
pub type mln_tile_operation = u32;
pub const MLN_TILE_OPERATION_REQUESTED_FROM_CACHE: mln_tile_operation = 0;
pub const MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK: mln_tile_operation = 1;
pub const MLN_TILE_OPERATION_LOAD_FROM_NETWORK: mln_tile_operation = 2;
pub const MLN_TILE_OPERATION_LOAD_FROM_CACHE: mln_tile_operation = 3;
pub const MLN_TILE_OPERATION_START_PARSE: mln_tile_operation = 4;
pub const MLN_TILE_OPERATION_END_PARSE: mln_tile_operation = 5;
pub const MLN_TILE_OPERATION_ERROR: mln_tile_operation = 6;
pub const MLN_TILE_OPERATION_CANCELLED: mln_tile_operation = 7;
pub const MLN_TILE_OPERATION_NULL: mln_tile_operation = 8;
pub type mln_url_match_flag = u32;
pub const MLN_URL_MATCH_EXACT: mln_url_match_flag = 0;
pub const MLN_URL_MATCH_GLOB: mln_url_match_flag = 1;
pub type mln_viewport_mode = u32;
pub const MLN_VIEWPORT_MODE_DEFAULT: mln_viewport_mode = 0;
pub const MLN_VIEWPORT_MODE_FLIPPED_Y: mln_viewport_mode = 1;
pub type mln_webgl_context_kind = u32;
pub const MLN_WEBGL_CONTEXT_EXISTING: mln_webgl_context_kind = 0;
pub const MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS: mln_webgl_context_kind = 1;
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_animation_options {
    pub fields: mln_animation_option_fields,
    pub duration_ms: f64,
    pub velocity: f64,
    pub min_zoom: f64,
    pub easing: mln_unit_bezier,
    pub transition_id: u64,
    pub end_handler: mln_camera_transition_handler,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_bound_options {
    pub size: u32,
    pub fields: mln_bound_option_fields,
    pub bounds: mln_lat_lng_bounds,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub min_pitch: f64,
    pub max_pitch: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_buffer_view {
    pub data: *const std::ffi::c_void,
    pub size: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_delta {
    pub size: u32,
    pub fields: mln_camera_delta_fields,
    pub offset: mln_screen_point,
    pub scale: f64,
    pub bearing: f64,
    pub pitch: f64,
    pub anchor: mln_screen_point,
    pub animation: mln_animation_options,
    pub gesture_phase: mln_gesture_phase,
    pub reserved: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_fit_options {
    pub size: u32,
    pub fields: mln_camera_fit_option_fields,
    pub padding: mln_edge_insets,
    pub bearing: f64,
    pub pitch: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_options {
    pub size: u32,
    pub fields: mln_camera_option_fields,
    pub center: mln_lat_lng,
    pub center_altitude: f64,
    pub padding: mln_edge_insets,
    pub anchor: mln_screen_point,
    pub zoom: f64,
    pub bearing: f64,
    pub pitch: f64,
    pub roll: f64,
    pub field_of_view: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_query_result {
    pub generation: u64,
    pub camera: mln_camera_options,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_transition_end {
    pub size: u32,
    pub outcome: mln_camera_transition_outcome,
    pub generation: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_transition_handler {
    pub callback: mln_camera_transition_end_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
    pub queue: mln_delivery_queue,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_update {
    pub size: u32,
    pub mode: mln_camera_update_mode,
    pub camera: mln_camera_options,
    pub animation: mln_animation_options,
    pub gesture_phase: mln_gesture_phase,
    pub reserved: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_canonical_tile_id {
    pub z: u32,
    pub x: u32,
    pub y: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_completion {
    pub size: u32,
    pub callback: mln_completion_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
    pub queue: mln_delivery_queue,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_completion_result {
    pub size: u32,
    pub status: mln_status,
    pub disposition: mln_command_disposition,
    pub value_size: u32,
    pub generation: u64,
    pub render_update_generation: u64,
    pub diagnostic: mln_buffer_view,
    pub value: *const std::ffi::c_void,
    pub value_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_custom_geometry_source_options {
    pub size: u32,
    pub fields: mln_custom_geometry_source_option_fields,
    pub fetch_tile: mln_custom_source_fetch_tile_callback,
    pub cancel_tile: mln_custom_source_cancel_tile_callback,
    pub user_data: *mut std::ffi::c_void,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub tolerance: f64,
    pub tile_size: u32,
    pub buffer: u32,
    pub clip: bool,
    pub wrap: bool,
    pub release_user_data: mln_user_data_release,
    pub queue: mln_delivery_queue,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_custom_mvt_vector_source_options {
    pub size: u32,
    pub fields: mln_custom_mvt_vector_source_option_fields,
    pub fetch_tile: mln_custom_source_fetch_tile_callback,
    pub cancel_tile: mln_custom_source_cancel_tile_callback,
    pub user_data: *mut std::ffi::c_void,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub release_user_data: mln_user_data_release,
    pub queue: mln_delivery_queue,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_diagnostic {
    pub size: u32,
    pub message: [std::ffi::c_char; 4096],
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_edge_insets {
    pub top: f64,
    pub left: f64,
    pub bottom: f64,
    pub right: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_egl_context_descriptor {
    pub display: *mut std::ffi::c_void,
    pub config: *mut std::ffi::c_void,
    pub share_context: *mut std::ffi::c_void,
    pub client_api: mln_opengl_client_api,
    pub get_proc_address: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_event_batch_view {
    pub size: u32,
    pub event_size: u32,
    pub events: *const mln_runtime_event,
    pub event_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_feature_state_selector {
    pub size: u32,
    pub fields: mln_feature_state_selector_fields,
    pub source_id: mln_buffer_view,
    pub source_layer_id: mln_buffer_view,
    pub feature_id: mln_buffer_view,
    pub state_key: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_frame_demand {
    pub size: u32,
    pub flags: mln_frame_demand_flags,
    pub token: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_free_camera_options {
    pub size: u32,
    pub fields: mln_free_camera_option_fields,
    pub position: mln_vec3,
    pub orientation: mln_quaternion,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_geojson_source_options {
    pub size: u32,
    pub fields: mln_geojson_source_option_fields,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub tolerance: f64,
    pub cluster_max_zoom: f64,
    pub cluster_properties: mln_buffer_view,
    pub buffer: u32,
    pub cluster_radius: u32,
    pub cluster_min_points: u32,
    pub line_metrics: bool,
    pub cluster: bool,
    pub synchronous_tiling: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_gpu_sync {
    pub size: u32,
    pub kind: mln_gpu_sync_kind,
    pub handle: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_http_header {
    pub name: mln_buffer_view,
    pub value: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_http_header_rule {
    pub kinds: mln_resource_kind_flags,
    pub flags: mln_url_match_flags,
    pub url: mln_buffer_view,
    pub headers: *const mln_http_header,
    pub header_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_http_header_transform {
    pub size: u32,
    pub callback: mln_http_header_transform_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
    pub rules: *const mln_http_header_rule,
    pub rule_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_http_header_transform_response {
    pub size: u32,
    pub context: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_image_content {
    pub left: f32,
    pub top: f32,
    pub right: f32,
    pub bottom: f32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_image_stretch {
    pub from: f32,
    pub to: f32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_lat_lng {
    pub latitude: f64,
    pub longitude: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_lat_lng_bounds {
    pub southwest: mln_lat_lng,
    pub northeast: mln_lat_lng,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_log_handler {
    pub size: u32,
    pub callback: mln_log_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
    pub queue: mln_delivery_queue,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_logical_extent {
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_map_options {
    pub size: u32,
    pub initial_extent: mln_logical_extent,
    pub map_mode: mln_map_mode,
    pub fast_pfor_enabled: bool,
    pub event_mask: mln_runtime_event_flags,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_map_snapshot {
    pub size: u32,
    pub debug_options: mln_map_debug_flags,
    pub generation: u64,
    pub fully_loaded: bool,
    pub rendering_stats_view_enabled: bool,
    pub gesture_in_progress: bool,
    pub transition_in_progress: bool,
    pub event_mask: mln_runtime_event_flags,
    pub latest_render_update_generation: u64,
    pub tile: mln_map_tile_options,
    pub bounds: mln_bound_options,
    pub free_camera: mln_free_camera_options,
    pub view: mln_map_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_map_tile_options {
    pub size: u32,
    pub fields: mln_map_tile_option_fields,
    pub prefetch_zoom_delta: u32,
    pub lod_min_radius: f64,
    pub lod_scale: f64,
    pub lod_pitch_threshold: f64,
    pub lod_zoom_shift: f64,
    pub lod_mode: mln_tile_lod_mode,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_map_view {
    pub size: u32,
    pub camera: mln_camera_options,
    pub extent: mln_logical_extent,
    pub projection_mode: mln_projection_mode,
    pub viewport: mln_map_viewport_options,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_map_viewport_options {
    pub size: u32,
    pub fields: mln_map_viewport_option_fields,
    pub north_orientation: mln_north_orientation,
    pub constrain_mode: mln_constrain_mode,
    pub viewport_mode: mln_viewport_mode,
    pub frustum_offset: mln_edge_insets,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_borrowed_texture {
    pub texture: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_borrowed_texture_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub textures: *const mln_metal_borrowed_texture,
    pub texture_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_context_descriptor {
    pub device: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_owned_texture_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub context: mln_metal_context_descriptor,
    pub pixel_format: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_surface_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub context: mln_metal_context_descriptor,
    pub layer: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_texture_frame {
    pub size: u32,
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub slot: u32,
    pub texture: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub pixel_format: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_offline_geometry_region_definition {
    pub style_url: mln_buffer_view,
    pub geometry: mln_buffer_view,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub pixel_ratio: f32,
    pub include_ideographs: bool,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_offline_region_definition {
    pub size: u32,
    pub type_: mln_offline_region_definition_type,
    pub data: mln_offline_region_definition_data,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub union mln_offline_region_definition_data {
    pub tile_pyramid: mln_offline_tile_pyramid_region_definition,
    pub geometry: mln_offline_geometry_region_definition,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_offline_region_info {
    pub id: mln_offline_region_id,
    pub definition: mln_offline_region_definition,
    pub metadata: *const u8,
    pub metadata_size: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_offline_region_status {
    pub download_state: mln_offline_region_download_state,
    pub completed_resource_count: u64,
    pub completed_resource_size: u64,
    pub completed_tile_count: u64,
    pub required_tile_count: u64,
    pub completed_tile_size: u64,
    pub required_resource_count: u64,
    pub required_resource_count_is_precise: bool,
    pub complete: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_offline_tile_pyramid_region_definition {
    pub style_url: mln_buffer_view,
    pub bounds: mln_lat_lng_bounds,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub pixel_ratio: f32,
    pub include_ideographs: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_opengl_borrowed_texture {
    pub texture: u32,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_opengl_borrowed_texture_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: mln_opengl_context_descriptor,
    pub textures: *const mln_opengl_borrowed_texture,
    pub texture_count: usize,
    pub target: u32,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_opengl_context_descriptor {
    pub platform: mln_opengl_context_platform,
    pub ownership: mln_opengl_context_ownership,
    pub data: mln_opengl_context_descriptor_data,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub union mln_opengl_context_descriptor_data {
    pub wgl: mln_wgl_context_descriptor,
    pub egl: mln_egl_context_descriptor,
    pub webgl: mln_webgl_context_descriptor,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_opengl_owned_texture_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub context: mln_opengl_context_descriptor,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_opengl_surface_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub context: mln_opengl_context_descriptor,
    pub surface: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_opengl_texture_frame {
    pub size: u32,
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub slot: u32,
    pub texture: u32,
    pub target: u32,
    pub internal_format: u32,
    pub format: u32,
    pub type_: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_premultiplied_rgba8_image {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub pixels: *const u8,
    pub byte_length: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_projected_meters {
    pub northing: f64,
    pub easting: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_projection_mode {
    pub size: u32,
    pub fields: mln_projection_mode_fields,
    pub axonometric: bool,
    pub x_skew: f64,
    pub y_skew: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_quaternion {
    pub x: f64,
    pub y: f64,
    pub z: f64,
    pub w: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_queue_lock {
    pub lock: mln_queue_lock_callback,
    pub unlock: mln_queue_lock_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_abandon_result {
    pub size: u32,
    pub disposition: mln_render_abandon_disposition,
    pub quarantined_resource_count: u32,
    pub reserved: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_frame_batch_view {
    pub size: u32,
    pub result_size: u32,
    pub results: *const mln_render_frame_result,
    pub result_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_frame_result {
    pub size: u32,
    pub disposition: mln_render_result,
    pub token: u64,
    pub map_update_generation: u64,
    pub generation: u64,
    pub frame_generation: u64,
    pub needs_repaint: bool,
    pub view: mln_map_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_session_attach_options {
    pub size: u32,
    pub driver: mln_render_driver_kind,
    pub requested_texture_ring_depth: u32,
    pub producer_sync_kind: mln_gpu_sync_kind,
    pub frame_wake: mln_wake,
    pub driver_work_wake: mln_wake,
    pub queue_lock: mln_queue_lock,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_session_capabilities {
    pub size: u32,
    pub driver: mln_render_driver_kind,
    pub texture_ring_depth: u32,
    pub flags: mln_render_session_capability_flags,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_session_snapshot {
    pub size: u32,
    pub state: mln_render_session_state,
    pub extent: mln_logical_extent,
    pub generation: u64,
    pub map_update_generation: u64,
    pub rendered_update_generation: u64,
    pub frame_generation: u64,
    pub acquired_frame_count: u32,
    pub pending_changes: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_rendered_feature {
    pub feature: mln_buffer_view,
    pub layer_id: mln_buffer_view,
    pub source_id: mln_buffer_view,
    pub source_layer_id: mln_buffer_view,
    pub state: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_rendered_feature_query_options {
    pub size: u32,
    pub fields: mln_rendered_feature_query_option_fields,
    pub layer_ids: *const mln_buffer_view,
    pub layer_id_count: usize,
    pub filter: mln_buffer_view,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_rendered_query_geometry {
    pub size: u32,
    pub type_: mln_rendered_query_geometry_type,
    pub data: mln_rendered_query_geometry_data,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub union mln_rendered_query_geometry_data {
    pub point: mln_screen_point,
    pub box_: mln_screen_box,
    pub line_string: mln_screen_line_string,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_rendering_stats {
    pub encoding_time: f64,
    pub rendering_time: f64,
    pub frame_count: i64,
    pub draw_call_count: i64,
    pub total_draw_call_count: i64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_provider {
    pub size: u32,
    pub callback: mln_resource_provider_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
    pub queue: mln_delivery_queue,
    pub routes: *const mln_resource_route,
    pub route_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_range {
    pub start: u64,
    pub end: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_request_cancel_handler {
    pub size: u32,
    pub callback: mln_resource_request_cancel_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
    pub queue: mln_delivery_queue,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_request_info {
    pub size: u32,
    pub fields: mln_resource_request_fields,
    pub requested_url: mln_buffer_view,
    pub resolved_url: mln_buffer_view,
    pub kind: mln_resource_kind,
    pub loading_method: mln_resource_loading_method,
    pub priority: mln_resource_priority,
    pub usage: mln_resource_usage,
    pub storage_policy: mln_resource_storage_policy,
    pub range: mln_resource_range,
    pub prior_modified_unix_ms: i64,
    pub prior_expires_unix_ms: i64,
    pub prior_etag: mln_buffer_view,
    pub prior_data: *const u8,
    pub prior_data_size: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_response {
    pub size: u32,
    pub fields: mln_resource_response_fields,
    pub status: mln_resource_response_status,
    pub error_reason: mln_resource_error_reason,
    pub bytes: *const u8,
    pub byte_count: usize,
    pub error_message: mln_buffer_view,
    pub must_revalidate: bool,
    pub modified_unix_ms: i64,
    pub expires_unix_ms: i64,
    pub etag: mln_buffer_view,
    pub retry_after_unix_ms: i64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_route {
    pub kinds: mln_resource_kind_flags,
    pub flags: mln_url_match_flags,
    pub url: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_transform {
    pub size: u32,
    pub callback: mln_resource_transform_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
    pub queue: mln_delivery_queue,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_runtime_event {
    pub type_: mln_runtime_event_type,
    pub source_type: mln_runtime_event_source_type,
    pub source: u64,
    pub generation: u64,
    pub code: u32,
    pub payload_type: mln_runtime_event_payload_type,
    pub message: mln_buffer_view,
    pub payload: mln_runtime_event_payload,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_handler {
    pub size: u32,
    pub callback: mln_runtime_event_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
    pub queue: mln_delivery_queue,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_offline_region_response_error {
    pub region_id: mln_offline_region_id,
    pub reason: mln_resource_error_reason,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_offline_region_status {
    pub region_id: mln_offline_region_id,
    pub status: mln_offline_region_status,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_offline_region_tile_count_limit {
    pub region_id: mln_offline_region_id,
    pub limit: u64,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub union mln_runtime_event_payload {
    pub render_frame: mln_runtime_event_render_frame,
    pub render_map: mln_runtime_event_render_map,
    pub tile_action: mln_runtime_event_tile_action,
    pub offline_region_status: mln_runtime_event_offline_region_status,
    pub offline_region_response_error: mln_runtime_event_offline_region_response_error,
    pub offline_region_tile_count_limit: mln_runtime_event_offline_region_tile_count_limit,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_render_frame {
    pub mode: mln_render_mode,
    pub needs_repaint: bool,
    pub placement_changed: bool,
    pub stats: mln_rendering_stats,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_render_map {
    pub mode: mln_render_mode,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_tile_action {
    pub operation: mln_tile_operation,
    pub tile_id: mln_tile_id,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_options {
    pub size: u32,
    pub asset_path: mln_buffer_view,
    pub cache_path: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_screen_box {
    pub min: mln_screen_point,
    pub max: mln_screen_point,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_screen_line_string {
    pub points: *const mln_screen_point,
    pub point_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_screen_point {
    pub x: f64,
    pub y: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_source_feature {
    pub feature: mln_buffer_view,
    pub source_layer_id: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_source_feature_query_options {
    pub size: u32,
    pub fields: mln_source_feature_query_option_fields,
    pub source_layer_ids: *const mln_buffer_view,
    pub source_layer_id_count: usize,
    pub filter: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_expression_info {
    pub size: u32,
    pub is_feature_constant: bool,
    pub is_zoom_constant: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_expression_inputs {
    pub size: u32,
    pub fields: mln_style_expression_input_fields,
    pub zoom: f64,
    pub color_ramp_parameter: f64,
    pub elevation: f64,
    pub feature: mln_buffer_view,
    pub feature_state: mln_buffer_view,
    pub global_state: mln_buffer_view,
    pub canonical_tile_id: mln_canonical_tile_id,
    pub available_images: *const mln_buffer_view,
    pub available_image_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_expression_options {
    pub size: u32,
    pub fields: mln_style_expression_option_fields,
    pub expected_type: mln_style_expression_type,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_expression_type {
    pub kind: mln_style_expression_type_kind,
    pub item_kind: mln_style_expression_type_kind,
    pub item_count: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_image_info {
    pub width: u32,
    pub height: u32,
    pub fields: mln_style_image_info_fields,
    pub pixels: mln_buffer_view,
    pub stretch_x: *const mln_image_stretch,
    pub stretch_x_count: usize,
    pub stretch_y: *const mln_image_stretch,
    pub stretch_y_count: usize,
    pub content: mln_image_content,
    pub text_fit_width: mln_style_image_text_fit,
    pub text_fit_height: mln_style_image_text_fit,
    pub pixel_ratio: f32,
    pub sdf: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_image_options {
    pub size: u32,
    pub fields: mln_style_image_option_fields,
    pub stretch_x: *const mln_image_stretch,
    pub stretch_x_count: usize,
    pub stretch_y: *const mln_image_stretch,
    pub stretch_y_count: usize,
    pub content: mln_image_content,
    pub text_fit_width: mln_style_image_text_fit,
    pub text_fit_height: mln_style_image_text_fit,
    pub pixel_ratio: f32,
    pub sdf: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_layer_info {
    pub id: mln_buffer_view,
    pub type_: mln_buffer_view,
    pub source_id: mln_buffer_view,
    pub source_layer: mln_buffer_view,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub visibility: mln_style_layer_visibility,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_source_info {
    pub id: mln_buffer_view,
    pub type_: mln_style_source_type,
    pub fields: mln_style_source_info_fields,
    pub is_volatile: bool,
    pub attribution: mln_buffer_view,
    pub url: mln_buffer_view,
    pub tilejson: mln_style_source_tile_info,
    pub bounds: mln_lat_lng_bounds,
    pub tile_size: u32,
    pub vector_encoding: mln_style_vector_tile_encoding,
    pub raster_encoding: mln_style_raster_dem_encoding,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_source_tile_info {
    pub tile_urls: *const mln_buffer_view,
    pub tile_url_count: usize,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub scheme: mln_style_tile_scheme,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_transition_options {
    pub size: u32,
    pub fields: mln_style_transition_option_fields,
    pub duration_ms: f64,
    pub delay_ms: f64,
    pub enable_placement_transitions: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_texture_image_info {
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_texture_readback_result {
    pub data: mln_buffer_view,
    pub info: mln_texture_image_info,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_tile_id {
    pub overscaled_z: u32,
    pub wrap: i32,
    pub canonical_z: u32,
    pub canonical_x: u32,
    pub canonical_y: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_unit_bezier {
    pub x1: f64,
    pub y1: f64,
    pub x2: f64,
    pub y2: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vulkan_borrowed_texture {
    pub image: mln_vulkan_non_dispatchable_handle,
    pub image_view: mln_vulkan_non_dispatchable_handle,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vulkan_borrowed_texture_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: mln_vulkan_context_descriptor,
    pub textures: *const mln_vulkan_borrowed_texture,
    pub texture_count: usize,
    pub format: u32,
    pub initial_layout: u32,
    pub final_layout: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vulkan_context_descriptor {
    pub instance: *mut std::ffi::c_void,
    pub physical_device: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub graphics_queue: *mut std::ffi::c_void,
    pub graphics_queue_family_index: u32,
    pub get_instance_proc_addr: *mut std::ffi::c_void,
    pub get_device_proc_addr: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vulkan_owned_texture_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub context: mln_vulkan_context_descriptor,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vulkan_surface_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub context: mln_vulkan_context_descriptor,
    pub surface: mln_vulkan_non_dispatchable_handle,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vulkan_texture_frame {
    pub size: u32,
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub slot: u32,
    pub image: mln_vulkan_non_dispatchable_handle,
    pub image_view: mln_vulkan_non_dispatchable_handle,
    pub device: *mut std::ffi::c_void,
    pub format: u32,
    pub layout: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_wake {
    pub callback: mln_wake_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_user_data_release,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgl_context_descriptor {
    pub kind: mln_webgl_context_kind,
    pub context: i32,
    pub canvas_selector: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_borrowed_texture {
    pub texture: *mut std::ffi::c_void,
    pub texture_view: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_borrowed_texture_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: mln_webgpu_context_descriptor,
    pub textures: *const mln_webgpu_borrowed_texture,
    pub texture_count: usize,
    pub format: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_context_descriptor {
    pub instance: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub queue: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_owned_texture_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub context: mln_webgpu_context_descriptor,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_surface_descriptor {
    pub size: u32,
    pub extent: mln_logical_extent,
    pub context: mln_webgpu_context_descriptor,
    pub surface: *mut std::ffi::c_void,
    pub format: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_texture_frame {
    pub size: u32,
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub slot: u32,
    pub texture: *mut std::ffi::c_void,
    pub texture_view: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub format: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_wgl_context_descriptor {
    pub device_context: *mut std::ffi::c_void,
    pub share_context: *mut std::ffi::c_void,
    pub get_proc_address: *mut std::ffi::c_void,
}
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_acquired_frame(pub u64);
pub type mln_animation_option_fields = u32;
pub type mln_bound_option_fields = u32;
pub type mln_camera_delta_fields = u32;
pub type mln_camera_fit_option_fields = u32;
pub type mln_camera_option_fields = u32;
pub type mln_camera_transition_end_callback = Option<
    unsafe extern "C" fn(user_data: *mut std::ffi::c_void, end: *const mln_camera_transition_end),
>;
pub type mln_completion_callback = Option<
    unsafe extern "C" fn(user_data: *mut std::ffi::c_void, result: *const mln_completion_result),
>;
pub type mln_custom_geometry_source_option_fields = u32;
pub type mln_custom_mvt_vector_source_option_fields = u32;
pub type mln_custom_source_cancel_tile_callback =
    Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void, tile_id: mln_canonical_tile_id)>;
pub type mln_custom_source_fetch_tile_callback =
    Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void, tile_id: mln_canonical_tile_id)>;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_delivery_queue(pub u64);
pub type mln_feature_state_selector_fields = u32;
pub type mln_frame_demand_flags = u32;
pub type mln_free_camera_option_fields = u32;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_geojson_source_data(pub u64);
pub type mln_geojson_source_option_fields = u32;
pub type mln_http_header_transform_callback = Option<
    unsafe extern "C" fn(
        user_data: *mut std::ffi::c_void,
        kind: mln_resource_kind,
        url: mln_buffer_view,
        out_response: *mut mln_http_header_transform_response,
    ) -> mln_status,
>;
pub type mln_log_callback = Option<
    unsafe extern "C" fn(
        user_data: *mut std::ffi::c_void,
        severity: mln_log_severity,
        event: mln_log_event,
        code: i64,
        message: mln_buffer_view,
    ) -> bool,
>;
pub type mln_log_severity_flags = u32;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_map(pub u64);
pub type mln_map_debug_flags = u32;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_map_projection(pub u64);
pub type mln_map_tile_option_fields = u32;
pub type mln_map_viewport_option_fields = u32;
pub type mln_offline_region_id = i64;
pub type mln_opengl_context_provider_flags = u32;
pub type mln_projection_mode_fields = u32;
pub type mln_queue_lock_callback = Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
pub type mln_render_backend_flags = u32;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_render_frame_batch(pub u64);
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_render_session(pub u64);
pub type mln_render_session_capability_flags = u32;
pub type mln_rendered_feature_query_option_fields = u32;
pub type mln_resource_kind_flags = u32;
pub type mln_resource_provider_callback = Option<
    unsafe extern "C" fn(
        user_data: *mut std::ffi::c_void,
        request: *const mln_resource_request_info,
        handle: mln_resource_request,
    ),
>;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_resource_request(pub u64);
pub type mln_resource_request_cancel_callback =
    Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void, handle: mln_resource_request)>;
pub type mln_resource_request_fields = u32;
pub type mln_resource_response_fields = u32;
pub type mln_resource_transform_callback = Option<
    unsafe extern "C" fn(
        user_data: *mut std::ffi::c_void,
        kind: mln_resource_kind,
        url: mln_buffer_view,
        request: mln_resource_transform_request,
    ),
>;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_resource_transform_request(pub u64);
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_runtime(pub u64);
pub type mln_runtime_event_callback = Option<
    unsafe extern "C" fn(user_data: *mut std::ffi::c_void, events: *const mln_event_batch_view),
>;
pub type mln_runtime_event_flags = u64;
pub type mln_source_feature_query_option_fields = u32;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_style_expression(pub u64);
pub type mln_style_expression_input_fields = u32;
pub type mln_style_expression_option_fields = u32;
pub type mln_style_image_info_fields = u32;
pub type mln_style_image_option_fields = u32;
pub type mln_style_source_info_fields = u32;
pub type mln_style_transition_option_fields = u32;
pub type mln_url_match_flags = u32;
pub type mln_user_data_release = Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
pub type mln_vulkan_non_dispatchable_handle = u64;
pub type mln_wake_callback = Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
native_handles!(
    mln_acquired_frame,
    mln_delivery_queue,
    mln_geojson_source_data,
    mln_map,
    mln_map_projection,
    mln_render_frame_batch,
    mln_render_session,
    mln_resource_request,
    mln_resource_transform_request,
    mln_runtime,
    mln_style_expression
);

unsafe extern "C" {
    pub fn mln_acquired_frame_discard(
        frame: mln_acquired_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_get_metal_texture(
        frame: mln_acquired_frame,
        out_frame: *mut mln_metal_texture_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_get_opengl_texture(
        frame: mln_acquired_frame,
        out_frame: *mut mln_opengl_texture_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_get_producer_sync(
        frame: mln_acquired_frame,
        out_sync: *mut mln_gpu_sync,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_get_result(
        frame: mln_acquired_frame,
        out_result: *mut mln_render_frame_result,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_get_vulkan_texture(
        frame: mln_acquired_frame,
        out_frame: *mut mln_vulkan_texture_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_get_webgpu_texture(
        frame: mln_acquired_frame,
        out_frame: *mut mln_webgpu_texture_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_release(
        frame: mln_acquired_frame,
        consumer_completion: *const mln_gpu_sync,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_view_begin(
        frame: mln_acquired_frame,
        out_scope: *mut *mut std::ffi::c_void,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_view_end(scope: *mut std::ffi::c_void);
    pub fn mln_android_init(
        jni_env: *mut std::ffi::c_void,
        context: *mut std::ffi::c_void,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_animation_options_default() -> mln_animation_options;
    pub fn mln_bound_options_default() -> mln_bound_options;
    pub fn mln_c_version() -> u32;
    pub fn mln_camera_delta_default() -> mln_camera_delta;
    pub fn mln_camera_fit_options_default() -> mln_camera_fit_options;
    pub fn mln_camera_options_default() -> mln_camera_options;
    pub fn mln_camera_update_default() -> mln_camera_update;
    pub fn mln_custom_geometry_source_options_default() -> mln_custom_geometry_source_options;
    pub fn mln_custom_mvt_vector_source_options_default() -> mln_custom_mvt_vector_source_options;
    pub fn mln_frame_demand_default() -> mln_frame_demand;
    pub fn mln_free_camera_options_default() -> mln_free_camera_options;
    pub fn mln_geojson_source_data_create(
        data: mln_buffer_view,
        options: *const mln_geojson_source_options,
        out_data: *mut mln_geojson_source_data,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_geojson_source_data_release(data: mln_geojson_source_data);
    pub fn mln_geojson_source_options_default() -> mln_geojson_source_options;
    pub fn mln_gpu_sync_default() -> mln_gpu_sync;
    pub fn mln_http_header_rule_default() -> mln_http_header_rule;
    pub fn mln_http_header_transform_response_set_header(
        response: *mut mln_http_header_transform_response,
        name: mln_buffer_view,
        value: mln_buffer_view,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_lat_lng_for_projected_meters(
        meters: mln_projected_meters,
        out_coordinate: *mut mln_lat_lng,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_log_clear_callback(out_diagnostic: *mut mln_diagnostic) -> mln_status;
    pub fn mln_log_set_async_severity_mask(
        mask: mln_log_severity_flags,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_log_set_callback(
        handler: *const mln_log_handler,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_logical_extent_default() -> mln_logical_extent;
    pub fn mln_logical_extent_physical_size(
        extent: mln_logical_extent,
        out_width: *mut u32,
        out_height: *mut u32,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_custom_geometry_source(
        map: mln_map,
        source_id: mln_buffer_view,
        options: *const mln_custom_geometry_source_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_custom_mvt_vector_source(
        map: mln_map,
        source_id: mln_buffer_view,
        options: *const mln_custom_mvt_vector_source_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_geojson_source_data(
        map: mln_map,
        source_id: mln_buffer_view,
        data: mln_geojson_source_data,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_geojson_source_url(
        map: mln_map,
        source_id: mln_buffer_view,
        url: mln_buffer_view,
        options: *const mln_geojson_source_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_image_source_image(
        map: mln_map,
        source_id: mln_buffer_view,
        coordinates: *const mln_lat_lng,
        coordinate_count: usize,
        image: *const mln_premultiplied_rgba8_image,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_image_source_url(
        map: mln_map,
        source_id: mln_buffer_view,
        coordinates: *const mln_lat_lng,
        coordinate_count: usize,
        url: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_style_layer_json(
        map: mln_map,
        layer_json: mln_buffer_view,
        before_layer_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_style_source_json(
        map: mln_map,
        source_id: mln_buffer_view,
        source_json: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_apply_camera_delta(
        map: mln_map,
        delta: *const mln_camera_delta,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_metal_borrowed_texture(
        map: mln_map,
        descriptor: *const mln_metal_borrowed_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_metal_owned_texture(
        map: mln_map,
        descriptor: *const mln_metal_owned_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_metal_surface(
        map: mln_map,
        descriptor: *const mln_metal_surface_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_opengl_borrowed_texture(
        map: mln_map,
        descriptor: *const mln_opengl_borrowed_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_opengl_owned_texture(
        map: mln_map,
        descriptor: *const mln_opengl_owned_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_opengl_surface(
        map: mln_map,
        descriptor: *const mln_opengl_surface_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_vulkan_borrowed_texture(
        map: mln_map,
        descriptor: *const mln_vulkan_borrowed_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_vulkan_owned_texture(
        map: mln_map,
        descriptor: *const mln_vulkan_owned_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_vulkan_surface(
        map: mln_map,
        descriptor: *const mln_vulkan_surface_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_webgpu_borrowed_texture(
        map: mln_map,
        descriptor: *const mln_webgpu_borrowed_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_webgpu_owned_texture(
        map: mln_map,
        descriptor: *const mln_webgpu_owned_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_attach_webgpu_surface(
        map: mln_map,
        descriptor: *const mln_webgpu_surface_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_begin_command_group(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_camera_for_geometry(
        map: mln_map,
        geometry: mln_buffer_view,
        fit_options: *const mln_camera_fit_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_camera_for_lat_lng_bounds(
        map: mln_map,
        bounds: mln_lat_lng_bounds,
        fit_options: *const mln_camera_fit_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_camera_for_lat_lngs(
        map: mln_map,
        coordinates: *const mln_lat_lng,
        coordinate_count: usize,
        fit_options: *const mln_camera_fit_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_cancel_camera_transition(
        map: mln_map,
        transition_id: u64,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_cancel_transitions(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_dispose(map: mln_map, out_diagnostic: *mut mln_diagnostic) -> mln_status;
    pub fn mln_map_dump_debug_logs(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_end_command_group(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_camera(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_feature_state(
        map: mln_map,
        selector: *const mln_feature_state_selector,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_global_state(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_image_source_coordinates(
        map: mln_map,
        source_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_loaded_style_json(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_snapshot(
        map: mln_map,
        out_snapshot: *mut mln_map_snapshot,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_image(
        map: mln_map,
        image_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_layer(
        map: mln_map,
        layer_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_layer_json(
        map: mln_map,
        layer_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_layer_property(
        map: mln_map,
        layer_id: mln_buffer_view,
        property_name: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_light_property(
        map: mln_map,
        property_name: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_source(
        map: mln_map,
        source_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_transition_options(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_url(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_invalidate_custom_geometry_source_region(
        map: mln_map,
        source_id: mln_buffer_view,
        bounds: mln_lat_lng_bounds,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_invalidate_custom_geometry_source_tile(
        map: mln_map,
        source_id: mln_buffer_view,
        tile_id: mln_canonical_tile_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_invalidate_custom_mvt_vector_source_tile(
        map: mln_map,
        source_id: mln_buffer_view,
        tile_id: mln_canonical_tile_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_lat_lng_bounds_for_camera(
        map: mln_map,
        camera: *const mln_camera_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_lat_lng_bounds_for_camera_unwrapped(
        map: mln_map,
        camera: *const mln_camera_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_list_style_layers(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_list_style_sources(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_move_style_layer(
        map: mln_map,
        layer_id: mln_buffer_view,
        before_layer_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_options_default() -> mln_map_options;
    pub fn mln_map_projection_create(
        view: *const mln_map_view,
        out_projection: *mut mln_map_projection,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_get_camera(
        projection: mln_map_projection,
        out_camera: *mut mln_camera_options,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_lat_lng_for_pixel(
        projection: mln_map_projection,
        point: mln_screen_point,
        out_coordinate: *mut mln_lat_lng,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_lat_lng_for_pixel_unwrapped(
        projection: mln_map_projection,
        point: mln_screen_point,
        out_coordinate: *mut mln_lat_lng,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_lat_lngs_for_pixels(
        projection: mln_map_projection,
        points: *const mln_screen_point,
        point_count: usize,
        out_coordinates: *mut mln_lat_lng,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_lat_lngs_for_pixels_unwrapped(
        projection: mln_map_projection,
        points: *const mln_screen_point,
        point_count: usize,
        out_coordinates: *mut mln_lat_lng,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_meters_per_pixel_at_latitude(
        projection: mln_map_projection,
        latitude: f64,
        out_meters_per_pixel: *mut f64,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_pixel_for_lat_lng(
        projection: mln_map_projection,
        coordinate: mln_lat_lng,
        out_point: *mut mln_screen_point,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_pixels_for_lat_lngs(
        projection: mln_map_projection,
        coordinates: *const mln_lat_lng,
        coordinate_count: usize,
        out_points: *mut mln_screen_point,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_release(projection: mln_map_projection);
    pub fn mln_map_projection_set_camera(
        projection: mln_map_projection,
        camera: *const mln_camera_options,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_release(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_remove_feature_state(
        map: mln_map,
        selector: *const mln_feature_state_selector,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_remove_style_image(
        map: mln_map,
        image_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_remove_style_layer(
        map: mln_map,
        layer_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_remove_style_source(
        map: mln_map,
        source_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_request_repaint(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_request_still_image(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_resize(
        map: mln_map,
        extent: mln_logical_extent,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_bounds(
        map: mln_map,
        options: *const mln_bound_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_custom_geometry_source_tile_data(
        map: mln_map,
        source_id: mln_buffer_view,
        tile_id: mln_canonical_tile_id,
        data: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_custom_geometry_source_tile_error(
        map: mln_map,
        source_id: mln_buffer_view,
        tile_id: mln_canonical_tile_id,
        message: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_custom_mvt_vector_source_tile_data(
        map: mln_map,
        source_id: mln_buffer_view,
        tile_id: mln_canonical_tile_id,
        data: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_custom_mvt_vector_source_tile_error(
        map: mln_map,
        source_id: mln_buffer_view,
        tile_id: mln_canonical_tile_id,
        message: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_debug_options(
        map: mln_map,
        options: mln_map_debug_flags,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_event_mask(
        map: mln_map,
        mask: mln_runtime_event_flags,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_feature_state(
        map: mln_map,
        selector: *const mln_feature_state_selector,
        state: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_free_camera_options(
        map: mln_map,
        options: *const mln_free_camera_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_geojson_source_data(
        map: mln_map,
        source_id: mln_buffer_view,
        data: mln_geojson_source_data,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_geojson_source_synchronous_tiling(
        map: mln_map,
        source_id: mln_buffer_view,
        enabled: bool,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_geojson_source_url(
        map: mln_map,
        source_id: mln_buffer_view,
        url: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_global_state_property(
        map: mln_map,
        property_name: mln_buffer_view,
        value: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_image_source_coordinates(
        map: mln_map,
        source_id: mln_buffer_view,
        coordinates: *const mln_lat_lng,
        coordinate_count: usize,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_image_source_image(
        map: mln_map,
        source_id: mln_buffer_view,
        image: *const mln_premultiplied_rgba8_image,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_image_source_url(
        map: mln_map,
        source_id: mln_buffer_view,
        url: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_projection_mode(
        map: mln_map,
        mode: *const mln_projection_mode,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_rendering_stats_view_enabled(
        map: mln_map,
        enabled: bool,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_style_image(
        map: mln_map,
        image_id: mln_buffer_view,
        image: *const mln_premultiplied_rgba8_image,
        options: *const mln_style_image_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_style_json(
        map: mln_map,
        json: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_style_layer_property(
        map: mln_map,
        layer_id: mln_buffer_view,
        property_name: mln_buffer_view,
        value: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_style_light_json(
        map: mln_map,
        light_json: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_style_light_property(
        map: mln_map,
        property_name: mln_buffer_view,
        value: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_style_source_volatile(
        map: mln_map,
        source_id: mln_buffer_view,
        is_volatile: bool,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_style_transition_options(
        map: mln_map,
        options: *const mln_style_transition_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_style_url(
        map: mln_map,
        url: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_tile_options(
        map: mln_map,
        options: *const mln_map_tile_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_viewport_options(
        map: mln_map,
        options: *const mln_map_viewport_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_tile_options_default() -> mln_map_tile_options;
    pub fn mln_map_update_camera(
        map: mln_map,
        update: *const mln_camera_update,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_viewport_options_default() -> mln_map_viewport_options;
    pub fn mln_metal_borrowed_texture_descriptor_default() -> mln_metal_borrowed_texture_descriptor;
    pub fn mln_metal_owned_texture_descriptor_default() -> mln_metal_owned_texture_descriptor;
    pub fn mln_metal_surface_descriptor_default() -> mln_metal_surface_descriptor;
    pub fn mln_network_get_status(
        out_status: *mut mln_network_status,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_network_set_status(
        status: mln_network_status,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_opengl_borrowed_texture_descriptor_default() -> mln_opengl_borrowed_texture_descriptor;
    pub fn mln_opengl_owned_texture_descriptor_default() -> mln_opengl_owned_texture_descriptor;
    pub fn mln_opengl_supported_context_provider_mask() -> mln_opengl_context_provider_flags;
    pub fn mln_opengl_surface_descriptor_default() -> mln_opengl_surface_descriptor;
    pub fn mln_plugin_get_register_function_v1() -> mln_plugin_register_function_v1;
    pub fn mln_premultiplied_rgba8_image_default() -> mln_premultiplied_rgba8_image;
    pub fn mln_projected_meters_for_lat_lng(
        coordinate: mln_lat_lng,
        out_meters: *mut mln_projected_meters,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_projection_mode_default() -> mln_projection_mode;
    pub fn mln_render_frame_batch_release(batch: mln_render_frame_batch);
    pub fn mln_render_session_abandon(
        session: mln_render_session,
        out_result: *mut mln_render_abandon_result,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_acquire_frame(
        session: mln_render_session,
        out_frame: *mut mln_acquired_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_attach_options_default() -> mln_render_session_attach_options;
    pub fn mln_render_session_barrier(
        session: mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_clear_data(
        session: mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_detach(
        session: mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_dispose(
        session: mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_drain_frame_results(
        session: mln_render_session,
        out_batch: *mut mln_render_frame_batch,
        out_view: *mut mln_render_frame_batch_view,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_dump_debug_logs(
        session: mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_get_capabilities(
        session: mln_render_session,
        out_capabilities: *mut mln_render_session_capabilities,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_get_snapshot(
        session: mln_render_session,
        out_snapshot: *mut mln_render_session_snapshot,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_query_feature_extensions(
        session: mln_render_session,
        source_id: mln_buffer_view,
        feature: mln_buffer_view,
        extension: mln_buffer_view,
        extension_field: mln_buffer_view,
        arguments: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_query_rendered_features(
        session: mln_render_session,
        geometry: *const mln_rendered_query_geometry,
        options: *const mln_rendered_feature_query_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_query_source_features(
        session: mln_render_session,
        source_id: mln_buffer_view,
        options: *const mln_source_feature_query_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_read_texture(
        session: mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_reduce_memory_use(
        session: mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_request_frame(
        session: mln_render_session,
        demand: *const mln_frame_demand,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_resize(
        session: mln_render_session,
        extent: mln_logical_extent,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_service_driver_work(
        session: mln_render_session,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_set_metal_borrowed_texture_target(
        session: mln_render_session,
        descriptor: *const mln_metal_borrowed_texture_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_set_metal_surface_target(
        session: mln_render_session,
        descriptor: *const mln_metal_surface_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_set_opengl_borrowed_texture_target(
        session: mln_render_session,
        descriptor: *const mln_opengl_borrowed_texture_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_set_opengl_surface_target(
        session: mln_render_session,
        descriptor: *const mln_opengl_surface_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_set_vulkan_borrowed_texture_target(
        session: mln_render_session,
        descriptor: *const mln_vulkan_borrowed_texture_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_set_vulkan_surface_target(
        session: mln_render_session,
        descriptor: *const mln_vulkan_surface_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_set_webgpu_borrowed_texture_target(
        session: mln_render_session,
        descriptor: *const mln_webgpu_borrowed_texture_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_set_webgpu_surface_target(
        session: mln_render_session,
        descriptor: *const mln_webgpu_surface_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_rendered_feature_query_options_default() -> mln_rendered_feature_query_options;
    pub fn mln_resource_request_complete(
        handle: mln_resource_request,
        response: *const mln_resource_response,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_resource_request_is_cancelled(
        handle: mln_resource_request,
        out_cancelled: *mut bool,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_resource_request_pass_through(
        handle: mln_resource_request,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_resource_request_release(handle: mln_resource_request);
    pub fn mln_resource_request_set_cancel_callback(
        handle: mln_resource_request,
        handler: *const mln_resource_request_cancel_handler,
        out_cancelled: *mut bool,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_resource_route_default() -> mln_resource_route;
    pub fn mln_resource_transform_request_complete(
        request: mln_resource_transform_request,
        url: mln_buffer_view,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_resource_transform_request_release(request: mln_resource_transform_request);
    pub fn mln_runtime_clear_event_handler(
        runtime: mln_runtime,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_clear_http_header_transform(
        runtime: mln_runtime,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_clear_resource_provider(
        runtime: mln_runtime,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_clear_resource_transform(
        runtime: mln_runtime,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_create(
        options: *const mln_runtime_options,
        out_runtime: *mut mln_runtime,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_create_map(
        runtime: mln_runtime,
        options: *const mln_map_options,
        out_map: *mut mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_create_offline_region(
        runtime: mln_runtime,
        definition: *const mln_offline_region_definition,
        metadata: *const u8,
        metadata_size: usize,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_delete_offline_region(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_dispose(
        runtime: mln_runtime,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_get_event_mask(
        runtime: mln_runtime,
        out_mask: *mut mln_runtime_event_flags,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_get_offline_region(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_get_offline_region_status(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_invalidate_offline_region(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_list_offline_regions(
        runtime: mln_runtime,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_merge_offline_regions(
        runtime: mln_runtime,
        side_database_path: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_options_default() -> mln_runtime_options;
    pub fn mln_runtime_release(
        runtime: mln_runtime,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_run_ambient_cache_operation(
        runtime: mln_runtime,
        operation: mln_ambient_cache_operation,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_set_event_handler(
        runtime: mln_runtime,
        handler: *const mln_runtime_event_handler,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_set_event_mask(
        runtime: mln_runtime,
        mask: mln_runtime_event_flags,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_set_http_header_transform(
        runtime: mln_runtime,
        transform: *const mln_http_header_transform,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_set_maximum_ambient_cache_size(
        runtime: mln_runtime,
        size: u64,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_set_offline_region_download_state(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        state: mln_offline_region_download_state,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_set_offline_region_observed(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        observed: bool,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_set_resource_provider(
        runtime: mln_runtime,
        provider: *const mln_resource_provider,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_set_resource_transform(
        runtime: mln_runtime,
        transform: *const mln_resource_transform,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_update_offline_region_metadata(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        metadata: *const u8,
        metadata_size: usize,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_source_feature_query_options_default() -> mln_source_feature_query_options;
    pub fn mln_style_expression_create(
        expression: mln_buffer_view,
        options: *const mln_style_expression_options,
        out_expression: *mut mln_style_expression,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_style_expression_evaluate(
        expression: mln_style_expression,
        inputs: *const mln_style_expression_inputs,
        out_json: *mut std::ffi::c_char,
        json_capacity: usize,
        out_json_length: *mut usize,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_style_expression_get_info(
        expression: mln_style_expression,
        out_info: *mut mln_style_expression_info,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_style_expression_inputs_default() -> mln_style_expression_inputs;
    pub fn mln_style_expression_options_default() -> mln_style_expression_options;
    pub fn mln_style_expression_release(expression: mln_style_expression);
    pub fn mln_style_image_options_default() -> mln_style_image_options;
    pub fn mln_style_transition_options_default() -> mln_style_transition_options;
    pub fn mln_supported_render_backend_mask() -> mln_render_backend_flags;
    pub fn mln_vulkan_borrowed_texture_descriptor_default() -> mln_vulkan_borrowed_texture_descriptor;
    pub fn mln_vulkan_owned_texture_descriptor_default() -> mln_vulkan_owned_texture_descriptor;
    pub fn mln_vulkan_surface_descriptor_default() -> mln_vulkan_surface_descriptor;
    pub fn mln_webgpu_borrowed_texture_descriptor_default() -> mln_webgpu_borrowed_texture_descriptor;
    pub fn mln_webgpu_owned_texture_descriptor_default() -> mln_webgpu_owned_texture_descriptor;
    pub fn mln_webgpu_surface_descriptor_default() -> mln_webgpu_surface_descriptor;
}
