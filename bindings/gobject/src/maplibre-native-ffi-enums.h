// Generated from the C headers by tools/bindgen. Do not edit.
#ifndef MAPLIBRE_NATIVE_FFI_ENUMS_H
#define MAPLIBRE_NATIVE_FFI_ENUMS_H

#include <glib-object.h>

G_BEGIN_DECLS

/**
 * MAPLIBRE_NATIVE_FFI_ERROR:
 *
 * The error domain of the library's failures. Each code is a failing status,
 * which `MaplibreNativeFfiError` names, and a code that it doesn't name is a
 * status that a later version of the native library added.
 */
#define MAPLIBRE_NATIVE_FFI_ERROR (maplibre_native_ffi_error_quark())
GQuark maplibre_native_ffi_error_quark(void);

/**
 * MaplibreNativeFfiError:
 * @MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT: An argument is invalid, such as
 *   malformed JSON or a value out of range.
 * @MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE: The call doesn't fit the current
 *   state, such as removing a source that a layer still uses.
 * @MAPLIBRE_NATIVE_FFI_ERROR_WRONG_THREAD: The call came from a thread that
 *   can't make it.
 * @MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED: The platform, the build, or the
 *   render target lacks the feature.
 * @MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR: The library or the platform hit an
 *   error, such as a failed graphics driver call.
 * @MAPLIBRE_NATIVE_FFI_ERROR_CANCELLED: The work stopped before it finished,
 *   such as when its handle was released.
 * @MAPLIBRE_NATIVE_FFI_ERROR_BUSY: Another call is still using the same object.
 * @MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST: The render target can't be drawn to
 *   anymore.
 * @MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND: Nothing has the given ID.
 * @MAPLIBRE_NATIVE_FFI_ERROR_ALREADY_EXISTS: Something already has the given
 *   ID.
 *
 * The result of a call or of asynchronous work.
 *
 * See `mln_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT = -1,
  MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE = -2,
  MAPLIBRE_NATIVE_FFI_ERROR_WRONG_THREAD = -3,
  MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED = -4,
  MAPLIBRE_NATIVE_FFI_ERROR_NATIVE_ERROR = -5,
  MAPLIBRE_NATIVE_FFI_ERROR_CANCELLED = -6,
  MAPLIBRE_NATIVE_FFI_ERROR_BUSY = -7,
  MAPLIBRE_NATIVE_FFI_ERROR_TARGET_LOST = -8,
  MAPLIBRE_NATIVE_FFI_ERROR_NOT_FOUND = -10,
  MAPLIBRE_NATIVE_FFI_ERROR_ALREADY_EXISTS = -11,
} MaplibreNativeFfiError;

GType maplibre_native_ffi_error_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_ERROR (maplibre_native_ffi_error_get_type())

/**
 * MaplibreNativeFfiCacheDatabaseOperation:
 * @MAPLIBRE_NATIVE_FFI_CACHE_DATABASE_OPERATION_CLEAR: Deletes the ambient
 *   cache's resources and frees the space they used. Offline regions keep their
 *   resources.
 * @MAPLIBRE_NATIVE_FFI_CACHE_DATABASE_OPERATION_INVALIDATE: Keeps the ambient
 *   cache's resources, but makes maps check each one with the server before
 *   they use it again. Offline regions keep their resources as they are.
 * @MAPLIBRE_NATIVE_FFI_CACHE_DATABASE_OPERATION_PACK: Compacts the whole
 *   database file to free the disk space that it no longer uses.
 * @MAPLIBRE_NATIVE_FFI_CACHE_DATABASE_OPERATION_RESET: Deletes the whole
 *   database, offline regions included, and starts an empty one.
 *
 * A maintenance action on a runtime's cache database.
 *
 * The cache database is the file that
 * `MaplibreNativeFfiRuntimeOptions:cache-path` names. It holds the ambient
 * cache, meaning the resources that maps loaded, and the offline regions.
 *
 * See `mln_cache_database_operation` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_CACHE_DATABASE_OPERATION_CLEAR = 1,
  MAPLIBRE_NATIVE_FFI_CACHE_DATABASE_OPERATION_INVALIDATE = 2,
  MAPLIBRE_NATIVE_FFI_CACHE_DATABASE_OPERATION_PACK = 3,
  MAPLIBRE_NATIVE_FFI_CACHE_DATABASE_OPERATION_RESET = 4,
} MaplibreNativeFfiCacheDatabaseOperation;

GType maplibre_native_ffi_cache_database_operation_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CACHE_DATABASE_OPERATION \
  (maplibre_native_ffi_cache_database_operation_get_type())

/**
 * MaplibreNativeFfiCameraChangeMode:
 * @MAPLIBRE_NATIVE_FFI_CAMERA_CHANGE_MODE_IMMEDIATE:
 * @MAPLIBRE_NATIVE_FFI_CAMERA_CHANGE_MODE_ANIMATED:
 *
 * Whether a camera change jumps or animates.
 *
 * See `mln_camera_change_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_CAMERA_CHANGE_MODE_IMMEDIATE = 0,
  MAPLIBRE_NATIVE_FFI_CAMERA_CHANGE_MODE_ANIMATED = 1,
} MaplibreNativeFfiCameraChangeMode;

GType maplibre_native_ffi_camera_change_mode_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_CHANGE_MODE \
  (maplibre_native_ffi_camera_change_mode_get_type())

/**
 * MaplibreNativeFfiCameraTransitionOutcome:
 * @MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_COMPLETED: The camera finished
 *   the change.
 * @MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_CANCELLED:
 *   `maplibre_native_ffi_map_cancel_camera_transitions_async()`,
 *   `maplibre_native_ffi_map_cancel_camera_transitions_with_id_async()`, a
 *   gesture's `MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_BEGIN` or
 *   `MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_CANCEL`,
 *   `maplibre_native_ffi_map_set_free_camera_options_async()`, or a resize that
 *   moved the camera back inside its bounds stopped the change. A command that
 *   fails before it moves the camera also reports this.
 * @MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_CLOSED: The map closed before
 *   the change ended.
 * @MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_REPLACED: A later camera
 *   change set every field that a part of the change was still animating, so
 *   that part stopped. Its other parts keep animating, and the change reports
 *   this outcome when they end, unless one was cancelled.
 *
 * How a camera change ended.
 *
 * A change that ends in more than one way reports
 * `MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_CANCELLED` if any of it was
 * cancelled, and otherwise
 * `MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_REPLACED` if any of it was
 * replaced.
 *
 * See `mln_camera_transition_outcome` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_COMPLETED = 0,
  MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_CANCELLED = 1,
  MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_CLOSED = 2,
  MAPLIBRE_NATIVE_FFI_CAMERA_TRANSITION_OUTCOME_REPLACED = 3,
} MaplibreNativeFfiCameraTransitionOutcome;

GType maplibre_native_ffi_camera_transition_outcome_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_TRANSITION_OUTCOME \
  (maplibre_native_ffi_camera_transition_outcome_get_type())

/**
 * MaplibreNativeFfiCameraUpdateMode:
 * @MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE_MODE_JUMP: Moves the camera at once.
 * @MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE_MODE_EASE: Animates the camera straight to
 *   the target.
 * @MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE_MODE_FLY: Zooms out, travels to the
 *   target, and zooms back in.
 *
 * How a camera command moves the camera.
 *
 * See `mln_camera_update_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE_MODE_JUMP = 0,
  MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE_MODE_EASE = 1,
  MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE_MODE_FLY = 2,
} MaplibreNativeFfiCameraUpdateMode;

GType maplibre_native_ffi_camera_update_mode_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_UPDATE_MODE \
  (maplibre_native_ffi_camera_update_mode_get_type())

/**
 * MaplibreNativeFfiConstrainMode:
 * @MAPLIBRE_NATIVE_FFI_CONSTRAIN_MODE_NONE:
 * @MAPLIBRE_NATIVE_FFI_CONSTRAIN_MODE_HEIGHT_ONLY: Keeps the world filling the
 *   view from top to bottom.
 * @MAPLIBRE_NATIVE_FFI_CONSTRAIN_MODE_WIDTH_AND_HEIGHT: Keeps the world filling
 *   the view in both directions.
 * @MAPLIBRE_NATIVE_FFI_CONSTRAIN_MODE_SCREEN: Keeps the whole view inside the
 *   bounds from `maplibre_native_ffi_map_set_bounds_async()`.
 *
 * How the map keeps the camera within the world or within its bounds.
 *
 * See `mln_constrain_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_CONSTRAIN_MODE_NONE = 0,
  MAPLIBRE_NATIVE_FFI_CONSTRAIN_MODE_HEIGHT_ONLY = 1,
  MAPLIBRE_NATIVE_FFI_CONSTRAIN_MODE_WIDTH_AND_HEIGHT = 2,
  MAPLIBRE_NATIVE_FFI_CONSTRAIN_MODE_SCREEN = 3,
} MaplibreNativeFfiConstrainMode;

GType maplibre_native_ffi_constrain_mode_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_CONSTRAIN_MODE \
  (maplibre_native_ffi_constrain_mode_get_type())

/**
 * MaplibreNativeFfiFrameDemandFlag:
 * @MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_FORCE: Draws a frame even when nothing
 *   changed since the last one. It can't be combined with
 *   `MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_WAIT_FOR_UPDATE`.
 * @MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_WAIT_FOR_UPDATE: When the map has nothing
 *   new to draw, waits for its next update instead of finishing with
 *   `MAPLIBRE_NATIVE_FFI_RENDER_RESULT_NO_UPDATE`.
 *
 * How a frame demand behaves.
 *
 * Without flags, a demand draws a frame when the map changed since the last
 * one, and otherwise finishes with
 * `MAPLIBRE_NATIVE_FFI_RENDER_RESULT_NO_UPDATE`. A session with a surface
 * target shows each frame that it draws.
 *
 * See `mln_frame_demand_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_FORCE = 1,
  MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_WAIT_FOR_UPDATE = 4,
} MaplibreNativeFfiFrameDemandFlag;

GType maplibre_native_ffi_frame_demand_flag_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_FRAME_DEMAND_FLAG \
  (maplibre_native_ffi_frame_demand_flag_get_type())

/**
 * MaplibreNativeFfiGesturePhase:
 * @MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_NONE: The change isn't part of a gesture.
 * @MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_BEGIN: The gesture starts. This cancels
 *   every running camera animation before the change applies.
 * @MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_UPDATE: The gesture continues. To start a
 *   gesture that keeps the running camera animations, start it with this phase
 *   instead of `MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_BEGIN`.
 * @MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_END:
 * @MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_CANCEL: The platform cancelled the
 *   gesture. This also cancels every running camera animation after the change
 *   applies.
 *
 * Where a camera change falls in a gesture that the user makes.
 *
 * `MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_BEGIN` and
 * `MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_UPDATE` mark a gesture in progress, and
 * `MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_END` and
 * `MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_CANCEL` end it.
 *
 * See `mln_gesture_phase` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_NONE = 0,
  MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_BEGIN = 1,
  MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_UPDATE = 2,
  MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_END = 3,
  MAPLIBRE_NATIVE_FFI_GESTURE_PHASE_CANCEL = 4,
} MaplibreNativeFfiGesturePhase;

GType maplibre_native_ffi_gesture_phase_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_GESTURE_PHASE \
  (maplibre_native_ffi_gesture_phase_get_type())

/**
 * MaplibreNativeFfiGpuSyncKind:
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_CPU_COMPLETE: The work has already finished, so
 *   there is nothing to wait for.
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_OPEN_GL_FENCE: `handle` is a `GLsync` in the
 *   session's share group.
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_QUEUE_ORDER: The work was submitted to the
 *   queue that both sides share, so work submitted to that queue later runs
 *   after it.
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE: `handle` is a
 *   `VkSemaphore`. Wait until it reaches `value`.
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_SYNC_FD: `handle` is a sync file descriptor.
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_METAL_SHARED_EVENT: `handle` is an
 *   `MTLSharedEvent`. Wait until it reaches `value`.
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_D3D12_FENCE: `handle` is a shared Windows
 *   handle to an `ID3D12Fence`. Wait until it reaches `value`.
 *
 * How one side waits for the other's GPU work on a shared texture.
 *
 * A sync file descriptor or an OpenGL fence signals once, so when
 * `maplibre_native_ffi_acquired_frame_release()` succeeds, the session takes
 * the one you pass. A Vulkan timeline semaphore, Metal shared event, or D3D12
 * fence counts up with each frame, so the one you pass stays yours. Keep it
 * valid until a detach or release of the session completes.
 *
 * See `mln_gpu_sync_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_CPU_COMPLETE = 0,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_OPEN_GL_FENCE = 1,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_QUEUE_ORDER = 2,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE = 3,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_SYNC_FD = 4,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_METAL_SHARED_EVENT = 5,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_D3D12_FENCE = 6,
} MaplibreNativeFfiGpuSyncKind;

GType maplibre_native_ffi_gpu_sync_kind_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_GPU_SYNC_KIND \
  (maplibre_native_ffi_gpu_sync_kind_get_type())

/**
 * MaplibreNativeFfiGpuSyncKindFlag:
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_CPU_COMPLETE:
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_OPEN_GL_FENCE:
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_QUEUE_ORDER:
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE:
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_SYNC_FD:
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT:
 * @MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_D3D12_FENCE:
 *
 * A GPU sync kind, as a bit in a set.
 *
 * See `mln_gpu_sync_kind_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_CPU_COMPLETE = 1,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_OPEN_GL_FENCE = 2,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_QUEUE_ORDER = 4,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE = 8,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_SYNC_FD = 16,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_METAL_SHARED_EVENT = 32,
  MAPLIBRE_NATIVE_FFI_GPU_SYNC_KIND_FLAG_D3D12_FENCE = 64,
} MaplibreNativeFfiGpuSyncKindFlag;

GType maplibre_native_ffi_gpu_sync_kind_flag_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_GPU_SYNC_KIND_FLAG \
  (maplibre_native_ffi_gpu_sync_kind_flag_get_type())

/**
 * MaplibreNativeFfiLogEvent:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_GENERAL:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_SETUP:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_SHADER:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_PARSE_STYLE:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_PARSE_TILE:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_RENDER:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_STYLE:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_DATABASE:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_HTTP_REQUEST:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_SPRITE:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_IMAGE:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_GRAPHICS_BACKEND:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_JNI:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_ANDROID:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_CRASH:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_GLYPH:
 * @MAPLIBRE_NATIVE_FFI_LOG_EVENT_TIMING:
 *
 * The part of the library that logged a message.
 *
 * See `mln_log_event` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_GENERAL = 0,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_SETUP = 1,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_SHADER = 2,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_PARSE_STYLE = 3,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_PARSE_TILE = 4,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_RENDER = 5,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_STYLE = 6,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_DATABASE = 7,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_HTTP_REQUEST = 8,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_SPRITE = 9,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_IMAGE = 10,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_GRAPHICS_BACKEND = 11,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_JNI = 12,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_ANDROID = 13,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_CRASH = 14,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_GLYPH = 15,
  MAPLIBRE_NATIVE_FFI_LOG_EVENT_TIMING = 16,
} MaplibreNativeFfiLogEvent;

GType maplibre_native_ffi_log_event_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_LOG_EVENT \
  (maplibre_native_ffi_log_event_get_type())

/**
 * MaplibreNativeFfiLogSeverity:
 * @MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_INFO:
 * @MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_WARNING:
 * @MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_ERROR:
 *
 * The severity of a log message.
 *
 * See `mln_log_severity` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_INFO = 1,
  MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_WARNING = 2,
  MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_ERROR = 3,
} MaplibreNativeFfiLogSeverity;

GType maplibre_native_ffi_log_severity_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_LOG_SEVERITY \
  (maplibre_native_ffi_log_severity_get_type())

/**
 * MaplibreNativeFfiLogSeverityFlag:
 * @MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_INFO:
 * @MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_WARNING:
 * @MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_ERROR:
 * @MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_DEFAULT: The severities logged on a
 *   background thread until `maplibre_native_ffi_set_log_async_severity_mask()`
 *   changes them.
 * @MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_ALL:
 *
 * A log severity, as a bit in a set.
 *
 * See `mln_log_severity_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/logging_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_INFO = 2,
  MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_WARNING = 4,
  MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_ERROR = 8,
  MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_DEFAULT = 6,
  MAPLIBRE_NATIVE_FFI_LOG_SEVERITY_FLAG_ALL = 14,
} MaplibreNativeFfiLogSeverityFlag;

GType maplibre_native_ffi_log_severity_flag_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_LOG_SEVERITY_FLAG \
  (maplibre_native_ffi_log_severity_flag_get_type())

/**
 * MaplibreNativeFfiMapDebugFlag:
 * @MAPLIBRE_NATIVE_FFI_MAP_DEBUG_TILE_BORDERS:
 * @MAPLIBRE_NATIVE_FFI_MAP_DEBUG_PARSE_STATUS:
 * @MAPLIBRE_NATIVE_FFI_MAP_DEBUG_TIMESTAMPS:
 * @MAPLIBRE_NATIVE_FFI_MAP_DEBUG_COLLISION:
 * @MAPLIBRE_NATIVE_FFI_MAP_DEBUG_OVERDRAW:
 * @MAPLIBRE_NATIVE_FFI_MAP_DEBUG_STENCIL_CLIP:
 * @MAPLIBRE_NATIVE_FFI_MAP_DEBUG_DEPTH_BUFFER:
 *
 * A debug overlay that the map draws.
 *
 * See `mln_map_debug_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_MAP_DEBUG_TILE_BORDERS = 2,
  MAPLIBRE_NATIVE_FFI_MAP_DEBUG_PARSE_STATUS = 4,
  MAPLIBRE_NATIVE_FFI_MAP_DEBUG_TIMESTAMPS = 8,
  MAPLIBRE_NATIVE_FFI_MAP_DEBUG_COLLISION = 16,
  MAPLIBRE_NATIVE_FFI_MAP_DEBUG_OVERDRAW = 32,
  MAPLIBRE_NATIVE_FFI_MAP_DEBUG_STENCIL_CLIP = 64,
  MAPLIBRE_NATIVE_FFI_MAP_DEBUG_DEPTH_BUFFER = 128,
} MaplibreNativeFfiMapDebugFlag;

GType maplibre_native_ffi_map_debug_flag_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_DEBUG_FLAG \
  (maplibre_native_ffi_map_debug_flag_get_type())

/**
 * MaplibreNativeFfiMapLoadError:
 * @MAPLIBRE_NATIVE_FFI_MAP_LOAD_ERROR_UNKNOWN:
 * @MAPLIBRE_NATIVE_FFI_MAP_LOAD_ERROR_STYLE_PARSE: The style document isn't a
 *   valid style.
 * @MAPLIBRE_NATIVE_FFI_MAP_LOAD_ERROR_STYLE_LOAD: The style's request failed,
 *   such as with a 404 or a missing file.
 *
 * Why a map failed to load a style.
 *
 * See `mln_map_load_error` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_MAP_LOAD_ERROR_UNKNOWN = 0,
  MAPLIBRE_NATIVE_FFI_MAP_LOAD_ERROR_STYLE_PARSE = 1,
  MAPLIBRE_NATIVE_FFI_MAP_LOAD_ERROR_STYLE_LOAD = 2,
} MaplibreNativeFfiMapLoadError;

GType maplibre_native_ffi_map_load_error_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_LOAD_ERROR \
  (maplibre_native_ffi_map_load_error_get_type())

/**
 * MaplibreNativeFfiMapMode:
 * @MAPLIBRE_NATIVE_FFI_MAP_MODE_CONTINUOUS: Renders whenever the map changes.
 * @MAPLIBRE_NATIVE_FFI_MAP_MODE_STATIC: Renders only when
 *   `maplibre_native_ffi_map_request_still_image_async()` asks for an image.
 * @MAPLIBRE_NATIVE_FFI_MAP_MODE_TILE: Renders still images of single tiles, as
 *   `MAPLIBRE_NATIVE_FFI_MAP_MODE_STATIC` does.
 *
 * When a map renders.
 *
 * See `mln_map_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_MAP_MODE_CONTINUOUS = 0,
  MAPLIBRE_NATIVE_FFI_MAP_MODE_STATIC = 1,
  MAPLIBRE_NATIVE_FFI_MAP_MODE_TILE = 2,
} MaplibreNativeFfiMapMode;

GType maplibre_native_ffi_map_mode_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_MAP_MODE \
  (maplibre_native_ffi_map_mode_get_type())

/**
 * MaplibreNativeFfiNetworkStatus:
 * @MAPLIBRE_NATIVE_FFI_NETWORK_STATUS_ONLINE:
 * @MAPLIBRE_NATIVE_FFI_NETWORK_STATUS_OFFLINE:
 *
 * Whether the library may use the network.
 *
 * See `mln_network_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_NETWORK_STATUS_ONLINE = 1,
  MAPLIBRE_NATIVE_FFI_NETWORK_STATUS_OFFLINE = 2,
} MaplibreNativeFfiNetworkStatus;

GType maplibre_native_ffi_network_status_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_NETWORK_STATUS \
  (maplibre_native_ffi_network_status_get_type())

/**
 * MaplibreNativeFfiNorthOrientation:
 * @MAPLIBRE_NATIVE_FFI_NORTH_ORIENTATION_UP:
 * @MAPLIBRE_NATIVE_FFI_NORTH_ORIENTATION_RIGHT:
 * @MAPLIBRE_NATIVE_FFI_NORTH_ORIENTATION_DOWN:
 * @MAPLIBRE_NATIVE_FFI_NORTH_ORIENTATION_LEFT:
 *
 * The screen direction that north points to when the bearing is 0.
 *
 * See `mln_north_orientation` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_NORTH_ORIENTATION_UP = 0,
  MAPLIBRE_NATIVE_FFI_NORTH_ORIENTATION_RIGHT = 1,
  MAPLIBRE_NATIVE_FFI_NORTH_ORIENTATION_DOWN = 2,
  MAPLIBRE_NATIVE_FFI_NORTH_ORIENTATION_LEFT = 3,
} MaplibreNativeFfiNorthOrientation;

GType maplibre_native_ffi_north_orientation_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_NORTH_ORIENTATION \
  (maplibre_native_ffi_north_orientation_get_type())

/**
 * MaplibreNativeFfiOfflineRegionDownloadState:
 * @MAPLIBRE_NATIVE_FFI_OFFLINE_REGION_DOWNLOAD_INACTIVE:
 * @MAPLIBRE_NATIVE_FFI_OFFLINE_REGION_DOWNLOAD_ACTIVE:
 *
 * Whether an offline region is downloading.
 *
 * See `mln_offline_region_download_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_OFFLINE_REGION_DOWNLOAD_INACTIVE = 0,
  MAPLIBRE_NATIVE_FFI_OFFLINE_REGION_DOWNLOAD_ACTIVE = 1,
} MaplibreNativeFfiOfflineRegionDownloadState;

GType maplibre_native_ffi_offline_region_download_state_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OFFLINE_REGION_DOWNLOAD_STATE \
  (maplibre_native_ffi_offline_region_download_state_get_type())

/**
 * MaplibreNativeFfiOpenGlClientApi:
 * @MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_UNSPECIFIED:
 * @MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GL:
 * @MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GLES:
 *
 * The OpenGL API that a context renders with.
 *
 * See `mln_open_gl_client_api` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_UNSPECIFIED = 0,
  MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GL = 1,
  MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GLES = 2,
} MaplibreNativeFfiOpenGlClientApi;

GType maplibre_native_ffi_open_gl_client_api_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CLIENT_API \
  (maplibre_native_ffi_open_gl_client_api_get_type())

/**
 * MaplibreNativeFfiOpenGlContextOwnership:
 * @MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_OWNERSHIP_SHARED: The session's context
 *   shares objects with your `share_context`.
 * @MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED: The session's
 *   context shares objects with no other context.
 *
 * Whether a render session's OpenGL context shares objects with yours.
 *
 * See `mln_open_gl_context_ownership` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_OWNERSHIP_SHARED = 0,
  MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_OWNERSHIP_DEDICATED = 1,
} MaplibreNativeFfiOpenGlContextOwnership;

GType maplibre_native_ffi_open_gl_context_ownership_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_OWNERSHIP \
  (maplibre_native_ffi_open_gl_context_ownership_get_type())

/**
 * MaplibreNativeFfiOpenGlContextProviderFlag:
 * @MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL:
 * @MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL:
 * @MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL:
 *
 * An OpenGL context platform, as a bit in a set.
 *
 * See `mln_open_gl_context_provider_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_PROVIDER_FLAG_WGL = 1,
  MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_PROVIDER_FLAG_EGL = 2,
  MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_PROVIDER_FLAG_WEB_GL = 4,
} MaplibreNativeFfiOpenGlContextProviderFlag;

GType maplibre_native_ffi_open_gl_context_provider_flag_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_OPEN_GL_CONTEXT_PROVIDER_FLAG \
  (maplibre_native_ffi_open_gl_context_provider_flag_get_type())

/**
 * MaplibreNativeFfiRenderAbandonDisposition:
 * @MAPLIBRE_NATIVE_FFI_RENDER_ABANDON_DISPOSITION_CLEAN:
 * @MAPLIBRE_NATIVE_FFI_RENDER_ABANDON_DISPOSITION_QUARANTINED: It kept some
 *   objects that it couldn't destroy safely. They stay allocated until the
 *   process exits.
 *
 * What `maplibre_native_ffi_render_session_abandon()` did with a session's
 * graphics objects.
 *
 * See `mln_render_abandon_disposition` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RENDER_ABANDON_DISPOSITION_CLEAN = 0,
  MAPLIBRE_NATIVE_FFI_RENDER_ABANDON_DISPOSITION_QUARANTINED = 1,
} MaplibreNativeFfiRenderAbandonDisposition;

GType maplibre_native_ffi_render_abandon_disposition_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_ABANDON_DISPOSITION \
  (maplibre_native_ffi_render_abandon_disposition_get_type())

/**
 * MaplibreNativeFfiRenderBackendFlag:
 * @MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_METAL:
 * @MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_VULKAN:
 * @MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_OPEN_GL:
 * @MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_WEB_GPU:
 *
 * A render backend, as a bit in a set.
 *
 * See `mln_render_backend_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/base_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_METAL = 1,
  MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_VULKAN = 2,
  MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_OPEN_GL = 4,
  MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_WEB_GPU = 8,
} MaplibreNativeFfiRenderBackendFlag;

GType maplibre_native_ffi_render_backend_flag_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_BACKEND_FLAG \
  (maplibre_native_ffi_render_backend_flag_get_type())

/**
 * MaplibreNativeFfiRenderDriverKind:
 * @MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER: A library thread.
 * @MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD: Your thread, when
 *   it calls `maplibre_native_ffi_render_session_service_driver_work()`.
 *
 * The thread that runs a render session's graphics work.
 *
 * See `mln_render_driver_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER = 1,
  MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD = 2,
} MaplibreNativeFfiRenderDriverKind;

GType maplibre_native_ffi_render_driver_kind_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_DRIVER_KIND \
  (maplibre_native_ffi_render_driver_kind_get_type())

/**
 * MaplibreNativeFfiRenderMode:
 * @MAPLIBRE_NATIVE_FFI_RENDER_MODE_PARTIAL:
 * @MAPLIBRE_NATIVE_FFI_RENDER_MODE_FULL:
 *
 * Whether a frame drew the map complete, or lacked resources that are still
 * loading.
 *
 * See `mln_render_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RENDER_MODE_PARTIAL = 0,
  MAPLIBRE_NATIVE_FFI_RENDER_MODE_FULL = 1,
} MaplibreNativeFfiRenderMode;

GType maplibre_native_ffi_render_mode_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_MODE \
  (maplibre_native_ffi_render_mode_get_type())

/**
 * MaplibreNativeFfiRenderResult:
 * @MAPLIBRE_NATIVE_FFI_RENDER_RESULT_RENDERED:
 * @MAPLIBRE_NATIVE_FFI_RENDER_RESULT_NO_UPDATE: The map had nothing new to
 *   draw.
 * @MAPLIBRE_NATIVE_FFI_RENDER_RESULT_TARGET_NOT_READY: The target couldn't take
 *   a frame, such as while a window is hidden or destroyed, or after
 *   `maplibre_native_ffi_render_session_release_target_async()`. Demand a frame
 *   again later.
 * @MAPLIBRE_NATIVE_FFI_RENDER_RESULT_SUPERSEDED: A later demand with the same
 *   flags replaced this one.
 * @MAPLIBRE_NATIVE_FFI_RENDER_RESULT_FAILED: The session failed, and
 *   `maplibre_native_ffi_render_session_get_failure()` reports why. Once
 *   `maplibre_native_ffi_render_session_release_async()` completes, you can
 *   attach a new session.
 *
 * How a frame demand finished.
 *
 * See `mln_render_result` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RENDER_RESULT_RENDERED = 0,
  MAPLIBRE_NATIVE_FFI_RENDER_RESULT_NO_UPDATE = 1,
  MAPLIBRE_NATIVE_FFI_RENDER_RESULT_TARGET_NOT_READY = 2,
  MAPLIBRE_NATIVE_FFI_RENDER_RESULT_SUPERSEDED = 3,
  MAPLIBRE_NATIVE_FFI_RENDER_RESULT_FAILED = 4,
} MaplibreNativeFfiRenderResult;

GType maplibre_native_ffi_render_result_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_RESULT \
  (maplibre_native_ffi_render_result_get_type())

/**
 * MaplibreNativeFfiRenderSessionCapabilityFlag:
 * @MAPLIBRE_NATIVE_FFI_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION:
 *   `maplibre_native_ffi_render_session_acquire_frame()` can take the frames it
 *   renders.
 * @MAPLIBRE_NATIVE_FFI_RENDER_SESSION_CAPABILITY_READBACK:
 *   `maplibre_native_ffi_render_session_read_texture_async()` can copy its
 *   texture to memory.
 * @MAPLIBRE_NATIVE_FFI_RENDER_SESSION_CAPABILITY_PRESENTATION: It shows frames
 *   on a surface.
 *
 * Something that a render session can do.
 *
 * See `mln_render_session_capability_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION = 1,
  MAPLIBRE_NATIVE_FFI_RENDER_SESSION_CAPABILITY_READBACK = 2,
  MAPLIBRE_NATIVE_FFI_RENDER_SESSION_CAPABILITY_PRESENTATION = 4,
} MaplibreNativeFfiRenderSessionCapabilityFlag;

GType maplibre_native_ffi_render_session_capability_flag_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_SESSION_CAPABILITY_FLAG \
  (maplibre_native_ffi_render_session_capability_flag_get_type())

/**
 * MaplibreNativeFfiRenderSessionState:
 * @MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_ATTACHING:
 * @MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_ATTACHED:
 * @MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_DETACHING:
 * @MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_DETACHED:
 * @MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_FAILED:
 * @MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_ABANDONED:
 *
 * Where a render session is in its life.
 *
 * See `mln_render_session_state` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__session_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_ATTACHING = 1,
  MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_ATTACHED = 2,
  MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_DETACHING = 3,
  MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_DETACHED = 4,
  MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_FAILED = 5,
  MAPLIBRE_NATIVE_FFI_RENDER_SESSION_STATE_ABANDONED = 6,
} MaplibreNativeFfiRenderSessionState;

GType maplibre_native_ffi_render_session_state_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RENDER_SESSION_STATE \
  (maplibre_native_ffi_render_session_state_get_type())

/**
 * MaplibreNativeFfiResourceErrorReason:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_OTHER: A reason that no other
 *   value names.
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_NOT_FOUND:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_SERVER: The server reported an
 *   error.
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_CONNECTION: The server couldn't be
 *   reached.
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_RATE_LIMIT:
 *
 * Why a resource request failed.
 *
 * See `mln_resource_error_reason` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_OTHER = 0,
  MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_NOT_FOUND = 1,
  MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_SERVER = 2,
  MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_CONNECTION = 3,
  MAPLIBRE_NATIVE_FFI_RESOURCE_ERROR_REASON_RATE_LIMIT = 4,
} MaplibreNativeFfiResourceErrorReason;

GType maplibre_native_ffi_resource_error_reason_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_ERROR_REASON \
  (maplibre_native_ffi_resource_error_reason_get_type())

/**
 * MaplibreNativeFfiResourceKind:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_UNKNOWN:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_STYLE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_SOURCE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_TILE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_GLYPHS:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_SPRITE_IMAGE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_SPRITE_JSON:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_IMAGE:
 *
 * The kind of a resource that the library loads.
 *
 * See `mln_resource_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_UNKNOWN = 0,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_STYLE = 1,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_SOURCE = 2,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_TILE = 3,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_GLYPHS = 4,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_SPRITE_IMAGE = 5,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_SPRITE_JSON = 6,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_IMAGE = 7,
} MaplibreNativeFfiResourceKind;

GType maplibre_native_ffi_resource_kind_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_KIND \
  (maplibre_native_ffi_resource_kind_get_type())

/**
 * MaplibreNativeFfiResourceKindFlag:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_UNKNOWN:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_STYLE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_SOURCE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_TILE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_GLYPHS:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_SPRITE_IMAGE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_SPRITE_JSON:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_IMAGE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_ALL:
 *
 * A resource kind, as a bit in a set.
 *
 * See `mln_resource_kind_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_UNKNOWN = 1,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_STYLE = 2,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_SOURCE = 4,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_TILE = 8,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_GLYPHS = 16,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_SPRITE_IMAGE = 32,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_SPRITE_JSON = 64,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_IMAGE = 128,
  MAPLIBRE_NATIVE_FFI_RESOURCE_KIND_FLAG_ALL = 255,
} MaplibreNativeFfiResourceKindFlag;

GType maplibre_native_ffi_resource_kind_flag_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_KIND_FLAG \
  (maplibre_native_ffi_resource_kind_flag_get_type())

/**
 * MaplibreNativeFfiResourceLoadingMethod:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_LOADING_METHOD_ALL: The cache or the network.
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_LOADING_METHOD_CACHE_ONLY:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_LOADING_METHOD_NETWORK_ONLY:
 *
 * Where a request may load its resource from.
 *
 * See `mln_resource_loading_method` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RESOURCE_LOADING_METHOD_ALL = 0,
  MAPLIBRE_NATIVE_FFI_RESOURCE_LOADING_METHOD_CACHE_ONLY = 1,
  MAPLIBRE_NATIVE_FFI_RESOURCE_LOADING_METHOD_NETWORK_ONLY = 2,
} MaplibreNativeFfiResourceLoadingMethod;

GType maplibre_native_ffi_resource_loading_method_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_LOADING_METHOD \
  (maplibre_native_ffi_resource_loading_method_get_type())

/**
 * MaplibreNativeFfiResourcePriority:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_PRIORITY_REGULAR:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_PRIORITY_LOW:
 *
 * The priority of a resource request.
 *
 * See `mln_resource_priority` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RESOURCE_PRIORITY_REGULAR = 0,
  MAPLIBRE_NATIVE_FFI_RESOURCE_PRIORITY_LOW = 1,
} MaplibreNativeFfiResourcePriority;

GType maplibre_native_ffi_resource_priority_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_PRIORITY \
  (maplibre_native_ffi_resource_priority_get_type())

/**
 * MaplibreNativeFfiResourceResponseStatus:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_OK:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_NO_CONTENT: The resource exists
 *   and is empty.
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED: The earlier copy
 *   that the request's `prior_` fields describe is current.
 *
 * The outcome of a resource request.
 *
 * See `mln_resource_response_status` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_OK = 0,
  MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_ERROR = 1,
  MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_NO_CONTENT = 2,
  MAPLIBRE_NATIVE_FFI_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED = 3,
} MaplibreNativeFfiResourceResponseStatus;

GType maplibre_native_ffi_resource_response_status_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_RESPONSE_STATUS \
  (maplibre_native_ffi_resource_response_status_get_type())

/**
 * MaplibreNativeFfiResourceStoragePolicy:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_STORAGE_POLICY_PERMANENT:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_STORAGE_POLICY_VOLATILE:
 *
 * Whether the library keeps a loaded resource in its cache.
 *
 * See `mln_resource_storage_policy` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RESOURCE_STORAGE_POLICY_PERMANENT = 0,
  MAPLIBRE_NATIVE_FFI_RESOURCE_STORAGE_POLICY_VOLATILE = 1,
} MaplibreNativeFfiResourceStoragePolicy;

GType maplibre_native_ffi_resource_storage_policy_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_STORAGE_POLICY \
  (maplibre_native_ffi_resource_storage_policy_get_type())

/**
 * MaplibreNativeFfiResourceUsage:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_USAGE_ONLINE:
 * @MAPLIBRE_NATIVE_FFI_RESOURCE_USAGE_OFFLINE:
 *
 * Whether a request loads a resource for a map or an offline region.
 *
 * See `mln_resource_usage` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RESOURCE_USAGE_ONLINE = 0,
  MAPLIBRE_NATIVE_FFI_RESOURCE_USAGE_OFFLINE = 1,
} MaplibreNativeFfiResourceUsage;

GType maplibre_native_ffi_resource_usage_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RESOURCE_USAGE \
  (maplibre_native_ffi_resource_usage_get_type())

/**
 * MaplibreNativeFfiRuntimeEventFlag:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_NONE:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_CAMERA_WILL_CHANGE:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_CAMERA_IS_CHANGING:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_CAMERA_DID_CHANGE:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_LOADING_STARTED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_LOADING_FINISHED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_LOADING_FAILED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_IDLE:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_ERROR:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_STARTED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_FINISHED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_STARTED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_FINISHED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_STYLE_IMAGE_MISSING:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_TILE_ACTION:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_SOURCE_CHANGED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_OFFLINE_REGION_STATUS_CHANGED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_OFFLINE_REGION_RESPONSE_ERROR:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_GLYPHS_REQUESTED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_GLYPHS_LOADED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_GLYPHS_ERROR:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_SPRITE_REQUESTED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_SPRITE_LOADED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_SPRITE_ERROR:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_ALL:
 *
 * An event type, as a bit in a set.
 *
 * See `mln_runtime_event_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_NONE = 0,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_CAMERA_WILL_CHANGE = 2,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_CAMERA_IS_CHANGING = 4,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_CAMERA_DID_CHANGE = 8,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED = 16,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_LOADING_STARTED = 32,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_LOADING_FINISHED = 64,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_LOADING_FAILED = 128,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_IDLE = 256,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_ERROR = 512,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_STARTED = 1024,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_FINISHED = 2048,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_STARTED = 4096,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_FINISHED = 8192,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_STYLE_IMAGE_MISSING = 16384,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_TILE_ACTION = 32768,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_SOURCE_CHANGED = 65536,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_OFFLINE_REGION_STATUS_CHANGED = 131072,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_OFFLINE_REGION_RESPONSE_ERROR = 262144,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED =
    524288,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_GLYPHS_REQUESTED = 1048576,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_GLYPHS_LOADED = 2097152,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_GLYPHS_ERROR = 4194304,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_SPRITE_REQUESTED = 8388608,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_SPRITE_LOADED = 16777216,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_SPRITE_ERROR = 33554432,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS = 66191358,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS = 917504,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_ALL = 67108862,
} MaplibreNativeFfiRuntimeEventFlag;

GType maplibre_native_ffi_runtime_event_flag_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_FLAG \
  (maplibre_native_ffi_runtime_event_flag_get_type())

/**
 * MaplibreNativeFfiRuntimeEventSourceType:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_SOURCE_RUNTIME:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_SOURCE_MAP:
 *
 * The kind of handle that raised an event.
 *
 * See `mln_runtime_event_source_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_SOURCE_RUNTIME = 0,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_SOURCE_MAP = 1,
} MaplibreNativeFfiRuntimeEventSourceType;

GType maplibre_native_ffi_runtime_event_source_type_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_SOURCE_TYPE \
  (maplibre_native_ffi_runtime_event_source_type_get_type())

/**
 * MaplibreNativeFfiRuntimeEventType:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_STYLE_LOADED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_LOADING_STARTED: The map started to
 *   load a style.
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_LOADING_FINISHED: The map loaded its
 *   style and everything that the current view needs, and rendered the result.
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_LOADING_FAILED: The map failed to load
 *   a style.
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_IDLE: The map rendered a complete
 *   frame and has nothing left to load, animate, or redraw.
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_ERROR:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING: The style uses an
 *   image that it doesn't have.
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_TILE_ACTION: A tile moved to another
 *   step of loading.
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SOURCE_CHANGED: A source in the style
 *   changed. Its TileJSON loaded for the first time or with new contents, or a
 *   call set its volatile flag, a GeoJSON source's data, or an image source's
 *   image or coordinates.
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR: A resource
 *   that the region needs failed to download.
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED:
 *   The region's download became inactive because the cache file's offline
 *   regions hold as many tiles from MapLibre's demo tile server as the limit
 *   allows. Only tiles of sources with a `maplibre://` URL count toward the
 *   limit.
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_GLYPHS_REQUESTED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_GLYPHS_LOADED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_GLYPHS_ERROR:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SPRITE_REQUESTED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SPRITE_LOADED:
 * @MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SPRITE_ERROR:
 *
 * The kind of an event.
 *
 * Only maps in `MAPLIBRE_NATIVE_FFI_MAP_MODE_CONTINUOUS` raise
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_LOADING_FINISHED`,
 * `MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_IDLE`, and the render frame and render
 * map events. Offline region events come only for the regions and types that
 * `maplibre_native_ffi_runtime_set_offline_region_observed_async()` selects.
 *
 * See `mln_runtime_event_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE = 1,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING = 2,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE = 3,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_STYLE_LOADED = 4,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_LOADING_STARTED = 5,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_LOADING_FINISHED = 6,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_LOADING_FAILED = 7,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_IDLE = 8,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_ERROR = 9,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED = 10,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED = 11,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED = 12,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED = 13,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING = 14,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_TILE_ACTION = 15,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SOURCE_CHANGED = 16,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED = 17,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR = 18,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED =
    19,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_GLYPHS_REQUESTED = 20,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_GLYPHS_LOADED = 21,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_GLYPHS_ERROR = 22,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SPRITE_REQUESTED = 23,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SPRITE_LOADED = 24,
  MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_SPRITE_ERROR = 25,
} MaplibreNativeFfiRuntimeEventType;

GType maplibre_native_ffi_runtime_event_type_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_RUNTIME_EVENT_TYPE \
  (maplibre_native_ffi_runtime_event_type_get_type())

/**
 * MaplibreNativeFfiStyleExpressionTypeKind:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_VALUE: Any type.
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_NUMBER:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_STRING:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_BOOLEAN:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_COLOR:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_OBJECT:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_FORMATTED:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_IMAGE:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_PADDING:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_VARIABLE_ANCHOR_OFFSET_COLLECTION:
 * @MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_ARRAY:
 *
 * The type of a style expression's result, or of the items of an array result.
 *
 * See `mln_style_expression_type_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style__expression_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_VALUE = 0,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_NUMBER = 1,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_STRING = 2,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_BOOLEAN = 3,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_COLOR = 4,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_OBJECT = 5,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_FORMATTED = 6,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_IMAGE = 7,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_PADDING = 8,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_VARIABLE_ANCHOR_OFFSET_COLLECTION =
    9,
  MAPLIBRE_NATIVE_FFI_STYLE_EXPRESSION_TYPE_ARRAY = 10,
} MaplibreNativeFfiStyleExpressionTypeKind;

GType maplibre_native_ffi_style_expression_type_kind_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_EXPRESSION_TYPE_KIND \
  (maplibre_native_ffi_style_expression_type_kind_get_type())

/**
 * MaplibreNativeFfiStyleImageTextFit:
 * @MAPLIBRE_NATIVE_FFI_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK:
 * @MAPLIBRE_NATIVE_FFI_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY:
 * @MAPLIBRE_NATIVE_FFI_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL:
 *
 * How an image stretches to fit text along one axis.
 *
 * See `mln_style_image_text_fit` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_STYLE_IMAGE_TEXT_FIT_STRETCH_OR_SHRINK = 0,
  MAPLIBRE_NATIVE_FFI_STYLE_IMAGE_TEXT_FIT_STRETCH_ONLY = 1,
  MAPLIBRE_NATIVE_FFI_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL = 2,
} MaplibreNativeFfiStyleImageTextFit;

GType maplibre_native_ffi_style_image_text_fit_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_IMAGE_TEXT_FIT \
  (maplibre_native_ffi_style_image_text_fit_get_type())

/**
 * MaplibreNativeFfiStyleLayerVisibility:
 * @MAPLIBRE_NATIVE_FFI_STYLE_LAYER_VISIBILITY_VISIBLE:
 * @MAPLIBRE_NATIVE_FFI_STYLE_LAYER_VISIBILITY_NONE:
 *
 * Whether a layer shows.
 *
 * See `mln_style_layer_visibility` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_STYLE_LAYER_VISIBILITY_VISIBLE = 0,
  MAPLIBRE_NATIVE_FFI_STYLE_LAYER_VISIBILITY_NONE = 1,
} MaplibreNativeFfiStyleLayerVisibility;

GType maplibre_native_ffi_style_layer_visibility_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_LAYER_VISIBILITY \
  (maplibre_native_ffi_style_layer_visibility_get_type())

/**
 * MaplibreNativeFfiStyleRasterDemEncoding:
 * @MAPLIBRE_NATIVE_FFI_STYLE_RASTER_DEM_ENCODING_MAPBOX:
 * @MAPLIBRE_NATIVE_FFI_STYLE_RASTER_DEM_ENCODING_TERRARIUM:
 *
 * How a raster DEM tile encodes elevation in its colors.
 *
 * See `mln_style_raster_dem_encoding` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_STYLE_RASTER_DEM_ENCODING_MAPBOX = 0,
  MAPLIBRE_NATIVE_FFI_STYLE_RASTER_DEM_ENCODING_TERRARIUM = 1,
} MaplibreNativeFfiStyleRasterDemEncoding;

GType maplibre_native_ffi_style_raster_dem_encoding_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_RASTER_DEM_ENCODING \
  (maplibre_native_ffi_style_raster_dem_encoding_get_type())

/**
 * MaplibreNativeFfiStyleSourceType:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_UNKNOWN:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_VECTOR:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_RASTER:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_RASTER_DEM:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_GEO_JSON:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_IMAGE:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_VIDEO:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_ANNOTATIONS:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_CUSTOM_GEOMETRY:
 * @MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_CUSTOM_VECTOR:
 *
 * The type of a style source.
 *
 * See `mln_style_source_type` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_UNKNOWN = 0,
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_VECTOR = 1,
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_RASTER = 2,
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_RASTER_DEM = 3,
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_GEO_JSON = 4,
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_IMAGE = 5,
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_VIDEO = 6,
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_ANNOTATIONS = 7,
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_CUSTOM_GEOMETRY = 8,
  MAPLIBRE_NATIVE_FFI_STYLE_SOURCE_TYPE_CUSTOM_VECTOR = 9,
} MaplibreNativeFfiStyleSourceType;

GType maplibre_native_ffi_style_source_type_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_SOURCE_TYPE \
  (maplibre_native_ffi_style_source_type_get_type())

/**
 * MaplibreNativeFfiStyleTileScheme:
 * @MAPLIBRE_NATIVE_FFI_STYLE_TILE_SCHEME_XYZ:
 * @MAPLIBRE_NATIVE_FFI_STYLE_TILE_SCHEME_TMS:
 *
 * Whether a tileset counts tile rows from the north (XYZ) or the south (TMS).
 *
 * See `mln_style_tile_scheme` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_STYLE_TILE_SCHEME_XYZ = 0,
  MAPLIBRE_NATIVE_FFI_STYLE_TILE_SCHEME_TMS = 1,
} MaplibreNativeFfiStyleTileScheme;

GType maplibre_native_ffi_style_tile_scheme_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_TILE_SCHEME \
  (maplibre_native_ffi_style_tile_scheme_get_type())

/**
 * MaplibreNativeFfiStyleVectorTileEncoding:
 * @MAPLIBRE_NATIVE_FFI_STYLE_VECTOR_TILE_ENCODING_MVT:
 * @MAPLIBRE_NATIVE_FFI_STYLE_VECTOR_TILE_ENCODING_MLT:
 *
 * The format of vector tiles.
 *
 * See `mln_style_vector_tile_encoding` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/style_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_STYLE_VECTOR_TILE_ENCODING_MVT = 0,
  MAPLIBRE_NATIVE_FFI_STYLE_VECTOR_TILE_ENCODING_MLT = 1,
} MaplibreNativeFfiStyleVectorTileEncoding;

GType maplibre_native_ffi_style_vector_tile_encoding_get_type(
  void
) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_STYLE_VECTOR_TILE_ENCODING \
  (maplibre_native_ffi_style_vector_tile_encoding_get_type())

/**
 * MaplibreNativeFfiTileLodMode:
 * @MAPLIBRE_NATIVE_FFI_TILE_LOD_MODE_DEFAULT: Uses the most detail at the
 *   center of the screen.
 * @MAPLIBRE_NATIVE_FFI_TILE_LOD_MODE_DISTANCE: Uses the most detail closest to
 *   the camera.
 *
 * How the map picks the level of detail of its tiles.
 *
 * See `mln_tile_lod_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_TILE_LOD_MODE_DEFAULT = 0,
  MAPLIBRE_NATIVE_FFI_TILE_LOD_MODE_DISTANCE = 1,
} MaplibreNativeFfiTileLodMode;

GType maplibre_native_ffi_tile_lod_mode_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_TILE_LOD_MODE \
  (maplibre_native_ffi_tile_lod_mode_get_type())

/**
 * MaplibreNativeFfiTileOperation:
 * @MAPLIBRE_NATIVE_FFI_TILE_OPERATION_REQUESTED_FROM_CACHE:
 * @MAPLIBRE_NATIVE_FFI_TILE_OPERATION_REQUESTED_FROM_NETWORK:
 * @MAPLIBRE_NATIVE_FFI_TILE_OPERATION_LOAD_FROM_NETWORK:
 * @MAPLIBRE_NATIVE_FFI_TILE_OPERATION_LOAD_FROM_CACHE:
 * @MAPLIBRE_NATIVE_FFI_TILE_OPERATION_START_PARSE:
 * @MAPLIBRE_NATIVE_FFI_TILE_OPERATION_END_PARSE:
 * @MAPLIBRE_NATIVE_FFI_TILE_OPERATION_ERROR:
 * @MAPLIBRE_NATIVE_FFI_TILE_OPERATION_CANCELLED:
 * @MAPLIBRE_NATIVE_FFI_TILE_OPERATION_NULL:
 *
 * A step in loading a tile.
 *
 * See `mln_tile_operation` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_TILE_OPERATION_REQUESTED_FROM_CACHE = 0,
  MAPLIBRE_NATIVE_FFI_TILE_OPERATION_REQUESTED_FROM_NETWORK = 1,
  MAPLIBRE_NATIVE_FFI_TILE_OPERATION_LOAD_FROM_NETWORK = 2,
  MAPLIBRE_NATIVE_FFI_TILE_OPERATION_LOAD_FROM_CACHE = 3,
  MAPLIBRE_NATIVE_FFI_TILE_OPERATION_START_PARSE = 4,
  MAPLIBRE_NATIVE_FFI_TILE_OPERATION_END_PARSE = 5,
  MAPLIBRE_NATIVE_FFI_TILE_OPERATION_ERROR = 6,
  MAPLIBRE_NATIVE_FFI_TILE_OPERATION_CANCELLED = 7,
  MAPLIBRE_NATIVE_FFI_TILE_OPERATION_NULL = 8,
} MaplibreNativeFfiTileOperation;

GType maplibre_native_ffi_tile_operation_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_TILE_OPERATION \
  (maplibre_native_ffi_tile_operation_get_type())

/**
 * MaplibreNativeFfiUrlMatchFlag:
 * @MAPLIBRE_NATIVE_FFI_URL_MATCH_GLOB: The URL matches the pattern as a glob.
 *   `*` matches any characters except `/`, `**` matches any characters, and `?`
 *   matches one character except `/`. `\` makes the next character match only
 *   itself. For example, `https:// *.example.com/tiles/ **` matches every URL
 *   under `/tiles/` on each subdomain of `example.com`.
 *
 * How a URL pattern matches a request's URL.
 *
 * A pattern matches the whole URL, query included. Without flags, it matches
 * only the URL that equals it exactly.
 *
 * See `mln_url_match_flag` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/runtime_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_URL_MATCH_GLOB = 1,
} MaplibreNativeFfiUrlMatchFlag;

GType maplibre_native_ffi_url_match_flag_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_URL_MATCH_FLAG \
  (maplibre_native_ffi_url_match_flag_get_type())

/**
 * MaplibreNativeFfiViewportMode:
 * @MAPLIBRE_NATIVE_FFI_VIEWPORT_MODE_DEFAULT:
 * @MAPLIBRE_NATIVE_FFI_VIEWPORT_MODE_FLIPPED_Y: Renders the image upside down.
 *
 * The vertical direction of the image that the map renders.
 *
 * See `mln_viewport_mode` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/map_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_VIEWPORT_MODE_DEFAULT = 0,
  MAPLIBRE_NATIVE_FFI_VIEWPORT_MODE_FLIPPED_Y = 1,
} MaplibreNativeFfiViewportMode;

GType maplibre_native_ffi_viewport_mode_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VIEWPORT_MODE \
  (maplibre_native_ffi_viewport_mode_get_type())

/**
 * MaplibreNativeFfiVulkanImageOwner:
 * @MAPLIBRE_NATIVE_FFI_VULKAN_IMAGE_OWNER_GRAPHICS_QUEUE: The image stays with
 *   the queue family of `graphics_queue_family_index`.
 * @MAPLIBRE_NATIVE_FFI_VULKAN_IMAGE_OWNER_EXTERNAL: Each frame releases the
 *   image to VK_QUEUE_FAMILY_EXTERNAL.
 * @MAPLIBRE_NATIVE_FFI_VULKAN_IMAGE_OWNER_FOREIGN: Each frame releases the
 *   image to VK_QUEUE_FAMILY_FOREIGN_EXT.
 *
 * The queue family that a borrowed image goes to after each frame.
 *
 * With `MAPLIBRE_NATIVE_FFI_VULKAN_IMAGE_OWNER_EXTERNAL` or
 * `MAPLIBRE_NATIVE_FFI_VULKAN_IMAGE_OWNER_FOREIGN`, acquire the image on your
 * side before you read it. You don't need to release it back.
 *
 * See `mln_vulkan_image_owner` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/texture_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_VULKAN_IMAGE_OWNER_GRAPHICS_QUEUE = 0,
  MAPLIBRE_NATIVE_FFI_VULKAN_IMAGE_OWNER_EXTERNAL = 1,
  MAPLIBRE_NATIVE_FFI_VULKAN_IMAGE_OWNER_FOREIGN = 2,
} MaplibreNativeFfiVulkanImageOwner;

GType maplibre_native_ffi_vulkan_image_owner_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_VULKAN_IMAGE_OWNER \
  (maplibre_native_ffi_vulkan_image_owner_get_type())

/**
 * MaplibreNativeFfiWebGlContextKind:
 * @MAPLIBRE_NATIVE_FFI_WEB_GL_CONTEXT_EXISTING: The session uses a context that
 *   you created with `emscripten_webgl_create_context()`.
 * @MAPLIBRE_NATIVE_FFI_WEB_GL_CONTEXT_TRANSFERRED_CANVAS: The library transfers
 *   the canvas that `canvas_selector` names to a worker and creates the context
 *   there.
 *
 * How a render session gets its WebGL context.
 *
 * See `mln_web_gl_context_kind` in the
 * [C API reference](https://maplibre.org/maplibre-native-ffi/reference/c/render__target_8h.html).
 */
typedef enum {
  MAPLIBRE_NATIVE_FFI_WEB_GL_CONTEXT_EXISTING = 0,
  MAPLIBRE_NATIVE_FFI_WEB_GL_CONTEXT_TRANSFERRED_CANVAS = 1,
} MaplibreNativeFfiWebGlContextKind;

GType maplibre_native_ffi_web_gl_context_kind_get_type(void) G_GNUC_CONST;
#define MAPLIBRE_NATIVE_FFI_TYPE_WEB_GL_CONTEXT_KIND \
  (maplibre_native_ffi_web_gl_context_kind_get_type())

G_END_DECLS

#endif
