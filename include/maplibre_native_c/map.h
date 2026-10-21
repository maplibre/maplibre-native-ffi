/**
 * @file maplibre_native_c/map.h
 * Public C API declarations for map lifecycle, shared types, and offline
 * regions.
 */

#ifndef MAPLIBRE_NATIVE_C_MAP_H
#define MAPLIBRE_NATIVE_C_MAP_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"
#include "runtime.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Field mask values for mln_camera_options. */
typedef enum MLN_BINDING("kind=bitmask") mln_camera_option_field : uint32_t {
  MLN_CAMERA_OPTION_CENTER = 1U << 0U,
  MLN_CAMERA_OPTION_ZOOM = 1U << 1U,
  MLN_CAMERA_OPTION_BEARING = 1U << 2U,
  MLN_CAMERA_OPTION_PITCH = 1U << 3U,
  MLN_CAMERA_OPTION_CENTER_ALTITUDE = 1U << 4U,
  MLN_CAMERA_OPTION_PADDING = 1U << 5U,
  MLN_CAMERA_OPTION_ANCHOR = 1U << 6U,
  MLN_CAMERA_OPTION_ROLL = 1U << 7U,
  MLN_CAMERA_OPTION_FOV = 1U << 8U,
} mln_camera_option_field;

/** Field mask values for mln_animation_options. */
typedef enum MLN_BINDING("kind=bitmask") mln_animation_option_field : uint32_t {
  MLN_ANIMATION_OPTION_DURATION = 1U << 0U,
  MLN_ANIMATION_OPTION_VELOCITY = 1U << 1U,
  MLN_ANIMATION_OPTION_MIN_ZOOM = 1U << 2U,
  MLN_ANIMATION_OPTION_EASING = 1U << 3U,
  MLN_ANIMATION_OPTION_TRANSITION_ID = 1U << 4U,
} mln_animation_option_field;

/** Field mask values for mln_camera_fit_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_camera_fit_option_field : uint32_t {
  MLN_CAMERA_FIT_OPTION_PADDING = 1U << 0U,
  MLN_CAMERA_FIT_OPTION_BEARING = 1U << 1U,
  MLN_CAMERA_FIT_OPTION_PITCH = 1U << 2U,
} mln_camera_fit_option_field;

/** Field mask values for mln_bound_options. */
typedef enum MLN_BINDING("kind=bitmask") mln_bound_option_field : uint32_t {
  /**
   * Selects mln_bound_options.bounds as a geographic constraint that the
   * camera center stays inside. Mutually exclusive with
   * MLN_BOUND_OPTION_UNBOUNDED.
   */
  MLN_BOUND_OPTION_BOUNDS = 1U << 0U,
  MLN_BOUND_OPTION_MIN_ZOOM = 1U << 1U,
  MLN_BOUND_OPTION_MAX_ZOOM = 1U << 2U,
  MLN_BOUND_OPTION_MIN_PITCH = 1U << 3U,
  MLN_BOUND_OPTION_MAX_PITCH = 1U << 4U,
  /**
   * Selects the unbounded geographic constraint, which leaves every camera
   * center unconstrained and lets the map pan freely across the antimeridian.
   * This differs from world bounds of -90/-180 to 90/180, which clamp
   * longitude to that range. Mutually exclusive with MLN_BOUND_OPTION_BOUNDS,
   * and leaves mln_bound_options.bounds unread.
   */
  MLN_BOUND_OPTION_UNBOUNDED = 1U << 5U,
} mln_bound_option_field;

/** Field mask values for mln_free_camera_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_free_camera_option_field : uint32_t {
  MLN_FREE_CAMERA_OPTION_POSITION = 1U << 0U,
  MLN_FREE_CAMERA_OPTION_ORIENTATION = 1U << 1U,
} mln_free_camera_option_field;

/** Field mask values for MapLibre axonometric rendering options. */
typedef enum MLN_BINDING("kind=bitmask") mln_projection_mode_field : uint32_t {
  MLN_PROJECTION_MODE_AXONOMETRIC = 1U << 0U,
  MLN_PROJECTION_MODE_X_SKEW = 1U << 1U,
  MLN_PROJECTION_MODE_Y_SKEW = 1U << 2U,
} mln_projection_mode_field;

/** Debug overlay mask values for mln_map_set_debug_options(). */
typedef enum MLN_BINDING("kind=bitmask") mln_map_debug_option : uint32_t {
  MLN_MAP_DEBUG_TILE_BORDERS = 1U << 1U,
  MLN_MAP_DEBUG_PARSE_STATUS = 1U << 2U,
  MLN_MAP_DEBUG_TIMESTAMPS = 1U << 3U,
  MLN_MAP_DEBUG_COLLISION = 1U << 4U,
  MLN_MAP_DEBUG_OVERDRAW = 1U << 5U,
  MLN_MAP_DEBUG_STENCIL_CLIP = 1U << 6U,
  MLN_MAP_DEBUG_DEPTH_BUFFER = 1U << 7U,
} mln_map_debug_option;

/** Map north orientation values used by mln_map_viewport_options. */
typedef enum mln_north_orientation : uint32_t {
  MLN_NORTH_ORIENTATION_UP = 0,
  MLN_NORTH_ORIENTATION_RIGHT = 1,
  MLN_NORTH_ORIENTATION_DOWN = 2,
  MLN_NORTH_ORIENTATION_LEFT = 3,
} mln_north_orientation;

/** Map constraint modes used by mln_map_viewport_options. */
typedef enum mln_constrain_mode : uint32_t {
  MLN_CONSTRAIN_MODE_NONE = 0,
  MLN_CONSTRAIN_MODE_HEIGHT_ONLY = 1,
  MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT = 2,
  MLN_CONSTRAIN_MODE_SCREEN = 3,
} mln_constrain_mode;

/** Viewport orientation modes used by mln_map_viewport_options. */
typedef enum mln_viewport_mode : uint32_t {
  MLN_VIEWPORT_MODE_DEFAULT = 0,
  MLN_VIEWPORT_MODE_FLIPPED_Y = 1,
} mln_viewport_mode;

