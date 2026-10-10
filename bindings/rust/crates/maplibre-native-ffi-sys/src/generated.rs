// Generated from C headers by tools/bindgen. Do not edit.
use super::*;

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
pub const MLN_CAMERA_DELTA_FIELD_ANCHOR: mln_camera_delta_field = 1;
pub type mln_camera_delta_kind = u32;
pub const MLN_CAMERA_DELTA_MOVE: mln_camera_delta_kind = 0;
pub const MLN_CAMERA_DELTA_SCALE: mln_camera_delta_kind = 1;
pub const MLN_CAMERA_DELTA_BEARING: mln_camera_delta_kind = 2;
pub const MLN_CAMERA_DELTA_PITCH: mln_camera_delta_kind = 3;
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
pub type mln_camera_update_mode = u32;
pub const MLN_CAMERA_UPDATE_MODE_JUMP: mln_camera_update_mode = 0;
pub const MLN_CAMERA_UPDATE_MODE_EASE: mln_camera_update_mode = 1;
pub const MLN_CAMERA_UPDATE_MODE_FLY: mln_camera_update_mode = 2;
pub type mln_command_disposition = u32;
pub const MLN_COMMAND_DISPOSITION_COMMITTED: mln_command_disposition = 0;
pub const MLN_COMMAND_DISPOSITION_SUPERSEDED: mln_command_disposition = 1;
pub const MLN_COMMAND_DISPOSITION_FAILED: mln_command_disposition = 2;
pub const MLN_COMMAND_DISPOSITION_CANCELLED: mln_command_disposition = 3;
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
pub const MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID: mln_feature_state_selector_field = 1;
pub const MLN_FEATURE_STATE_SELECTOR_FEATURE_ID: mln_feature_state_selector_field = 2;
pub const MLN_FEATURE_STATE_SELECTOR_STATE_KEY: mln_feature_state_selector_field = 4;
pub type mln_frame_demand_flag = u32;
pub const MLN_FRAME_DEMAND_IF_NEEDED: mln_frame_demand_flag = 1;
pub const MLN_FRAME_DEMAND_PRESENT: mln_frame_demand_flag = 2;
pub type mln_free_camera_option_field = u32;
pub const MLN_FREE_CAMERA_OPTION_POSITION: mln_free_camera_option_field = 1;
pub const MLN_FREE_CAMERA_OPTION_ORIENTATION: mln_free_camera_option_field = 2;
pub type mln_geojson_source_option_field = u32;
pub const MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM: mln_geojson_source_option_field = 1;
pub const MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM: mln_geojson_source_option_field = 2;
pub const MLN_GEOJSON_SOURCE_OPTION_TOLERANCE: mln_geojson_source_option_field = 4;
pub const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM: mln_geojson_source_option_field = 8;
pub const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES: mln_geojson_source_option_field = 16;
pub const MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE: mln_geojson_source_option_field = 32;
pub const MLN_GEOJSON_SOURCE_OPTION_BUFFER: mln_geojson_source_option_field = 64;
pub const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS: mln_geojson_source_option_field = 128;
pub const MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS: mln_geojson_source_option_field = 256;
pub const MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS: mln_geojson_source_option_field = 512;
pub const MLN_GEOJSON_SOURCE_OPTION_CLUSTER: mln_geojson_source_option_field = 1024;
pub const MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING: mln_geojson_source_option_field = 2048;
pub type mln_gesture_phase = u32;
pub const MLN_GESTURE_PHASE_NONE: mln_gesture_phase = 0;
pub const MLN_GESTURE_PHASE_BEGIN: mln_gesture_phase = 1;
pub const MLN_GESTURE_PHASE_UPDATE: mln_gesture_phase = 2;
pub const MLN_GESTURE_PHASE_END: mln_gesture_phase = 3;
pub const MLN_GESTURE_PHASE_CANCEL: mln_gesture_phase = 4;
pub type mln_gpu_sync_kind = u32;
pub const MLN_GPU_SYNC_CPU_COMPLETE: mln_gpu_sync_kind = 0;
pub const MLN_GPU_SYNC_METAL_SHARED_EVENT: mln_gpu_sync_kind = 1;
pub const MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE: mln_gpu_sync_kind = 2;
pub const MLN_GPU_SYNC_OPENGL_FENCE: mln_gpu_sync_kind = 3;
pub const MLN_GPU_SYNC_WEBGPU_TOKEN: mln_gpu_sync_kind = 4;
pub type mln_location_indicator_image_kind = u32;
pub const MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP: mln_location_indicator_image_kind = 0;
pub const MLN_LOCATION_INDICATOR_IMAGE_KIND_BEARING: mln_location_indicator_image_kind = 1;
pub const MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW: mln_location_indicator_image_kind = 2;
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
pub type mln_log_severity_mask = u32;
pub const MLN_LOG_SEVERITY_MASK_INFO: mln_log_severity_mask = 2;
pub const MLN_LOG_SEVERITY_MASK_WARNING: mln_log_severity_mask = 4;
pub const MLN_LOG_SEVERITY_MASK_ERROR: mln_log_severity_mask = 8;
pub const MLN_LOG_SEVERITY_MASK_DEFAULT: mln_log_severity_mask = 6;
pub const MLN_LOG_SEVERITY_MASK_ALL: mln_log_severity_mask = 14;
pub type mln_map_debug_option = u32;
pub const MLN_MAP_DEBUG_TILE_BORDERS: mln_map_debug_option = 2;
pub const MLN_MAP_DEBUG_PARSE_STATUS: mln_map_debug_option = 4;
pub const MLN_MAP_DEBUG_TIMESTAMPS: mln_map_debug_option = 8;
pub const MLN_MAP_DEBUG_COLLISION: mln_map_debug_option = 16;
pub const MLN_MAP_DEBUG_OVERDRAW: mln_map_debug_option = 32;
pub const MLN_MAP_DEBUG_STENCIL_CLIP: mln_map_debug_option = 64;
pub const MLN_MAP_DEBUG_DEPTH_BUFFER: mln_map_debug_option = 128;
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
pub type mln_queried_feature_field = u32;
pub const MLN_QUERIED_FEATURE_SOURCE_ID: mln_queried_feature_field = 1;
pub const MLN_QUERIED_FEATURE_SOURCE_LAYER_ID: mln_queried_feature_field = 2;
pub const MLN_QUERIED_FEATURE_STATE: mln_queried_feature_field = 4;
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
pub const MLN_RENDER_RESULT_SIZE_PENDING: mln_render_result = 2;
pub const MLN_RENDER_RESULT_TARGET_NOT_READY: mln_render_result = 3;
pub const MLN_RENDER_RESULT_SUPERSEDED: mln_render_result = 4;
pub const MLN_RENDER_RESULT_DEADLINE_MISSED: mln_render_result = 5;
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
pub const MLN_RENDERED_FEATURE_QUERY_OPTION_FILTER: mln_rendered_feature_query_option_field = 2;
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
pub type mln_resource_loading_method = u32;
pub const MLN_RESOURCE_LOADING_METHOD_ALL: mln_resource_loading_method = 0;
pub const MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY: mln_resource_loading_method = 1;
pub const MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY: mln_resource_loading_method = 2;
pub type mln_resource_priority = u32;
pub const MLN_RESOURCE_PRIORITY_REGULAR: mln_resource_priority = 0;
pub const MLN_RESOURCE_PRIORITY_LOW: mln_resource_priority = 1;
pub type mln_resource_provider_decision = u32;
pub const MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH: mln_resource_provider_decision = 0;
pub const MLN_RESOURCE_PROVIDER_DECISION_HANDLE: mln_resource_provider_decision = 1;
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
pub type mln_runtime_event_mask = u64;
pub const MLN_RUNTIME_EVENT_MASK_NONE: mln_runtime_event_mask = 0;
pub const MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE: mln_runtime_event_mask = 2;
pub const MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING: mln_runtime_event_mask = 4;
pub const MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE: mln_runtime_event_mask = 8;
pub const MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED: mln_runtime_event_mask = 16;
pub const MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED: mln_runtime_event_mask = 32;
pub const MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED: mln_runtime_event_mask = 64;
pub const MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED: mln_runtime_event_mask = 128;
pub const MLN_RUNTIME_EVENT_MASK_MAP_IDLE: mln_runtime_event_mask = 256;
pub const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE: mln_runtime_event_mask = 512;
pub const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR: mln_runtime_event_mask = 1024;
pub const MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED: mln_runtime_event_mask = 2048;
pub const MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED: mln_runtime_event_mask = 4096;
pub const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED: mln_runtime_event_mask = 8192;
pub const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED: mln_runtime_event_mask = 16384;
pub const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED: mln_runtime_event_mask = 32768;
pub const MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED: mln_runtime_event_mask = 65536;
pub const MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING: mln_runtime_event_mask = 131072;
pub const MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION: mln_runtime_event_mask = 262144;
pub const MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED: mln_runtime_event_mask = 4194304;
pub const MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED: mln_runtime_event_mask = 524288;
pub const MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR: mln_runtime_event_mask = 1048576;
pub const MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: mln_runtime_event_mask =
    2097152;