/** Field mask values for mln_map_viewport_options. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_map_viewport_option_field : uint32_t {
  MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION = 1U << 0U,
  MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE = 1U << 1U,
  MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE = 1U << 2U,
  MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET = 1U << 3U,
} mln_map_viewport_option_field;

/** Tile LOD algorithms used by mln_map_tile_options. */
typedef enum mln_tile_lod_mode : uint32_t {
  MLN_TILE_LOD_MODE_DEFAULT = 0,
  MLN_TILE_LOD_MODE_DISTANCE = 1,
} mln_tile_lod_mode;

/** Field mask values for mln_map_tile_options. */
typedef enum MLN_BINDING("kind=bitmask") mln_map_tile_option_field : uint32_t {
  MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA = 1U << 0U,
  MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS = 1U << 1U,
  MLN_MAP_TILE_OPTION_LOD_SCALE = 1U << 2U,
  MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD = 1U << 3U,
  MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT = 1U << 4U,
  MLN_MAP_TILE_OPTION_LOD_MODE = 1U << 5U,
} mln_map_tile_option_field;

/** Map rendering modes used when creating a map. */
typedef enum mln_map_mode : uint32_t {
  /** Continuously updates as data arrives and map state changes. */
  MLN_MAP_MODE_CONTINUOUS = 0,
  /** Produces one-off still images of an arbitrary viewport. */
  MLN_MAP_MODE_STATIC = 1,
  /** Produces one-off still images for a single tile. */
  MLN_MAP_MODE_TILE = 2,
} mln_map_mode;
/** Logical extent in UI pixels and the device-pixel scale. */
typedef struct mln_logical_extent {
  /** Width in UI pixels. Defaults to 256. */
  uint32_t width;
  /** Height in UI pixels. Defaults to 256. */
  uint32_t height;
  /** Device pixels per UI pixel. Defaults to 1.0. */
  double scale_factor;
} mln_logical_extent;

/** Returns a 256 by 256 logical extent with one device pixel per UI pixel. */
MLN_API mln_logical_extent mln_logical_extent_default(void) MLN_NOEXCEPT;

/** Options used when creating a map. */
typedef struct mln_map_options {
  uint32_t size;
  /**
   * Initial logical extent. Width and height must be nonzero, and scale_factor
   * must be finite and positive. scale_factor is fixed for the map's lifetime.
   *
   * After creation, only mln_map_resize() changes the width and height. A
   * render session submits it to size the map to its target.
   */
  mln_logical_extent initial_extent;
  /** One of mln_map_mode. Defaults to MLN_MAP_MODE_CONTINUOUS. */
  uint32_t map_mode MLN_BINDING("enum=mln_map_mode");
  /**
   * Decodes MapLibre Tile (MLT) tiles whose integer streams use FastPFOR
   * encodings. Defaults to false.
   *
   * Enable this on maps that read vector sources created with
   * MLN_STYLE_VECTOR_TILE_ENCODING_MLT from a tile set that uses FastPFOR. It
   * is fixed for the lifetime of the map and applies to every MLT source the
   * map loads. A map created with this false decodes every other MLT encoding
   * and logs a tile parse warning for the FastPFOR ones.
   */
  bool fast_pfor_enabled;
  /**
   * Map-originated event types this map queues, as a bitwise OR of
   * mln_runtime_event_mask values.
   *
   * This field is always read. MLN_RUNTIME_EVENT_MASK_ALL selects every event
   * type this library reports, and MLN_RUNTIME_EVENT_MASK_NONE queues none.
   * Defaults to MLN_RUNTIME_EVENT_MASK_ALL.
   *
   * The mask applies throughout construction, including the camera events that
   * MapLibre reports while it initializes the map's size.
   */
  uint64_t event_mask MLN_BINDING("enum=mln_runtime_event_mask");
} mln_map_options;

/** Screen-space point in logical map pixels. */
typedef struct mln_screen_point {
  double x;
  double y;
} mln_screen_point;

/** Screen-space inset in logical map pixels. */
typedef struct mln_edge_insets {
  double top;
  double left;
  double bottom;
  double right;
} mln_edge_insets;

/** Geographic coordinate in degrees used by map and projection APIs. */
typedef struct mln_lat_lng {
  /** Latitude in degrees, finite and within [-90, 90] on input. */
  double latitude;
  /** Longitude in degrees. Input longitude must be finite. */
  double longitude;
} mln_lat_lng;

/** Camera fields used by snapshots and camera updates. */
typedef struct mln_camera_options {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_camera_option_field");
  mln_lat_lng center MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_CENTER");
  double center_altitude
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_CENTER_ALTITUDE");
  mln_edge_insets padding
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_PADDING");
  /** Optional screen-space focal point in logical map pixels. */
  mln_screen_point anchor
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_ANCHOR");
  double zoom MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_ZOOM");
  double bearing MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_BEARING");
  double pitch MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_PITCH");
  double roll MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_ROLL");
  double field_of_view MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_FOV");
} mln_camera_options;

/** Cubic easing curve for animated camera transitions. */
typedef struct mln_unit_bezier {
  double x1;
  double y1;
  double x2;
  double y2;
} mln_unit_bezier;

/** How the transitions of one camera command ended. */
typedef enum mln_camera_transition_outcome : uint32_t {
  /**
   * Every property of the command reached its target, or a later camera write
   * replaced it.
   */
  MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED = 0,
  /**
   * mln_map_cancel_transitions(), mln_map_cancel_camera_transition(), or
   * MLN_GESTURE_PHASE_CANCEL ended the transitions, or the command failed
   * before it started them.
   */
  MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED = 1,
  /** The map closed before the transitions ended. */
  MLN_CAMERA_TRANSITION_OUTCOME_CLOSED = 2,
} mln_camera_transition_outcome;

/**
 * The end of one camera command's transitions, borrowed for the callback.
 */