pub const MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS: mln_runtime_event_mask = 4718590;
pub const MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS: mln_runtime_event_mask = 3670016;
pub const MLN_RUNTIME_EVENT_MASK_ALL: mln_runtime_event_mask = 8388606;
pub type mln_runtime_event_payload_type = u32;
pub const MLN_RUNTIME_EVENT_PAYLOAD_NONE: mln_runtime_event_payload_type = 0;
pub const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME: mln_runtime_event_payload_type = 1;
pub const MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP: mln_runtime_event_payload_type = 2;
pub const MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION: mln_runtime_event_payload_type = 4;
pub const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS: mln_runtime_event_payload_type = 5;
pub const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR: mln_runtime_event_payload_type =
    6;
pub const MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT:
    mln_runtime_event_payload_type = 7;
pub const MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED: mln_runtime_event_payload_type = 9;
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
pub const MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE: mln_runtime_event_type = 9;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_ERROR: mln_runtime_event_type = 10;
pub const MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED: mln_runtime_event_type = 11;
pub const MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED: mln_runtime_event_type = 12;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED: mln_runtime_event_type = 13;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED: mln_runtime_event_type = 14;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED: mln_runtime_event_type = 15;
pub const MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED: mln_runtime_event_type = 16;
pub const MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING: mln_runtime_event_type = 17;
pub const MLN_RUNTIME_EVENT_MAP_TILE_ACTION: mln_runtime_event_type = 18;
pub const MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED: mln_runtime_event_type = 19;
pub const MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR: mln_runtime_event_type = 20;
pub const MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: mln_runtime_event_type = 21;
pub const MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED: mln_runtime_event_type = 22;
pub type mln_source_feature_query_option_field = u32;
pub const MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS: mln_source_feature_query_option_field =
    1;
pub const MLN_SOURCE_FEATURE_QUERY_OPTION_FILTER: mln_source_feature_query_option_field = 2;
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
pub const MLN_STYLE_SOURCE_INFO_URL: mln_style_source_info_field = 1;
pub const MLN_STYLE_SOURCE_INFO_TILEJSON: mln_style_source_info_field = 2;
pub const MLN_STYLE_SOURCE_INFO_BOUNDS: mln_style_source_info_field = 4;
pub const MLN_STYLE_SOURCE_INFO_TILE_SIZE: mln_style_source_info_field = 8;
pub const MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING: mln_style_source_info_field = 16;
pub const MLN_STYLE_SOURCE_INFO_RASTER_ENCODING: mln_style_source_info_field = 32;
pub const MLN_STYLE_SOURCE_INFO_ATTRIBUTION: mln_style_source_info_field = 64;
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
pub type mln_style_tile_source_option_field = u32;
pub const MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM: mln_style_tile_source_option_field = 1;
pub const MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM: mln_style_tile_source_option_field = 2;
pub const MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION: mln_style_tile_source_option_field = 4;
pub const MLN_STYLE_TILE_SOURCE_OPTION_SCHEME: mln_style_tile_source_option_field = 8;
pub const MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS: mln_style_tile_source_option_field = 16;
pub const MLN_STYLE_TILE_SOURCE_OPTION_TILE_SIZE: mln_style_tile_source_option_field = 32;
pub const MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING: mln_style_tile_source_option_field = 64;
pub const MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING: mln_style_tile_source_option_field = 128;
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
pub type mln_viewport_mode = u32;
pub const MLN_VIEWPORT_MODE_DEFAULT: mln_viewport_mode = 0;
pub const MLN_VIEWPORT_MODE_FLIPPED_Y: mln_viewport_mode = 1;
pub type mln_webgl_context_kind = u32;
pub const MLN_WEBGL_CONTEXT_EXISTING: mln_webgl_context_kind = 0;
pub const MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS: mln_webgl_context_kind = 1;
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_animation_options {
    pub size: u32,
    pub fields: u32,
    pub duration_ms: f64,
    pub velocity: f64,
    pub min_zoom: f64,
    pub easing: mln_unit_bezier,
    pub transition_id: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_bound_options {
    pub size: u32,
    pub fields: u32,
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
    pub fields: u32,
    pub kind: u32,
    pub offset: mln_screen_point,
    pub amount: f64,
    pub anchor: mln_screen_point,
    pub animation: mln_animation_options,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_fit_options {
    pub size: u32,
    pub fields: u32,
    pub padding: mln_edge_insets,
    pub bearing: f64,
    pub pitch: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_options {
    pub size: u32,
    pub fields: u32,
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
    pub size: u32,
    pub reserved: u32,
    pub generation: u64,
    pub camera: mln_camera_options,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_camera_update {
    pub size: u32,
    pub mode: u32,
    pub camera: mln_camera_options,
    pub animation: mln_animation_options,
    pub gesture_phase: u32,
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
    pub release_user_data: mln_completion_release,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_completion_result {
    pub size: u32,
    pub status: i32,
    pub disposition: u32,
    pub reserved: u32,
    pub generation: u64,
    pub diagnostic: mln_buffer_view,
    pub value: *const std::ffi::c_void,
    pub value_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_custom_geometry_source_options {
    pub size: u32,
    pub fields: u32,
    pub fetch_tile: mln_custom_geometry_source_tile_callback,
    pub cancel_tile: mln_custom_geometry_source_tile_callback,
    pub user_data: *mut std::ffi::c_void,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub tolerance: f64,
    pub tile_size: u32,
    pub buffer: u32,
    pub clip: bool,
    pub wrap: bool,
    pub release_user_data: mln_custom_geometry_source_release_callback,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_custom_mvt_vector_source_options {
    pub size: u32,
    pub fields: u32,
    pub fetch_tile: mln_custom_mvt_vector_source_tile_callback,
    pub cancel_tile: mln_custom_mvt_vector_source_tile_callback,
    pub user_data: *mut std::ffi::c_void,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub release_user_data: mln_custom_mvt_vector_source_release_callback,
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
    pub size: u32,
    pub display: *mut std::ffi::c_void,
    pub config: *mut std::ffi::c_void,
    pub share_context: *mut std::ffi::c_void,
    pub client_api: u32,
    pub get_proc_address: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_event_batch_view {
    pub size: u32,
    pub event_size: u32,
    pub events: *const mln_runtime_event,
    pub event_count: usize,
    pub messages: *const std::ffi::c_char,
    pub messages_size: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_feature_state_selector {
    pub size: u32,
    pub fields: u32,
    pub source_id: mln_buffer_view,
    pub source_layer_id: mln_buffer_view,
    pub feature_id: mln_buffer_view,
    pub state_key: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_frame_demand {
    pub size: u32,
    pub flags: u32,
    pub token: u64,
    pub coalescing_boundary: u64,
    pub timeout_ns: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_free_camera_options {
    pub size: u32,
    pub fields: u32,
    pub position: mln_vec3,
    pub orientation: mln_quaternion,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_geojson_source_options {
    pub size: u32,
    pub fields: u32,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub tolerance: f64,
    pub cluster_max_zoom: f64,
    pub cluster_properties: mln_buffer_view,
    pub tile_size: u32,
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
    pub kind: u32,
    pub object: u64,
    pub value: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_http_header_transform {
    pub size: u32,
    pub callback: mln_http_header_transform_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_runtime_callback_release,
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
    pub map_mode: u32,
    pub fast_pfor_enabled: bool,
    pub event_mask: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_map_snapshot {
    pub size: u32,
    pub debug_options: u32,
    pub generation: u64,
    pub camera: mln_camera_options,
    pub logical_extent: mln_logical_extent,
    pub projection_mode: mln_projection_mode,
    pub viewport: mln_map_viewport_options,
    pub fully_loaded: bool,
    pub rendering_stats_view_enabled: bool,
    pub repaint_demand: bool,
    pub gesture_in_progress: bool,
    pub event_mask: u64,
    pub latest_render_update_generation: u64,
    pub tile: mln_map_tile_options,
    pub bounds: mln_bound_options,
    pub free_camera: mln_free_camera_options,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_map_tile_options {
    pub size: u32,
    pub fields: u32,
    pub prefetch_zoom_delta: u32,
    pub lod_min_radius: f64,
    pub lod_scale: f64,
    pub lod_pitch_threshold: f64,
    pub lod_zoom_shift: f64,
    pub lod_mode: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_map_viewport_options {
    pub size: u32,
    pub fields: u32,
    pub north_orientation: u32,
    pub constrain_mode: u32,
    pub viewport_mode: u32,
    pub frustum_offset: mln_edge_insets,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_borrowed_texture_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub texture: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_context_descriptor {
    pub size: u32,
    pub device: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_owned_texture_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub context: mln_metal_context_descriptor,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_owned_texture_frame {
    pub size: u32,
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub texture: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub pixel_format: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_metal_surface_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub context: mln_metal_context_descriptor,
    pub layer: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_offline_geometry_region_definition {
    pub size: u32,
    pub style_url: *const std::ffi::c_char,
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
    pub type_: u32,
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
    pub size: u32,
    pub id: mln_offline_region_id,
    pub definition: mln_offline_region_definition,
    pub metadata: *const u8,
    pub metadata_size: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_offline_region_status {
    pub size: u32,
    pub download_state: u32,
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
    pub size: u32,
    pub style_url: *const std::ffi::c_char,
    pub bounds: mln_lat_lng_bounds,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub pixel_ratio: f32,
    pub include_ideographs: bool,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_opengl_borrowed_texture_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: mln_opengl_context_descriptor,
    pub texture: u32,
    pub target: u32,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_opengl_context_descriptor {
    pub size: u32,
    pub platform: u32,
    pub ownership: u32,
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
    pub extent: mln_render_target_extent,
    pub context: mln_opengl_context_descriptor,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_opengl_owned_texture_frame {
    pub size: u32,
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub texture: u32,
    pub target: u32,
    pub internal_format: u32,
    pub format: u32,
    pub type_: u32,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_opengl_surface_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub context: mln_opengl_context_descriptor,
    pub surface: *mut std::ffi::c_void,
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
    pub fields: u32,
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
pub struct mln_queried_feature {
    pub size: u32,
    pub fields: u32,
    pub feature: mln_buffer_view,
    pub source_id: mln_buffer_view,
    pub source_layer_id: mln_buffer_view,
    pub state: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_queue_lock {
    pub size: u32,
    pub lock: mln_queue_lock_callback,
    pub unlock: mln_queue_lock_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_queue_lock_release,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_abandon_result {
    pub size: u32,
    pub disposition: u32,
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
    pub disposition: u32,
    pub token: u64,
    pub map_update_generation: u64,
    pub extent_generation: u64,
    pub frame_generation: u64,
    pub needs_repaint: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_session_attach_options {
    pub size: u32,
    pub driver: u32,
    pub requested_texture_ring_depth: u32,
    pub reserved: u32,
    pub frame_wake: mln_wake,
    pub driver_work_wake: mln_wake,
    pub queue_lock: mln_queue_lock,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_session_capabilities {
    pub size: u32,
    pub driver: u32,
    pub texture_ring_depth: u32,
    pub flags: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_session_snapshot {
    pub size: u32,
    pub state: u32,
    pub driver: u32,
    pub latest_result: u32,
    pub extent: mln_render_target_extent,
    pub generation: u64,
    pub map_update_generation: u64,
    pub rendered_update_generation: u64,
    pub extent_generation: u64,
    pub frame_generation: u64,
    pub latest_demand_token: u64,
    pub pending_demand_count: u32,
    pub acquired_frame_count: u32,
    pub target_ready: bool,
    pub pending_changes: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_render_target_extent {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_rendered_feature_query_options {
    pub size: u32,
    pub fields: u32,
    pub layer_ids: *const mln_buffer_view,
    pub layer_id_count: usize,
    pub filter: mln_buffer_view,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_rendered_query_geometry {
    pub size: u32,
    pub type_: u32,
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
    pub release_user_data: mln_runtime_callback_release,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_range {
    pub start: u64,
    pub end: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_request {
    pub size: u32,
    pub fields: u32,
    pub requested_url: *const std::ffi::c_char,
    pub resolved_url: *const std::ffi::c_char,
    pub kind: u32,
    pub loading_method: u32,
    pub priority: u32,
    pub usage: u32,
    pub storage_policy: u32,
    pub range: mln_resource_range,
    pub prior_modified_unix_ms: i64,
    pub prior_expires_unix_ms: i64,
    pub prior_etag: *const std::ffi::c_char,
    pub prior_data: *const u8,
    pub prior_data_size: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_response {
    pub size: u32,
    pub fields: u32,
    pub status: u32,
    pub error_reason: u32,
    pub bytes: *const u8,
    pub byte_count: usize,
    pub error_message: *const std::ffi::c_char,
    pub must_revalidate: bool,
    pub modified_unix_ms: i64,
    pub expires_unix_ms: i64,
    pub etag: *const std::ffi::c_char,
    pub retry_after_unix_ms: i64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_transform {
    pub size: u32,
    pub callback: mln_resource_transform_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_runtime_callback_release,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_resource_transform_response {
    pub size: u32,
    pub url: *const std::ffi::c_char,
    pub context: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Clone, Copy)]
pub struct mln_runtime_event {
    pub type_: u32,
    pub source_type: u32,
    pub source: u64,
    pub code: i32,
    pub payload_type: u32,
    pub message_offset: u64,
    pub message_size: u32,
    pub payload: mln_runtime_event_payload,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_camera_transition_finished {
    pub transition_id: u64,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_offline_region_response_error {
    pub region_id: mln_offline_region_id,
    pub reason: u32,
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
    pub camera_transition_finished: mln_runtime_event_camera_transition_finished,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_render_frame {
    pub mode: u32,
    pub needs_repaint: bool,
    pub placement_changed: bool,
    pub stats: mln_rendering_stats,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_render_map {
    pub mode: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_event_tile_action {
    pub operation: u32,
    pub tile_id: mln_tile_id,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_runtime_options {
    pub size: u32,
    pub flags: u32,
    pub asset_path: *const std::ffi::c_char,
    pub cache_path: *const std::ffi::c_char,
    pub event_mask: u64,
    pub event_wake: mln_wake,
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
pub struct mln_source_feature_query_options {
    pub size: u32,
    pub fields: u32,
    pub source_layer_ids: *const mln_buffer_view,
    pub source_layer_id_count: usize,
    pub filter: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_image_info {
    pub size: u32,
    pub fields: u32,
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
    pub stretch_x_count: usize,
    pub stretch_y_count: usize,
    pub content: mln_image_content,
    pub text_fit_width: u32,
    pub text_fit_height: u32,
    pub pixel_ratio: f32,
    pub sdf: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_image_options {
    pub size: u32,
    pub fields: u32,
    pub stretch_x: *const mln_image_stretch,
    pub stretch_x_count: usize,
    pub stretch_y: *const mln_image_stretch,
    pub stretch_y_count: usize,
    pub content: mln_image_content,
    pub text_fit_width: u32,
    pub text_fit_height: u32,
    pub pixel_ratio: f32,
    pub sdf: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_image_result {
    pub size: u32,
    pub reserved: u32,
    pub info: mln_style_image_info,
    pub pixels: mln_buffer_view,
    pub stretch_x: *const mln_image_stretch,
    pub stretch_x_count: usize,
    pub stretch_y: *const mln_image_stretch,
    pub stretch_y_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_image_stretches_result {
    pub size: u32,
    pub reserved: u32,
    pub stretch_x: *const mln_image_stretch,
    pub stretch_x_count: usize,
    pub stretch_y: *const mln_image_stretch,
    pub stretch_y_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_layer_entry {
    pub size: u32,
    pub id: mln_buffer_view,
    pub type_: mln_buffer_view,
    pub source_id: mln_buffer_view,
    pub source_layer: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_layer_info {
    pub size: u32,
    pub reserved: u32,
    pub type_: mln_buffer_view,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub visibility: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_layer_result {
    pub size: u32,
    pub reserved: u32,
    pub info: mln_style_layer_info,
    pub source_id: mln_buffer_view,
    pub source_layer: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_source_info {
    pub size: u32,
    pub type_: u32,
    pub fields: u32,
    pub id_size: usize,
    pub is_volatile: bool,
    pub attribution_size: usize,
    pub url_size: usize,
    pub tilejson: mln_style_source_tile_info,
    pub bounds: mln_lat_lng_bounds,
    pub tile_size: u32,
    pub vector_encoding: u32,
    pub raster_encoding: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_source_result {
    pub size: u32,
    pub reserved: u32,
    pub info: mln_style_source_info,
    pub attribution: mln_buffer_view,
    pub url: mln_buffer_view,
    pub tile_urls: *const mln_buffer_view,
    pub tile_url_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_source_tile_info {
    pub tile_count: usize,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub scheme: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_source_tile_urls_result {
    pub size: u32,
    pub reserved: u32,
    pub tile_urls: *const mln_buffer_view,
    pub tile_url_count: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_tile_source_options {
    pub size: u32,
    pub fields: u32,
    pub min_zoom: f64,
    pub max_zoom: f64,
    pub attribution: mln_buffer_view,
    pub scheme: u32,
    pub bounds: mln_lat_lng_bounds,
    pub tile_size: u32,
    pub vector_encoding: u32,
    pub raster_encoding: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_style_transition_options {
    pub size: u32,
    pub fields: u32,
    pub duration_ms: f64,
    pub delay_ms: f64,
    pub enable_placement_transitions: bool,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_texture_image_info {
    pub size: u32,
    pub width: u32,
    pub height: u32,
    pub stride: u32,
    pub byte_length: usize,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_texture_readback_result {
    pub size: u32,
    pub reserved: u32,
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
pub struct mln_vulkan_borrowed_texture_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: mln_vulkan_context_descriptor,
    pub image: mln_vulkan_non_dispatchable_handle,
    pub image_view: mln_vulkan_non_dispatchable_handle,
    pub format: u32,
    pub initial_layout: u32,
    pub final_layout: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vulkan_context_descriptor {
    pub size: u32,
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
    pub extent: mln_render_target_extent,
    pub context: mln_vulkan_context_descriptor,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vulkan_owned_texture_frame {
    pub size: u32,
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub image: mln_vulkan_non_dispatchable_handle,
    pub image_view: mln_vulkan_non_dispatchable_handle,
    pub device: *mut std::ffi::c_void,
    pub format: u32,
    pub layout: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_vulkan_surface_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub context: mln_vulkan_context_descriptor,
    pub surface: mln_vulkan_non_dispatchable_handle,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_wake {
    pub size: u32,
    pub callback: mln_wake_callback,
    pub user_data: *mut std::ffi::c_void,
    pub release_user_data: mln_wake_release,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgl_context_descriptor {
    pub size: u32,
    pub kind: u32,
    pub context: i32,
    pub canvas_selector: mln_buffer_view,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_borrowed_texture_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub physical_width: u32,
    pub physical_height: u32,
    pub context: mln_webgpu_context_descriptor,
    pub texture: *mut std::ffi::c_void,
    pub texture_view: *mut std::ffi::c_void,
    pub format: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_context_descriptor {
    pub size: u32,
    pub instance: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub queue: *mut std::ffi::c_void,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_owned_texture_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub context: mln_webgpu_context_descriptor,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_owned_texture_frame {
    pub size: u32,
    pub generation: u64,
    pub width: u32,
    pub height: u32,
    pub scale_factor: f64,
    pub frame_id: u64,
    pub texture: *mut std::ffi::c_void,
    pub texture_view: *mut std::ffi::c_void,
    pub device: *mut std::ffi::c_void,
    pub format: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_webgpu_surface_descriptor {
    pub size: u32,
    pub extent: mln_render_target_extent,
    pub context: mln_webgpu_context_descriptor,
    pub surface: *mut std::ffi::c_void,
    pub format: u32,
}
#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct mln_wgl_context_descriptor {
    pub size: u32,
    pub device_context: *mut std::ffi::c_void,
    pub share_context: *mut std::ffi::c_void,
    pub get_proc_address: *mut std::ffi::c_void,
}
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_acquired_frame(pub u64);
pub type mln_completion_callback = Option<
    unsafe extern "C" fn(user_data: *mut std::ffi::c_void, result: *const mln_completion_result),
>;
pub type mln_completion_release = Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
pub type mln_custom_geometry_source_release_callback =
    Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
pub type mln_custom_geometry_source_tile_callback =
    Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void, tile_id: mln_canonical_tile_id)>;
pub type mln_custom_mvt_vector_source_release_callback =
    Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
pub type mln_custom_mvt_vector_source_tile_callback =
    Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void, tile_id: mln_canonical_tile_id)>;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_event_batch(pub u64);
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_geojson_source_data(pub u64);
pub type mln_http_header_transform_callback = Option<
    unsafe extern "C" fn(
        user_data: *mut std::ffi::c_void,
        kind: u32,
        url: *const std::ffi::c_char,
        out_response: *mut mln_http_header_transform_response,
    ) -> mln_status,
>;
pub type mln_log_callback = Option<
    unsafe extern "C" fn(
        user_data: *mut std::ffi::c_void,
        severity: u32,
        event: u32,
        code: i64,
        message: *const std::ffi::c_char,
    ) -> u32,
>;
pub type mln_log_callback_release = Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_map(pub u64);
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_map_projection(pub u64);
pub type mln_offline_region_id = i64;
pub type mln_queue_lock_callback = Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
pub type mln_queue_lock_release = Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_render_frame_batch(pub u64);
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_render_session(pub u64);
pub type mln_resource_provider_callback = Option<
    unsafe extern "C" fn(
        user_data: *mut std::ffi::c_void,
        request: *const mln_resource_request,
        handle: mln_resource_request_handle,
    ) -> u32,
>;
pub type mln_resource_request_cancel_callback =
    Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_resource_request_handle(pub u64);
pub type mln_resource_transform_callback = Option<
    unsafe extern "C" fn(
        user_data: *mut std::ffi::c_void,
        kind: u32,
        url: *const std::ffi::c_char,
        out_response: *mut mln_resource_transform_response,
    ) -> mln_status,
>;
#[repr(transparent)]
#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct mln_runtime(pub u64);
pub type mln_runtime_callback_release =
    Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
pub type mln_vulkan_non_dispatchable_handle = u64;
pub type mln_wake_callback = Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
pub type mln_wake_release = Option<unsafe extern "C" fn(user_data: *mut std::ffi::c_void)>;
native_handles!(
    mln_acquired_frame,
    mln_event_batch,
    mln_geojson_source_data,
    mln_map,
    mln_map_projection,
    mln_render_frame_batch,
    mln_render_session,
    mln_resource_request_handle,
    mln_runtime
);

unsafe extern "C" {
    pub fn mln_acquired_frame_dispose(
        frame: mln_acquired_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_get_metal_texture(
        frame: mln_acquired_frame,
        out_frame: *mut mln_metal_owned_texture_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_get_opengl_texture(
        frame: mln_acquired_frame,
        out_frame: *mut mln_opengl_owned_texture_frame,
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
        out_frame: *mut mln_vulkan_owned_texture_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_get_webgpu_texture(
        frame: mln_acquired_frame,
        out_frame: *mut mln_webgpu_owned_texture_frame,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_acquired_frame_release(
        frame: *mut mln_acquired_frame,
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
        jni_class: *mut std::ffi::c_void,
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
    pub fn mln_event_batch_get(
        batch: mln_event_batch,
        out_view: *mut mln_event_batch_view,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_event_batch_release(batch: mln_event_batch);
    pub fn mln_frame_demand_default() -> mln_frame_demand;
    pub fn mln_free_camera_options_default() -> mln_free_camera_options;
    pub fn mln_geojson_source_data_create(
        data: mln_buffer_view,
        options: *const mln_geojson_source_options,
        out_data: *mut mln_geojson_source_data,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_geojson_source_data_destroy(data: mln_geojson_source_data);
    pub fn mln_geojson_source_options_default() -> mln_geojson_source_options;
    pub fn mln_gpu_sync_default() -> mln_gpu_sync;
    pub fn mln_http_header_transform_response_set(
        response: *mut mln_http_header_transform_response,
        name: *const std::ffi::c_char,
        name_size: usize,
        value: *const std::ffi::c_char,
        value_size: usize,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_lat_lng_for_projected_meters(
        meters: mln_projected_meters,
        out_coordinate: *mut mln_lat_lng,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_log_clear_callback(out_diagnostic: *mut mln_diagnostic) -> mln_status;
    pub fn mln_log_set_async_severity_mask(
        mask: u32,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_log_set_callback(
        callback: mln_log_callback,
        user_data: *mut std::ffi::c_void,
        release_user_data: mln_log_callback_release,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_color_relief_layer(
        map: mln_map,
        layer_id: mln_buffer_view,
        source_id: mln_buffer_view,
        before_layer_id: mln_buffer_view,
        completion: *const mln_completion,
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
    pub fn mln_map_add_hillshade_layer(
        map: mln_map,
        layer_id: mln_buffer_view,
        source_id: mln_buffer_view,
        before_layer_id: mln_buffer_view,
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
    pub fn mln_map_add_location_indicator_layer(
        map: mln_map,
        layer_id: mln_buffer_view,
        before_layer_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_raster_dem_source_tiles(
        map: mln_map,
        source_id: mln_buffer_view,
        tiles: *const mln_buffer_view,
        tile_count: usize,
        options: *const mln_style_tile_source_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_raster_dem_source_url(
        map: mln_map,
        source_id: mln_buffer_view,
        url: mln_buffer_view,
        options: *const mln_style_tile_source_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_raster_source_tiles(
        map: mln_map,
        source_id: mln_buffer_view,
        tiles: *const mln_buffer_view,
        tile_count: usize,
        options: *const mln_style_tile_source_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_raster_source_url(
        map: mln_map,
        source_id: mln_buffer_view,
        url: mln_buffer_view,
        options: *const mln_style_tile_source_options,
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
    pub fn mln_map_add_vector_source_tiles(
        map: mln_map,
        source_id: mln_buffer_view,
        tiles: *const mln_buffer_view,
        tile_count: usize,
        options: *const mln_style_tile_source_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_add_vector_source_url(
        map: mln_map,
        source_id: mln_buffer_view,
        url: mln_buffer_view,
        options: *const mln_style_tile_source_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_apply_camera_delta(
        map: mln_map,
        delta: *const mln_camera_delta,
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
    pub fn mln_map_camera_query(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_camera_snapshot_get(
        map: mln_map,
        out_camera: *mut mln_camera_options,
        out_generation: *mut u64,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_cancel_transitions(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_copy_layer_source_id(
        map: mln_map,
        layer_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_copy_layer_source_layer(
        map: mln_map,
        layer_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_copy_style_image_premultiplied_rgba8(
        map: mln_map,
        image_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_copy_style_image_stretches(
        map: mln_map,
        image_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_copy_style_source_attribution(
        map: mln_map,
        source_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_copy_style_source_url(
        map: mln_map,
        source_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_create(
        runtime: mln_runtime,
        options: *const mln_map_options,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_dispose(map: mln_map, out_diagnostic: *mut mln_diagnostic) -> mln_status;
    pub fn mln_map_dump_debug_logs(
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
    pub fn mln_map_get_layer_filter(
        map: mln_map,
        layer_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_layer_property(
        map: mln_map,
        layer_id: mln_buffer_view,
        property_name: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_image_info(
        map: mln_map,
        image_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_layer_info(
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
    pub fn mln_map_get_style_light_property(
        map: mln_map,
        property_name: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_source_info(
        map: mln_map,
        source_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_get_style_source_tile_urls(
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
    pub fn mln_map_lat_lng_for_pixel(
        map: mln_map,
        point: mln_screen_point,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_lat_lng_for_pixel_unwrapped(
        map: mln_map,
        point: mln_screen_point,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_lat_lngs_for_pixels(
        map: mln_map,
        points: *const mln_screen_point,
        point_count: usize,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_lat_lngs_for_pixels_unwrapped(
        map: mln_map,
        points: *const mln_screen_point,
        point_count: usize,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_list_style_layer_ids(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_list_style_layers(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_list_style_source_ids(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_loaded_style_json(
        map: mln_map,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_meters_per_pixel_at_latitude(
        map: mln_map,
        latitude: f64,
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
    pub fn mln_map_pixel_for_lat_lng(
        map: mln_map,
        coordinate: mln_lat_lng,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_pixels_for_lat_lngs(
        map: mln_map,
        coordinates: *const mln_lat_lng,
        coordinate_count: usize,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_close(
        projection: mln_map_projection,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_create(
        map: mln_map,
        completion: *const mln_completion,
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
    pub fn mln_map_projection_set_camera(
        projection: mln_map_projection,
        camera: *const mln_camera_options,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_set_visible_coordinates(
        projection: mln_map_projection,
        coordinates: *const mln_lat_lng,
        coordinate_count: usize,
        padding: mln_edge_insets,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_projection_set_visible_geometry(
        projection: mln_map_projection,
        geometry: mln_buffer_view,
        padding: mln_edge_insets,
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
        options: u32,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_event_mask(
        map: mln_map,
        mask: u64,
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
    pub fn mln_map_set_layer_filter(
        map: mln_map,
        layer_id: mln_buffer_view,
        filter: *const mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_layer_max_zoom(
        map: mln_map,
        layer_id: mln_buffer_view,
        max_zoom: f64,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_layer_min_zoom(
        map: mln_map,
        layer_id: mln_buffer_view,
        min_zoom: f64,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_layer_property(
        map: mln_map,
        layer_id: mln_buffer_view,
        property_name: mln_buffer_view,
        value: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_layer_source_id(
        map: mln_map,
        layer_id: mln_buffer_view,
        source_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_layer_source_layer(
        map: mln_map,
        layer_id: mln_buffer_view,
        source_layer: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_layer_visibility(
        map: mln_map,
        layer_id: mln_buffer_view,
        visibility: u32,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_location_indicator_accuracy_radius(
        map: mln_map,
        layer_id: mln_buffer_view,
        radius: f64,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_location_indicator_bearing(
        map: mln_map,
        layer_id: mln_buffer_view,
        bearing: f64,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_location_indicator_image_name(
        map: mln_map,
        layer_id: mln_buffer_view,
        image_kind: u32,
        image_id: mln_buffer_view,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_set_location_indicator_location(
        map: mln_map,
        layer_id: mln_buffer_view,
        coordinate: mln_lat_lng,
        altitude: f64,
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
        url: *const std::ffi::c_char,
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
    pub fn mln_map_snapshot_get(
        map: mln_map,
        out_snapshot: *mut mln_map_snapshot,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_map_style_url(
        map: mln_map,
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
    pub fn mln_metal_borrowed_texture_attach(
        map: mln_map,
        descriptor: *const mln_metal_borrowed_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_metal_borrowed_texture_descriptor_default() -> mln_metal_borrowed_texture_descriptor;
    pub fn mln_metal_borrowed_texture_set_target(
        session: mln_render_session,
        descriptor: *const mln_metal_borrowed_texture_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_metal_owned_texture_attach(
        map: mln_map,
        descriptor: *const mln_metal_owned_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_metal_owned_texture_descriptor_default() -> mln_metal_owned_texture_descriptor;
    pub fn mln_metal_surface_attach(
        map: mln_map,
        descriptor: *const mln_metal_surface_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_metal_surface_descriptor_default() -> mln_metal_surface_descriptor;
    pub fn mln_metal_surface_set_target(
        session: mln_render_session,
        descriptor: *const mln_metal_surface_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_network_status_get(
        out_status: *mut u32,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_network_status_set(status: u32, out_diagnostic: *mut mln_diagnostic) -> mln_status;
    pub fn mln_opengl_borrowed_texture_attach(
        map: mln_map,
        descriptor: *const mln_opengl_borrowed_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_opengl_borrowed_texture_descriptor_default() -> mln_opengl_borrowed_texture_descriptor;
    pub fn mln_opengl_borrowed_texture_set_target(
        session: mln_render_session,
        descriptor: *const mln_opengl_borrowed_texture_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_opengl_owned_texture_attach(
        map: mln_map,
        descriptor: *const mln_opengl_owned_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_opengl_owned_texture_descriptor_default() -> mln_opengl_owned_texture_descriptor;
    pub fn mln_opengl_supported_context_provider_mask() -> u32;
    pub fn mln_opengl_surface_attach(
        map: mln_map,
        descriptor: *const mln_opengl_surface_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_opengl_surface_descriptor_default() -> mln_opengl_surface_descriptor;
    pub fn mln_opengl_surface_set_target(
        session: mln_render_session,
        descriptor: *const mln_opengl_surface_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_plugin_get_register_function_v1() -> mln_plugin_register_function_v1;
    pub fn mln_premultiplied_rgba8_image_default() -> mln_premultiplied_rgba8_image;
    pub fn mln_projected_meters_for_lat_lng(
        coordinate: mln_lat_lng,
        out_meters: *mut mln_projected_meters,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_projection_mode_default() -> mln_projection_mode;
    pub fn mln_render_frame_batch_get(
        batch: mln_render_frame_batch,
        out_view: *mut mln_render_frame_batch_view,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
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
    pub fn mln_render_session_destroy(
        session: mln_render_session,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_detach(
        session: mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_dispose(
        session: mln_render_session,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_drain_frame_results(
        session: mln_render_session,
        out_batch: *mut mln_render_frame_batch,
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
    pub fn mln_render_session_projection_create(
        session: mln_render_session,
        out_projection: *mut mln_map_projection,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_query_feature_extensions(
        session: mln_render_session,
        source_id: mln_buffer_view,
        feature: mln_buffer_view,
        extension: mln_buffer_view,
        extension_field: mln_buffer_view,
        arguments: *const mln_buffer_view,
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
        extent: *const mln_render_target_extent,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_session_service_driver_work(
        session: mln_render_session,
        max_work: usize,
        out_serviced: *mut usize,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_render_target_extent_physical_size(
        extent: *const mln_render_target_extent,
        out_width: *mut u32,
        out_height: *mut u32,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_rendered_feature_query_options_default() -> mln_rendered_feature_query_options;
    pub fn mln_rendered_query_geometry_box(box_: mln_screen_box) -> mln_rendered_query_geometry;
    pub fn mln_rendered_query_geometry_line_string(
        points: *const mln_screen_point,
        point_count: usize,
    ) -> mln_rendered_query_geometry;
    pub fn mln_rendered_query_geometry_point(
        point: mln_screen_point,
    ) -> mln_rendered_query_geometry;
    pub fn mln_resource_request_cancelled(
        handle: mln_resource_request_handle,
        out_cancelled: *mut bool,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_resource_request_complete(
        handle: mln_resource_request_handle,
        response: *const mln_resource_response,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_resource_request_release(handle: mln_resource_request_handle);
    pub fn mln_resource_request_set_cancel_callback(
        handle: mln_resource_request_handle,
        callback: mln_resource_request_cancel_callback,
        user_data: *mut std::ffi::c_void,
        release_user_data: mln_runtime_callback_release,
        out_cancelled: *mut bool,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_resource_request_wait_until_retired(
        handle: mln_resource_request_handle,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_resource_transform_response_set_url(
        response: *mut mln_resource_transform_response,
        url: *const std::ffi::c_char,
        url_size: usize,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_barrier(
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
    pub fn mln_runtime_dispose(
        runtime: mln_runtime,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_drain_events(
        runtime: mln_runtime,
        out_batch: *mut mln_event_batch,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_get_event_mask(
        runtime: mln_runtime,
        out_mask: *mut u64,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_region_create(
        runtime: mln_runtime,
        definition: *const mln_offline_region_definition,
        metadata: *const u8,
        metadata_size: usize,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_region_delete(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_region_get(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_region_get_status(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_region_invalidate(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_region_set_download_state(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        state: u32,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_region_set_observed(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        observed: bool,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_region_update_metadata(
        runtime: mln_runtime,
        region_id: mln_offline_region_id,
        metadata: *const u8,
        metadata_size: usize,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_regions_list(
        runtime: mln_runtime,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_offline_regions_merge_database(
        runtime: mln_runtime,
        side_database_path: *const std::ffi::c_char,
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
        operation: u32,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_runtime_set_event_mask(
        runtime: mln_runtime,
        mask: u64,
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
    pub fn mln_source_feature_query_options_default() -> mln_source_feature_query_options;
    pub fn mln_style_image_info_default() -> mln_style_image_info;
    pub fn mln_style_image_options_default() -> mln_style_image_options;
    pub fn mln_style_tile_source_options_default() -> mln_style_tile_source_options;
    pub fn mln_style_transition_options_default() -> mln_style_transition_options;
    pub fn mln_supported_render_backend_mask() -> u32;
    pub fn mln_texture_image_info_default() -> mln_texture_image_info;
    pub fn mln_texture_read_premultiplied_rgba8(
        session: mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_vulkan_borrowed_texture_attach(
        map: mln_map,
        descriptor: *const mln_vulkan_borrowed_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_vulkan_borrowed_texture_descriptor_default() -> mln_vulkan_borrowed_texture_descriptor;
    pub fn mln_vulkan_borrowed_texture_set_target(
        session: mln_render_session,
        descriptor: *const mln_vulkan_borrowed_texture_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_vulkan_owned_texture_attach(
        map: mln_map,
        descriptor: *const mln_vulkan_owned_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_vulkan_owned_texture_descriptor_default() -> mln_vulkan_owned_texture_descriptor;
    pub fn mln_vulkan_surface_attach(
        map: mln_map,
        descriptor: *const mln_vulkan_surface_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_vulkan_surface_descriptor_default() -> mln_vulkan_surface_descriptor;
    pub fn mln_vulkan_surface_set_target(
        session: mln_render_session,
        descriptor: *const mln_vulkan_surface_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_webgpu_borrowed_texture_attach(
        map: mln_map,
        descriptor: *const mln_webgpu_borrowed_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_webgpu_borrowed_texture_descriptor_default() -> mln_webgpu_borrowed_texture_descriptor;
    pub fn mln_webgpu_borrowed_texture_set_target(
        session: mln_render_session,
        descriptor: *const mln_webgpu_borrowed_texture_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_webgpu_owned_texture_attach(
        map: mln_map,
        descriptor: *const mln_webgpu_owned_texture_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_webgpu_owned_texture_descriptor_default() -> mln_webgpu_owned_texture_descriptor;
    pub fn mln_webgpu_surface_attach(
        map: mln_map,
        descriptor: *const mln_webgpu_surface_descriptor,
        options: *const mln_render_session_attach_options,
        out_session: *mut mln_render_session,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
    pub fn mln_webgpu_surface_descriptor_default() -> mln_webgpu_surface_descriptor;
    pub fn mln_webgpu_surface_set_target(
        session: mln_render_session,
        descriptor: *const mln_webgpu_surface_descriptor,
        completion: *const mln_completion,
        out_diagnostic: *mut mln_diagnostic,
    ) -> mln_status;
}