typedef struct mln_camera_transition_end {
  uint32_t size;
  /** One of mln_camera_transition_outcome. */
  uint32_t outcome MLN_BINDING("enum=mln_camera_transition_outcome");
  /**
   * Generation of the published snapshot that shows the camera where the
   * transitions left it. The map events of the change that ended them carry
   * this generation and are queued before the callback runs. Zero for
   * MLN_CAMERA_TRANSITION_OUTCOME_CLOSED.
   */
  uint64_t generation;
} mln_camera_transition_end;

/**
 * Receives the end of one camera command's transitions.
 *
 * A handler in queue mode queues a copy as an
 * mln_camera_transition_end_delivery instead.
 */
MLN_BINDING("delivery=MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END")
typedef void (*mln_camera_transition_end_callback)(
  void* user_data, const mln_camera_transition_end* end
);

/**
 * Callback or queue state that one camera command copies to report the end of
 * its transitions.
 *
 * A handler with neither a callback nor a queue is disabled, and a disabled
 * handler must not carry release_user_data. See
 * mln_animation_options.end_handler for when the callback runs.
 */
typedef struct mln_camera_transition_handler {
  mln_camera_transition_end_callback callback MLN_BINDING("nullable=true");
  /** Opaque host token in queue mode; native code never dereferences it. */
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data on the runtime worker after the callback
   * returns. A rejected command runs neither callback, and the caller keeps
   * user_data. Null in queue mode.
   */
  mln_user_data_release release_user_data;
  /**
   * Optional. Queue delivery in place of callback: the end is copied into one
   * MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END delivery carrying user_data,
   * followed by one MLN_DELIVERY_TYPE_RETIRED delivery where callback mode
   * runs release_user_data. A command rejects an invalid queue handle with
   * MLN_STATUS_INVALID_ARGUMENT and a released queue with
   * MLN_STATUS_INVALID_STATE.
   */
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_camera_transition_handler MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** Optional animation controls for camera transitions. */
typedef struct mln_animation_options {
  uint32_t fields MLN_BINDING("enum=mln_animation_option_field");
  /**
   * Duration in milliseconds. Must be finite and non-negative. Values that
   * would overflow MapLibre Native's internal duration are invalid.
   *
   * When omitted, ease transitions apply immediately. Fly transitions derive
   * their duration from velocity.
   */
  double duration_ms
    MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_DURATION");
  /**
   * Average fly velocity in screenfuls per second. Must be positive and finite;
   * defaults to 1.2 when omitted. A flight whose derived duration exceeds
   * Native's nanosecond duration range fails its completion with
   * MLN_STATUS_NATIVE_ERROR. An explicit duration overrides velocity.
   */
  double velocity MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_VELOCITY");
  /** Peak zoom for flyTo transitions. */
  double min_zoom MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_MIN_ZOOM");
  mln_unit_bezier easing
    MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_EASING");
  /**
   * Caller-chosen identity of the command that these options animate, which
   * mln_map_cancel_camera_transition() matches.
   *
   * The C API passes the value through without interpreting it, so callers
   * pick their own scheme, such as a monotonically increasing counter. Several
   * commands may share one identity. A command without this field matches no
   * identity.
   */
  uint64_t transition_id
    MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_TRANSITION_ID");
  /**
   * Reports the end of the command's transitions. Disabled by default.
   *
   * For each accepted command that carries an enabled handler, the callback
   * runs exactly once, on the runtime worker, and release_user_data runs after
   * it. A command ends when every MapLibre transition that it started has
   * ended, so a camera delta that pans and zooms reports once, after both. A
   * jump, a zero-duration transition, and a command that changes no camera
   * field end right after the command applies.
   *
   * The callback runs after the map events of the change that ended the
   * transitions are queued, and never from inside a MapLibre Native call. A
   * transition that a command ends reports before that command's completion
   * runs. A transition that ends outside a command, as rendered frames advance
   * it, reports from a later task on the runtime worker. When the map closes
   * first, the callback runs with MLN_CAMERA_TRANSITION_OUTCOME_CLOSED before
   * the release completes.
   *
   * The callback may call any C API that a completion callback may call,
   * including camera commands, but must return promptly. A rejected command
   * runs neither callback and leaves user_data with the caller.
   */
  mln_camera_transition_handler end_handler;
} mln_animation_options;

/** Camera transition behavior for mln_camera_update. */
typedef enum mln_camera_update_mode : uint32_t {
  MLN_CAMERA_UPDATE_MODE_JUMP = 0,
  MLN_CAMERA_UPDATE_MODE_EASE = 1,
  MLN_CAMERA_UPDATE_MODE_FLY = 2,
} mln_camera_update_mode;

/**
 * Gesture boundary carried atomically with a camera update or delta.
 *
 * The phase is applied around the camera write and is reported by
 * mln_map_snapshot.gesture_in_progress.
 */
typedef enum mln_gesture_phase : uint32_t {
  /** The update carries no gesture boundary and leaves the flag as it is. */
  MLN_GESTURE_PHASE_NONE = 0,
  /**
   * Marks a gesture as in progress before the camera write. It does not
   * cancel running transitions; use mln_map_cancel_transitions() for that.
   */
  MLN_GESTURE_PHASE_BEGIN = 1,
  /** Keeps the gesture marked as in progress before the camera write. */
  MLN_GESTURE_PHASE_UPDATE = 2,
  /** Clears the gesture flag after the camera write. */
  MLN_GESTURE_PHASE_END = 3,
  /**
   * Cancels transitions running after the camera write, then clears the
   * gesture flag.
   */
  MLN_GESTURE_PHASE_CANCEL = 4,
} mln_gesture_phase;

/** Field mask values for mln_camera_delta. */
typedef enum MLN_BINDING("kind=bitmask") mln_camera_delta_field : uint32_t {
  MLN_CAMERA_DELTA_OFFSET = 1U << 0U,
  MLN_CAMERA_DELTA_SCALE = 1U << 1U,
  MLN_CAMERA_DELTA_BEARING = 1U << 2U,
  MLN_CAMERA_DELTA_PITCH = 1U << 3U,
  MLN_CAMERA_DELTA_ANCHOR = 1U << 4U,
} mln_camera_delta_field;

/**
 * One atomic relative camera update.
 *
 * Each selected component resolves against the camera as it stands when the
 * command runs. Immediate deltas therefore compose exactly, however many are
 * queued. An animated delta starts from the value a running transition has
 * reached and replaces that component's transition, so queue animated deltas
 * only after the previous one has rendered or finished.
 *
 * OFFSET pans first, in the current camera's logical pixels: the content moves
 * by offset, so positive x moves it right and positive y moves it down, and a
 * pointer drag delta passes through unchanged. The pan stops short of the
 * horizon. SCALE, BEARING, and PITCH then change the camera about anchor when
 * ANCHOR is set, keeping the coordinate under anchor in the panned camera
 * fixed, or about the center otherwise. ANCHOR requires SCALE, BEARING, or
 * PITCH. ANCHOR combines with OFFSET only for an immediate delta, one whose
 * animation selects no DURATION or a zero duration; a two-finger gesture passes
 * its centroid movement as offset and its new centroid as anchor. An animated
 * bearing change takes the shorter way around.
 *
 * fields may be zero, in which case the delta changes only the gesture phase
 * and its animation's end handler runs at once. gesture_phase is applied around
 * the camera write, as in mln_camera_update. The command copies this struct
 * before returning.
 */
typedef struct mln_camera_delta {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_camera_delta_field");
  /** Pan in logical map pixels; the content moves by this offset. */
  mln_screen_point offset
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_OFFSET");
  /** Positive zoom factor; 2 zooms in one level. */
  double scale MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_SCALE");
  /** Degrees added to the bearing. */
  double bearing MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_BEARING");
  /** Degrees added to the pitch; positive tilts further from straight down. */
  double pitch MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_PITCH");
  /**
   * Screen point in logical map pixels that scale, bearing, and pitch keep
   * fixed.
   */
  mln_screen_point anchor
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_ANCHOR");
  mln_animation_options animation;
  uint32_t gesture_phase MLN_BINDING("enum=mln_gesture_phase");
  uint32_t reserved MLN_BINDING("kind=reserved");
} mln_camera_delta;

/**
 * One atomic absolute camera update.
 *
 * The command copies this struct before returning.
 */
typedef struct mln_camera_update {
  uint32_t size;
  uint32_t mode MLN_BINDING("enum=mln_camera_update_mode");
  mln_camera_options camera;
  mln_animation_options animation;
  uint32_t gesture_phase MLN_BINDING("enum=mln_gesture_phase");
  uint32_t reserved MLN_BINDING("kind=reserved");
} mln_camera_update;

/** Optional fitting controls for camera-for-viewport queries. */
typedef struct mln_camera_fit_options {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_camera_fit_option_field");
  mln_edge_insets padding
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_FIT_OPTION_PADDING");
  double bearing MLN_BINDING("mask=fields;bit=MLN_CAMERA_FIT_OPTION_BEARING");
  double pitch MLN_BINDING("mask=fields;bit=MLN_CAMERA_FIT_OPTION_PITCH");
} mln_camera_fit_options;

/** Three-component vector used by free camera options. */
typedef struct mln_vec3 {
  double x;
  double y;
  double z;
} mln_vec3;

/** Quaternion stored as x, y, z, w components. */
typedef struct mln_quaternion {
  double x;
  double y;
  double z;
  double w;
} mln_quaternion;

/** Free camera position and orientation in MapLibre Native camera space. */
typedef struct mln_free_camera_options {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_free_camera_option_field");
  mln_vec3 position
    MLN_BINDING("mask=fields;bit=MLN_FREE_CAMERA_OPTION_POSITION");
  mln_quaternion orientation
    MLN_BINDING("mask=fields;bit=MLN_FREE_CAMERA_OPTION_ORIENTATION");
} mln_free_camera_options;

/** Optional fields for mln_feature_state_selector. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_feature_state_selector_field : uint32_t {
  MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID = 1U << 0U,
  MLN_FEATURE_STATE_SELECTOR_FEATURE_ID = 1U << 1U,
  MLN_FEATURE_STATE_SELECTOR_STATE_KEY = 1U << 2U,
} mln_feature_state_selector_field;

/** Feature-state source, feature, and key selector. */
typedef struct mln_feature_state_selector {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_feature_state_selector_field");
  /** Source ID. Required and borrowed for the duration of the call. */
  mln_buffer_view source_id;
  /** Optional source layer ID. Required for vector-source disambiguation. */
  mln_buffer_view source_layer_id
    MLN_BINDING("mask=fields;bit=MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID");
  /** Optional feature ID. Required by set and get, optional for remove. */
  mln_buffer_view feature_id
    MLN_BINDING("mask=fields;bit=MLN_FEATURE_STATE_SELECTOR_FEATURE_ID");
  /** Optional state key. Used only by remove and requires feature_id. */
  mln_buffer_view state_key
    MLN_BINDING("mask=fields;bit=MLN_FEATURE_STATE_SELECTOR_STATE_KEY");
} mln_feature_state_selector;

/** Geographic bounds in degrees. */
typedef struct mln_lat_lng_bounds {
  mln_lat_lng southwest;
  mln_lat_lng northeast;
} mln_lat_lng_bounds;

/** Optional map camera constraint fields. */
typedef struct mln_bound_options {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_bound_option_field");
  /** Read when fields contains MLN_BOUND_OPTION_BOUNDS. */
  mln_lat_lng_bounds bounds
    MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_BOUNDS");
  double min_zoom MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_MIN_ZOOM");
  double max_zoom MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_MAX_ZOOM");
  double min_pitch MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_MIN_PITCH");
  double max_pitch MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_MAX_PITCH");
} mln_bound_options;

/** Tile-pyramid offline region definition. */
typedef struct mln_offline_tile_pyramid_region_definition {
  /** Style URL. Copied during region creation. */
  mln_buffer_view style_url;
  mln_lat_lng_bounds bounds;
  double min_zoom;
  /**
   * Maximum zoom. Positive infinity follows MapLibre Native behavior and lets
   * each tile source use its own maximum zoom.
   */
  double max_zoom;
  float pixel_ratio;
  bool include_ideographs;
} mln_offline_tile_pyramid_region_definition;

/** Geometry offline region definition. */
typedef struct mln_offline_geometry_region_definition {
  /** Style URL. Copied during region creation. */
  mln_buffer_view style_url;
  /** UTF-8 GeoJSON Geometry bytes. Borrowed during region creation. */
  mln_buffer_view geometry MLN_BINDING("encoding=json");
  double min_zoom;
  /**
   * Maximum zoom. Positive infinity follows MapLibre Native behavior and lets
   * each tile source use its own maximum zoom.
   */
  double max_zoom;
  float pixel_ratio;
  bool include_ideographs;
} mln_offline_geometry_region_definition;

/** Offline region definition data. */
typedef union mln_offline_region_definition_data {
  mln_offline_tile_pyramid_region_definition tile_pyramid
    MLN_BINDING("variant=MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID");
  mln_offline_geometry_region_definition geometry
    MLN_BINDING("variant=MLN_OFFLINE_REGION_DEFINITION_GEOMETRY");
} mln_offline_region_definition_data;

/** Tagged offline region definition. */
typedef struct mln_offline_region_definition {
  uint32_t size;
  /** One of mln_offline_region_definition_type. */
  uint32_t type MLN_BINDING("enum=mln_offline_region_definition_type");
  mln_offline_region_definition_data data MLN_BINDING("tag=type");
} mln_offline_region_definition;

/**
 * Region data delivered by an offline completion.
 *
 * The record and every pointer it carries, including the definition's style
 * URL and geometry and the metadata bytes, are borrowed for the duration of the
 * completion callback.
 */
typedef struct mln_offline_region_info {
  mln_offline_region_id id;
  mln_offline_region_definition definition;
  /** Metadata bytes. */
  const uint8_t* metadata MLN_BINDING("length=metadata_size;encoding=bytes");
  size_t metadata_size;
} mln_offline_region_info;

/**
 * Starts creating an offline region.
 *
 * Input strings, GeoJSON geometry bytes, and metadata are copied before this
 * call returns. A successful completion borrows one mln_offline_region_info
 * value, valid only for the duration of the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, definition
 *   is null or malformed, metadata is null with a non-zero metadata_size, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK and one region record.
 * - MLN_STATUS_UNSUPPORTED when the stored definition type is unsupported.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=operation;result=mln_offline_region_info")
MLN_API mln_status mln_runtime_create_offline_region(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata MLN_BINDING("length=metadata_size;encoding=bytes"),
  size_t metadata_size, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts getting one offline region by ID.
 *
 * A successful completion borrows zero or one mln_offline_region_info value,
 * depending on whether the region exists, valid only for the duration of the
 * callback. A missing region completes successfully with no value, as every
 * read of a whole object does; operations on one region complete with
 * MLN_STATUS_NOT_FOUND instead.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK and zero or one region record.
 * - MLN_STATUS_UNSUPPORTED when the stored definition type is unsupported.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=query;result=mln_offline_region_info;nullable=true")
MLN_API mln_status mln_runtime_get_offline_region(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts listing the offline regions in the runtime database.
 *
 * A successful completion borrows value_count mln_offline_region_info values,
 * value_size bytes apart, valid only for the duration of the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK and one record per stored region.
 * - MLN_STATUS_UNSUPPORTED when a stored definition type is unsupported.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=query;result=mln_offline_region_info;shape=array")
MLN_API mln_status mln_runtime_list_offline_regions(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts merging offline regions from another MapLibre offline database.
 *
 * side_database_path must identify an existing, readable MapLibre offline
 * database whose schema version matches this library. The runtime opens the
 * database read-only and leaves its contents unchanged. Keep the file present
 * and unchanged until the completion runs.
 *
 * The merge copies only tiles and resources assigned to a region. It does not
 * accept an MBTiles database, which uses a different schema.
 *
 * The call validates that the database opens read-only and copies
 * side_database_path before it accepts the submission; it does not inspect the
 * schema synchronously. A path that names no existing readable database file is
 * rejected with MLN_STATUS_INVALID_ARGUMENT before acceptance. Format, schema,
 * and later filesystem errors reach the completion as a non-OK status and
 * diagnostic.
 *
 * A successful completion borrows value_count mln_offline_region_info values,
 * value_size bytes apart and one per merged region, valid only for the
 * duration of the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle,
 *   side_database_path has null data with a nonzero size, contains embedded
 *   NUL, or names no readable database file, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK and one record per merged region.
 * - MLN_STATUS_UNSUPPORTED when a merged definition type is unsupported.
 * - MLN_STATUS_NATIVE_ERROR when the merge fails, including a schema mismatch.
 */
MLN_BINDING("execution=operation;result=mln_offline_region_info;shape=array")
MLN_API mln_status mln_runtime_merge_offline_regions(
  mln_runtime runtime, mln_buffer_view side_database_path,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts updating opaque binary metadata for an offline region.
 *
 * Metadata bytes are copied before this call returns. A successful completion
 * borrows one mln_offline_region_info value carrying the updated region, valid
 * only for the duration of the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, metadata is
 *   null with a non-zero metadata_size, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK and one region record.
 * - MLN_STATUS_NOT_FOUND when no region carries region_id.
 * - MLN_STATUS_UNSUPPORTED when the stored definition type is unsupported.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=operation;result=mln_offline_region_info")
MLN_API mln_status mln_runtime_update_offline_region_metadata(
  mln_runtime runtime, mln_offline_region_id region_id,
  const uint8_t* metadata MLN_BINDING("length=metadata_size;encoding=bytes"),
  size_t metadata_size, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts getting the current download status for an offline region.
 *
 * A successful completion borrows one mln_offline_region_status value, valid
 * only for the duration of the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK and one status value.
 * - MLN_STATUS_NOT_FOUND when no region carries region_id.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=query;result=mln_offline_region_status")
MLN_API mln_status mln_runtime_get_offline_region_status(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Enables or disables runtime events for an offline region.
 *
 * Observer callbacks are copied into runtime events. Disabling observation
 * prevents future events for this region and leaves queued events unchanged.
 * The completion reports the terminal status and carries no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK when observation was changed.
 * - MLN_STATUS_NOT_FOUND when no region carries region_id.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_offline_region_observed(
  mln_runtime runtime, mln_offline_region_id region_id, bool observed,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets an offline region's native download state.
 *
 * Register observation separately with
 * mln_runtime_set_offline_region_observed() to receive progress and error
 * events. The completion reports the terminal status and carries no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, state is not
 *   an mln_offline_region_download_state value, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK when the download state was set.
 * - MLN_STATUS_NOT_FOUND when no region carries region_id.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_offline_region_download_state(
  mln_runtime runtime, mln_offline_region_id region_id,
  uint32_t state MLN_BINDING("enum=mln_offline_region_download_state"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Invalidates cached resources for an offline region.
 *
 * The completion reports the terminal status and carries no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK when the region's resources were invalidated.
 * - MLN_STATUS_NOT_FOUND when no region carries region_id.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_invalidate_offline_region(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Deletes an offline region.
 *
 * The completion reports the terminal status and carries no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK when the region was deleted.
 * - MLN_STATUS_NOT_FOUND when no region carries region_id.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_delete_offline_region(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Lower-level Spherical Mercator projected-meter coordinate.
 *
 * Map coordinate conversion APIs use mln_lat_lng. This type is only for
 * Mercator helper functions.
 */
typedef struct mln_projected_meters {
  /** Distance measured northward from the equator, in meters. */
  double northing;
  /** Distance measured eastward from the prime meridian, in meters. */
  double easting;
} mln_projected_meters;

/**
 * MapLibre axonometric rendering options used for snapshots and commands.
 *
 * These control the live map render transform. Geographic coordinates retain
 * their coordinate model.
 */
typedef struct mln_projection_mode {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_projection_mode_field");
  /** Enables a non-perspective axonometric render transform. */
  bool axonometric
    MLN_BINDING("mask=fields;bit=MLN_PROJECTION_MODE_AXONOMETRIC");
  /** Native x-skew factor used by the axonometric transform. */
  double x_skew MLN_BINDING("mask=fields;bit=MLN_PROJECTION_MODE_X_SKEW");
  /** Native y-skew factor used by the axonometric transform. */
  double y_skew MLN_BINDING("mask=fields;bit=MLN_PROJECTION_MODE_Y_SKEW");
} mln_projection_mode;

/** Live map viewport and render-transform controls. */
typedef struct mln_map_viewport_options {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_map_viewport_option_field");
  /** One of mln_north_orientation. */
  uint32_t north_orientation MLN_BINDING(
    "mask=fields;bit=MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION;"
    "enum=mln_north_orientation"
  );
  /** One of mln_constrain_mode. */
  uint32_t constrain_mode MLN_BINDING(
    "mask=fields;bit=MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;"
    "enum=mln_constrain_mode"
  );
  /** One of mln_viewport_mode. */
  uint32_t viewport_mode MLN_BINDING(
    "mask=fields;bit=MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE;"
    "enum=mln_viewport_mode"
  );
  mln_edge_insets frustum_offset
    MLN_BINDING("mask=fields;bit=MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET");
} mln_map_viewport_options;

/**
 * Everything that a projection depends on: the transform that a published
 * snapshot or a rendered frame describes.
 *
 * Every member is set on output. mln_map_projection_create() reads every
 * member and requires every field of each nested struct, except
 * MLN_CAMERA_OPTION_ANCHOR, which it ignores. Camera constraints are map
 * policy and stay out of the view, because no conversion reads them.
 *
 * A later version may append members. mln_map_snapshot and
 * mln_render_frame_result embed the view as their last member, so their own
 * size versions that growth.
 */
typedef struct mln_map_view {
  uint32_t size;
  /**
   * The camera. Its center is unwrapped: the longitude keeps the visible world
   * copy and may fall outside -180 to 180. Every field except
   * MLN_CAMERA_OPTION_ANCHOR is set.
   */
  mln_camera_options camera;
  /**
   * Logical size and device-pixel scale. A snapshot carries the map's scale
   * factor, and a frame carries the scale factor of the target that drew it.
   */
  mln_logical_extent extent;
  /** Axonometric mode and skew, with every field set. */
  mln_projection_mode projection_mode;
  /**
   * North orientation, constrain mode, viewport mode, and frustum offset, with
   * every field set.
   */
  mln_map_viewport_options viewport;
} mln_map_view;

/** Tile prefetch and LOD tuning controls. */
typedef struct mln_map_tile_options {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_map_tile_option_field");
  /** Native uint8_t prefetch zoom delta. */
  uint32_t prefetch_zoom_delta
    MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA");
  double lod_min_radius
    MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS");
  double lod_scale MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_SCALE");
  double lod_pitch_threshold
    MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD");
  double lod_zoom_shift
    MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT");
  /** One of mln_tile_lod_mode. */
  uint32_t lod_mode MLN_BINDING(
    "mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_MODE;enum=mln_tile_lod_mode"
  );
} mln_map_tile_options;
/**
 * Immutable map state copied from the latest published generation.
 *
 * Every field is unkeyed, fixed-size map state that changes only through this
 * map's own commands or through load progress. Each map command that runs
 * publishes a new generation and reports it through its completion, when it has
 * one, even when the command changes nothing or fails, so a snapshot whose
 * generation is at or past a completion observes that command.
 */
typedef struct mln_map_snapshot {
  uint32_t size;
  /** Debug overlay mask of mln_map_debug_option values. */
  uint32_t debug_options MLN_BINDING("enum=mln_map_debug_option");
  uint64_t generation;
  /** True once every requested style and tile resource finished loading. */
  bool fully_loaded;
  bool rendering_stats_view_enabled;
  /**
   * True while the map is inside a gesture.
   *
   * A camera update or delta whose gesture_phase is MLN_GESTURE_PHASE_BEGIN or
   * MLN_GESTURE_PHASE_UPDATE sets it; MLN_GESTURE_PHASE_END and
   * MLN_GESTURE_PHASE_CANCEL clear it.
   */
  bool gesture_in_progress;
  uint64_t event_mask MLN_BINDING("enum=mln_runtime_event_mask");
  /**
   * Generation of the latest render update the map published. A rendered
   * frame at or past it draws map state that includes every command this
   * snapshot observes, though animations and resource loads finish in later
   * frames.
   *
   * A command publishes the render update for its change before it reports
   * its completion. When this snapshot's generation is at or past a command's
   * completion, a frame result with disposition MLN_RENDER_RESULT_RENDERED and
   * a map_update_generation at or past this value was rendered from map state
   * that includes the command. For a camera command with a duration, that
   * state is the transition's first step; the commanded camera appears in the
   * update after the transition's last frame. Resources a command starts
   * loading, such as a style or source URL, appear in later updates as they
   * load.
   */
  uint64_t latest_render_update_generation;
  mln_map_tile_options tile;
  mln_bound_options bounds;
  mln_free_camera_options free_camera;
  /**
   * The map's camera, extent, projection mode, and viewport options.
   * mln_map_projection_create() converts coordinates for this view.
   */
  mln_map_view view;
} mln_map_snapshot;

/** Camera result borrowed for an ordered camera-query completion. */
typedef struct mln_camera_query_result {
  uint64_t generation;
  mln_camera_options camera;
} mln_camera_query_result;

/**
 * Returns map options initialized for this C API version.
 */
MLN_API mln_map_options mln_map_options_default(void) MLN_NOEXCEPT;

/**
 * Starts creation of a map on the runtime.
 *
 * Input is copied before this function returns. The completion value points to
 * one mln_map handle when status is MLN_STATUS_OK.
 *
 * Returns:
 * - MLN_STATUS_OK when the creation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle; options is
 *   null, undersized, or carries an invalid field; options->initial_extent has
 *   a zero width or height, or a scale_factor that is not finite and positive;
 *   or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when map creation fails.
 */
MLN_BINDING("execution=lifecycle;result=mln_map")
MLN_API mln_status mln_runtime_create_map(
  mln_runtime runtime, const mln_map_options* options,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies the map's latest published snapshot.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or out_snapshot
 *   is null or undersized.
 * - MLN_STATUS_INVALID_STATE when map has been released.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("execution=snapshot")
MLN_API mln_status mln_map_get_snapshot(
  mln_map map, mln_map_snapshot* out_snapshot MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Submits the sole post-creation logical extent update.
 *
 * extent.scale_factor must equal the scale factor the map was created with;
 * only width and height may change. A render session sizes the map to its
 * target: attaching a target, replacing it, and mln_render_session_resize()
 * each submit this command with the target's width and height when the call
 * is accepted. An attach that then fails, or a replacement that the driver
 * then refuses, leaves the map at the size that the target named. While a
 * render session is attached, a direct map resize completes with
 * MLN_STATUS_INVALID_STATE; size the map through the session instead.
 * The completion reports terminal disposition and the snapshot generation
 * that the resize published. Resizes apply in acceptance order, so the last
 * one accepted sets the size. A resize to the map's current width and height
 * changes nothing and publishes no render update.
 *
 * Returns:
 * - MLN_STATUS_OK when the resize was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle; extent has a
 *   zero width or height, or a scale_factor that is not finite and positive;
 *   extent.scale_factor differs from the value the map was created with; or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when applying the size throws.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_resize(
  mln_map map, mln_logical_extent extent, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Requests a repaint for a continuous map.
 *
 * A continuous map also publishes a render update on its own when style data,
 * resources, the camera, or a transition changes. A host keeps a frame demand
 * with MLN_FRAME_DEMAND_WAIT_FOR_UPDATE pending and arms the next one as each
 * result arrives to render updates as the map publishes them.
 *
 * Returns:
 * - MLN_STATUS_OK when the request was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released, or map is not
 *   continuous or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_request_repaint(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Begins a command group, which holds this map's render updates until the
 * group ends.
 *
 * Each command publishes at most one render update. A group combines several
 * commands, such as adding a layer and setting its filter, so that no frame
 * shows only some of them. Commands accepted between this call and the
 * matching mln_map_end_command_group() still commit one at a time, and each
 * completion and snapshot reports its own commit. The render updates of those
 * commands, and of loading and transitions meanwhile, stay unpublished, and
 * the end of the outermost group publishes the latest one. Groups nest. A
 * command that fails inside a group leaves the group open and leaves the
 * other commands applied.
 *
 * While a group is open the map publishes no render update, including those
 * that camera changes, gestures, resource loading, and transitions raise, so
 * attached render targets show nothing new. A render-session resize, and a
 * still image already in flight, finish only after the group ends. Submit the
 * commands and the matching end without waiting for any completion in between,
 * and submit the end on every path, including when a command between them
 * fails to submit.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_begin_command_group(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Ends the innermost command group that mln_map_begin_command_group() began.
 *
 * Ending the outermost group publishes the latest render update that the group
 * held, and the snapshot that this command publishes reports that update in
 * latest_render_update_generation.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_STATE when map has no open command group.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_end_command_group(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Submits a copied per-feature-state command.
 *
 * selector->source_id, selector->feature_id, and state are copied before
 * return. state must contain one UTF-8 JSON object and is validated before
 * return. The committed command requests a map repaint.
 *
 * Feature state belongs to the map and applies to the next render update,
 * including the first presented frame that contains the source.
 * mln_map_get_feature_state() reads the map's state.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, selector is null
 *   or invalid, selector lacks MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, state is
 *   empty, invalid JSON, or not an object, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released, or the runtime is
 *   closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  mln_buffer_view state MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered read of per-feature state from this map.
 *
 * selector->source_id and selector->feature_id are copied before return. The
 * read observes every map command accepted before it and copies the map
 * store, not the last rendered frame, so it does not require a render session
 * or a loaded source. The completion borrows UTF-8 bytes holding one JSON
 * object for the callback. Missing feature state is reported as an empty
 * object.
 *
 * Returns:
 * - MLN_STATUS_OK when the read was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, selector is null
 *   or invalid, selector lacks MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released, or the runtime is
 *   closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("execution=query;result=mln_buffer_view;encoding=json")
MLN_API mln_status mln_map_get_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Removes per-feature state from this map.
 *
 * selector->source_id is required. selector->feature_id and selector->state_key
 * are optional. Passing both removes one state key from one feature. Passing
 * only feature_id removes all state for that feature. Passing neither removes
 * all feature state for the source/source-layer. The accepted command requests
 * a map repaint.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, selector is null
 *   or invalid, selector has MLN_FEATURE_STATE_SELECTOR_STATE_KEY without
 *   MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released, or the runtime is
 *   closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_remove_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Requests one still image for a static or tile map.
 *
 * Keep servicing the selected render driver while this request is pending.
 * Submit frame demands with mln_render_session_request_frame() and drain their
 * terminal results with mln_render_session_drain_frame_results(). A
 * caller-graphics-thread driver also requires calls to
 * mln_render_session_service_driver_work() on its graphics thread. Any demand
 * may complete the image, but only with a fully loaded frame that rendered
 * the update this request published or a later one; a frame of an earlier
 * update never does. That frame may be acquired or read back after the
 * completion runs.
 *
 * Returns:
 * - MLN_STATUS_OK when the request was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released, or map is not in
 *   MLN_MAP_MODE_STATIC or MLN_MAP_MODE_TILE, or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_STATE when a still-image request was already pending
 *   when this request ran, or when the map had an open command group.
 * - MLN_STATUS_CANCELLED when the map closes before the image is produced.
 * - MLN_STATUS_NATIVE_ERROR when rendering the image fails.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_map_request_still_image(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a map after synchronous state preflight.
 *
 * A successful call consumes the public handle before returning. The
 * completion runs after previously accepted work is terminal, the native map
 * can no longer call the host, and map-owned callback state has been released.
 * Backend worker and graphics-resource cleanup may continue after completion.
 * The completion state follows the one-shot ownership contract. Teardown may
 * allocate; allocation failure after acceptance terminates the process.
 *
 * Returns:
 * - MLN_STATUS_OK when release was accepted and the handle was consumed.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is already closing
 *   or still has an attached render session.
 * - MLN_STATUS_NATIVE_ERROR when the completion could not be allocated.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_release(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Consumes a map handle without observing its asynchronous retirement.
 *
 * Retirement releases callback state and resources on their required threads.
 * Attached sessions retain the map until they detach or retire. A successful
 * call consumes the handle before returning. A failed call retains caller
 * ownership. Teardown may allocate; allocation failure after acceptance
 * terminates the process.
 *
 * Returns:
 * - MLN_STATUS_OK when the handle was consumed.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle.
 * - MLN_STATUS_INVALID_STATE when map has been released or is already closing.
 */
MLN_API mln_status
mln_map_dispose(mln_map map, mln_diagnostic* out_diagnostic) MLN_NOEXCEPT;

/**
 * Queues a style URL command.
 *
 * The function copies url before returning acceptance. The completion reports
 * committed or failed application. Style loading events continue to report
 * network, decode, and parse results after a committed command.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, url has null
 *   data with a nonzero size or contains embedded NUL, or completion is
 * invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released, or the runtime is
 *   closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_url(
  mln_map map, mln_buffer_view url, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Queues an inline style JSON command.
 *
 * The function copies json before returning acceptance. The completion reports
 * committed or failed application. Style loading events continue to report
 * later resource and parse results after a committed command.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, json.data is
 *   null, json.size is zero, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released, or the runtime is
 *   closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when style parsing or application fails.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_json(
  mln_map map, mln_buffer_view json MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered copy of the last successfully parsed style document.
 *
 * The completion borrows the copied UTF-8 bytes for the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the copy throws.
 */
MLN_BINDING("execution=query;result=mln_buffer_view;encoding=json")
MLN_API mln_status mln_map_get_loaded_style_json(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts an ordered copy of the last requested style URL.
 *
 * The completion borrows the copied UTF-8 bytes for the callback.
 *
 * Returns:
 * - MLN_STATUS_OK when the query is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the copy throws.
 */
MLN_BINDING("execution=query;result=mln_buffer_view")
MLN_API mln_status mln_map_get_style_url(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Selects which map-originated event types this map queues.
 *
 * Filtering an unselected event type allocates no event storage.
 *
 * This call accepts only bits in MLN_RUNTIME_EVENT_MASK_ALL. It reads the bits
 * in MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS and ignores runtime-scoped bits, so
 * MLN_RUNTIME_EVENT_MASK_ALL selects every map-originated type.
 * mln_map_get_snapshot() reports the last committed mask.
 *
 * A map that has not been narrowed selects every map-originated event type this
 * library reports, which covers types a caller's header may not declare. A new
 * mask applies to later events and keeps the events already queued.
 *
 * Select every event type the host reads. These types carry state a host
 * reaches no other way:
 *
 * - MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED and
 *   MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR carry native failure text.
 *
 * Style commands report application failures through their completions
 * regardless of this mask.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is an invalid handle, mask contains an
 *   unknown bit, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_event_mask(
  mln_map map, uint64_t mask MLN_BINDING("enum=mln_runtime_event_mask"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_MAP_H
