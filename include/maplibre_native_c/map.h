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
typedef enum mln_camera_option_field : uint32_t {
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
typedef enum mln_animation_option_field : uint32_t {
  MLN_ANIMATION_OPTION_DURATION = 1U << 0U,
  MLN_ANIMATION_OPTION_VELOCITY = 1U << 1U,
  MLN_ANIMATION_OPTION_MIN_ZOOM = 1U << 2U,
  MLN_ANIMATION_OPTION_EASING = 1U << 3U,
  MLN_ANIMATION_OPTION_TRANSITION_ID = 1U << 4U,
} mln_animation_option_field;

/** Field mask values for mln_camera_fit_options. */
typedef enum mln_camera_fit_option_field : uint32_t {
  MLN_CAMERA_FIT_OPTION_PADDING = 1U << 0U,
  MLN_CAMERA_FIT_OPTION_BEARING = 1U << 1U,
  MLN_CAMERA_FIT_OPTION_PITCH = 1U << 2U,
} mln_camera_fit_option_field;

/** Field mask values for mln_bound_options. */
typedef enum mln_bound_option_field : uint32_t {
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
typedef enum mln_free_camera_option_field : uint32_t {
  MLN_FREE_CAMERA_OPTION_POSITION = 1U << 0U,
  MLN_FREE_CAMERA_OPTION_ORIENTATION = 1U << 1U,
} mln_free_camera_option_field;

/** Field mask values for MapLibre axonometric rendering options. */
typedef enum mln_projection_mode_field : uint32_t {
  MLN_PROJECTION_MODE_AXONOMETRIC = 1U << 0U,
  MLN_PROJECTION_MODE_X_SKEW = 1U << 1U,
  MLN_PROJECTION_MODE_Y_SKEW = 1U << 2U,
} mln_projection_mode_field;

/** Debug overlay mask values for mln_map_set_debug_options(). */
typedef enum mln_map_debug_option : uint32_t {
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
typedef enum mln_map_viewport_option_field : uint32_t {
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
typedef enum mln_map_tile_option_field : uint32_t {
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
/** Logical map extent in UI pixels and device-pixel scale. */
typedef struct mln_logical_extent {
  uint32_t width;
  uint32_t height;
  double scale_factor;
} mln_logical_extent;

/** Options used when creating a map. */
typedef struct mln_map_options {
  uint32_t size;
  /**
   * Initial logical extent. Width and height must be positive. The scale
   * factor must be positive and finite.
   *
   * After creation, mln_map_resize() is the only function that changes this
   * extent.
   */
  mln_logical_extent initial_extent;
  /** One of mln_map_mode. Defaults to MLN_MAP_MODE_CONTINUOUS. */
  uint32_t map_mode;
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
   * This field is always read, so set it explicitly.
   * MLN_RUNTIME_EVENT_MASK_ALL selects every event type this library reports
   * and is the value mln_map_options_default() populates.
   * MLN_RUNTIME_EVENT_MASK_NONE queues none.
   *
   * The mask applies throughout construction, including the camera events that
   * MapLibre reports while it initializes the map's size.
   */
  uint64_t event_mask;
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

/** Camera fields used by snapshots and camera updates. */
typedef struct mln_camera_options {
  uint32_t size;
  uint32_t fields;
  double latitude;
  double longitude;
  double center_altitude;
  mln_edge_insets padding;
  /** Optional screen-space focal point in logical map pixels. */
  mln_screen_point anchor;
  double zoom;
  double bearing;
  double pitch;
  double roll;
  double field_of_view;
} mln_camera_options;

/** Cubic easing curve for animated camera transitions. */
typedef struct mln_unit_bezier {
  double x1;
  double y1;
  double x2;
  double y2;
} mln_unit_bezier;

/** Optional animation controls for camera transitions. */
typedef struct mln_animation_options {
  uint32_t size;
  uint32_t fields;
  /**
   * Duration in milliseconds. Must be finite and non-negative. Values that
   * would overflow MapLibre Native's internal duration are invalid.
   *
   * When omitted, ease transitions apply immediately. Fly transitions derive
   * their duration from velocity.
   */
  double duration_ms;
  /**
   * Average fly velocity in screenfuls per second. Must be positive and
   * defaults to 1.2 when omitted.
   */
  double velocity;
  /** Peak zoom for flyTo transitions. */
  double min_zoom;
  mln_unit_bezier easing;
  /**
   * Caller-chosen identity for the transition this options struct starts.
   *
   * When MLN_ANIMATION_OPTION_TRANSITION_ID is set, the transition emits one
   * MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED event carrying this value
   * in its mln_runtime_event_camera_transition_finished payload. The C API
   * passes the value through without interpreting it, so callers pick their own
   * scheme, such as a monotonically increasing counter.
   *
   * Each transition emits that event exactly once when it completes, is
   * superseded, is cancelled, or applies instantly. The event carries no
   * completion reason, so a host that needs to distinguish outcomes compares
   * the resulting camera against the requested one.
   *
   * The event is queued on the runtime that owns the map and is drained by
   * mln_runtime_drain_events(). For a transition that runs to completion, it is
   * queued immediately before that transition's
   * MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE event. A map reports the terminal
   * outcome only while its event mask selects
   * MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED.
   *
   * When this field is omitted, the transition emits no such event.
   */
  uint64_t transition_id;
} mln_animation_options;

/** Relative camera operation carried by mln_camera_delta. */
typedef enum mln_camera_delta_kind : uint32_t {
  MLN_CAMERA_DELTA_MOVE = 0,
  MLN_CAMERA_DELTA_SCALE = 1,
  MLN_CAMERA_DELTA_BEARING = 2,
  MLN_CAMERA_DELTA_PITCH = 3,
} mln_camera_delta_kind;

/**
 * One relative camera operation.
 *
 * MOVE reads offset. SCALE reads amount as a positive factor. BEARING and
 * PITCH read amount as degrees. SCALE and BEARING apply anchor when has_anchor
 * is true. Every operation reads animation.
 */
typedef struct mln_camera_delta {
  uint32_t size;
  uint32_t kind;
  mln_screen_point offset;
  double amount;
  bool has_anchor;
  mln_screen_point anchor;
  mln_animation_options animation;
} mln_camera_delta;

/** Camera transition behavior for mln_camera_update. */
typedef enum mln_camera_update_mode : uint32_t {
  MLN_CAMERA_UPDATE_MODE_JUMP = 0,
  MLN_CAMERA_UPDATE_MODE_EASE = 1,
  MLN_CAMERA_UPDATE_MODE_FLY = 2,
} mln_camera_update_mode;

/** Gesture boundary carried atomically with a camera update. */
typedef enum mln_gesture_phase : uint32_t {
  MLN_GESTURE_PHASE_NONE = 0,
  MLN_GESTURE_PHASE_BEGIN = 1,
  MLN_GESTURE_PHASE_UPDATE = 2,
  MLN_GESTURE_PHASE_END = 3,
  MLN_GESTURE_PHASE_CANCEL = 4,
} mln_gesture_phase;

/**
 * One atomic absolute camera update.
 *
 * The command copies this struct before returning.
 */
typedef struct mln_camera_update {
  uint32_t size;
  uint32_t mode;
  mln_camera_options camera;
  mln_animation_options animation;
  uint32_t gesture_phase;
  uint32_t reserved;
} mln_camera_update;

/** Optional fitting controls for camera-for-viewport queries. */
typedef struct mln_camera_fit_options {
  uint32_t size;
  uint32_t fields;
  mln_edge_insets padding;
  double bearing;
  double pitch;
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
  uint32_t fields;
  mln_vec3 position;
  mln_quaternion orientation;
} mln_free_camera_options;

/** Geographic coordinate in degrees used by map and projection APIs. */
typedef struct mln_lat_lng {
  /** Latitude in degrees. Input latitude must be finite and within [-90, 90].
   */
  double latitude;
  /** Longitude in degrees. Input longitude must be finite. */
  double longitude;
} mln_lat_lng;

/** Optional fields for mln_feature_state_selector. */
typedef enum mln_feature_state_selector_field : uint32_t {
  MLN_FEATURE_STATE_SELECTOR_SOURCE_LAYER_ID = 1U << 0U,
  MLN_FEATURE_STATE_SELECTOR_FEATURE_ID = 1U << 1U,
  MLN_FEATURE_STATE_SELECTOR_STATE_KEY = 1U << 2U,
} mln_feature_state_selector_field;

/** Feature-state source, feature, and key selector. */
typedef struct mln_feature_state_selector {
  uint32_t size;
  uint32_t fields;
  /** Source ID. Required and borrowed for the duration of the call. */
  mln_buffer_view source_id;
  /** Optional source layer ID. Required for vector-source disambiguation. */
  mln_buffer_view source_layer_id;
  /** Optional feature ID string. Required by set/get and optional for remove.
   */
  mln_buffer_view feature_id;
  /** Optional state key. Used only by remove and requires feature_id. */
  mln_buffer_view state_key;
} mln_feature_state_selector;

/** Geographic bounds in degrees. */
typedef struct mln_lat_lng_bounds {
  mln_lat_lng southwest;
  mln_lat_lng northeast;
} mln_lat_lng_bounds;

/** Optional map camera constraint fields. */
typedef struct mln_bound_options {
  uint32_t size;
  uint32_t fields;
  /** Read when fields contains MLN_BOUND_OPTION_BOUNDS. */
  mln_lat_lng_bounds bounds;
  double min_zoom;
  double max_zoom;
  double min_pitch;
  double max_pitch;
} mln_bound_options;

/** Tile-pyramid offline region definition. */
typedef struct mln_offline_tile_pyramid_region_definition {
  uint32_t size;
  /** Style URL. Copied during region creation. */
  const char* style_url;
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
  uint32_t size;
  /** Style URL. Copied during region creation. */
  const char* style_url;
  /** UTF-8 GeoJSON Geometry bytes. Borrowed during region creation. */
  mln_buffer_view geometry;
  double min_zoom;
  /**
   * Maximum zoom. Positive infinity follows MapLibre Native behavior and lets
   * each tile source use its own maximum zoom.
   */
  double max_zoom;
  float pixel_ratio;
  bool include_ideographs;
} mln_offline_geometry_region_definition;

/** Tagged offline region definition. */
typedef struct mln_offline_region_definition {
  uint32_t size;
  /** One of mln_offline_region_definition_type. */
  uint32_t type;
  union {
    mln_offline_tile_pyramid_region_definition tile_pyramid;
    mln_offline_geometry_region_definition geometry;
  } data;
} mln_offline_region_definition;

/** Region data view returned from a snapshot or list handle. */
typedef struct mln_offline_region_info {
  uint32_t size;
  mln_offline_region_id id;
  mln_offline_region_definition definition;
  /** Metadata bytes. Valid until the owner snapshot/list is destroyed.
   */
  const uint8_t* metadata;
  size_t metadata_size;
} mln_offline_region_info;

/**
 * Starts creating an offline region.
 *
 * Input strings, GeoJSON geometry bytes, and metadata are copied before this
 * call returns. A successful completion borrows one
 * mln_offline_region_snapshot value.
 */
MLN_API mln_status mln_runtime_offline_region_create(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts getting an offline region snapshot by ID.
 *
 * A successful completion borrows zero or one mln_offline_region_snapshot
 * value, depending on whether the region exists.
 */
MLN_API mln_status mln_runtime_offline_region_get(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts listing offline region snapshots in the runtime database.
 *
 * A successful completion borrows one mln_offline_region_list value.
 */
MLN_API mln_status mln_runtime_offline_regions_list(
  mln_runtime runtime, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts merging offline regions from another database path.
 *
 * The side database may be upgraded in place by native code and must be
 * writable when native merge requires it. A successful completion borrows one
 * mln_offline_region_list value.
 */
MLN_API mln_status mln_runtime_offline_regions_merge_database(
  mln_runtime runtime, const char* side_database_path,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts updating opaque binary metadata for an offline region.
 *
 * A successful completion borrows one mln_offline_region_snapshot value.
 */
MLN_API mln_status mln_runtime_offline_region_update_metadata(
  mln_runtime runtime, mln_offline_region_id region_id, const uint8_t* metadata,
  size_t metadata_size, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts getting the current download status for an offline region.
 *
 * A successful completion borrows one mln_offline_region_status value.
 */
MLN_API mln_status mln_runtime_offline_region_get_status(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Enables or disables runtime events for an offline region.
 *
 * Observer callbacks are copied into runtime events. Disabling observation
 * prevents future events for this region and leaves queued events unchanged.
 * The completion reports the terminal status.
 */
MLN_API mln_status mln_runtime_offline_region_set_observed(
  mln_runtime runtime, mln_offline_region_id region_id, bool observed,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Sets an offline region's native download state.
 *
 * Register observation separately with
 * mln_runtime_offline_region_set_observed() to receive progress and error
 * events. The completion reports the terminal status.
 */
MLN_API mln_status mln_runtime_offline_region_set_download_state(
  mln_runtime runtime, mln_offline_region_id region_id, uint32_t state,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Invalidates cached resources for an offline region.
 *
 * The completion reports the terminal status.
 */
MLN_API mln_status mln_runtime_offline_region_invalidate(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Deletes an offline region.
 *
 * The completion reports the terminal status.
 */
MLN_API mln_status mln_runtime_offline_region_delete(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Copies a region data view out of a snapshot handle.
 *
 * On success, out_info receives pointers into snapshot-owned storage. Those
 * pointers remain valid until the snapshot is destroyed.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when snapshot is null or not live, out_info is
 *   null, or out_info->size is too small.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_offline_region_snapshot_get(
  mln_offline_region_snapshot snapshot, mln_offline_region_info* out_info
) MLN_NOEXCEPT;

/** Destroys an offline region snapshot handle. Null is accepted as a no-op. */
MLN_API void mln_offline_region_snapshot_destroy(
  mln_offline_region_snapshot snapshot
) MLN_NOEXCEPT;

/**
 * Gets the number of regions in a list handle.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when list is null or not live, or out_count is
 *   null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_offline_region_list_count(
  mln_offline_region_list list, size_t* out_count
) MLN_NOEXCEPT;

/**
 * Copies a region data view for one list entry.
 *
 * On success, out_info receives pointers into list-owned storage. Those
 * pointers remain valid until the list is destroyed.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when list is null or not live, index is out of
 *   range, out_info is null, or out_info->size is too small.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_offline_region_list_get(
  mln_offline_region_list list, size_t index, mln_offline_region_info* out_info
) MLN_NOEXCEPT;

/** Destroys an offline region list handle. Null is accepted as a no-op. */
MLN_API void mln_offline_region_list_destroy(
  mln_offline_region_list list
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
 * MapLibre Native names this native type ProjectionMode. It controls the live
 * map render transform, not the geographic coordinate model.
 */
typedef struct mln_projection_mode {
  uint32_t size;
  uint32_t fields;
  /** Enables a non-perspective axonometric render transform. */
  bool axonometric;
  /** Native x-skew factor used by the axonometric transform. */
  double x_skew;
  /** Native y-skew factor used by the axonometric transform. */
  double y_skew;
} mln_projection_mode;

/** Live map viewport and render-transform controls. */
typedef struct mln_map_viewport_options {
  uint32_t size;
  uint32_t fields;
  /** One of mln_north_orientation. */
  uint32_t north_orientation;
  /** One of mln_constrain_mode. */
  uint32_t constrain_mode;
  /** One of mln_viewport_mode. */
  uint32_t viewport_mode;
  mln_edge_insets frustum_offset;
} mln_map_viewport_options;

/** Tile prefetch and LOD tuning controls. */
typedef struct mln_map_tile_options {
  uint32_t size;
  uint32_t fields;
  /** Native uint8_t prefetch zoom delta. */
  uint32_t prefetch_zoom_delta;
  double lod_min_radius;
  double lod_scale;
  double lod_pitch_threshold;
  double lod_zoom_shift;
  /** One of mln_tile_lod_mode. */
  uint32_t lod_mode;
} mln_map_tile_options;
/**
 * Immutable map state copied from the latest published generation.
 *
 * Every field is unkeyed, fixed-size map state that changes only through this
 * map's own commands or through load progress. Each committed map command
 * publishes a new generation and reports it through its completion, so a
 * snapshot whose generation is at or past a completion observes that commit.
 */
typedef struct mln_map_snapshot {
  uint32_t size;
  /** Debug overlay mask of mln_map_debug_option values. */
  uint32_t debug_options;
  uint64_t generation;
  mln_camera_options camera;
  mln_logical_extent logical_extent;
  mln_projection_mode projection_mode;
  mln_map_viewport_options viewport;
  /** True once every requested style and tile resource finished loading. */
  bool fully_loaded;
  bool rendering_stats_view_enabled;
  bool repaint_demand;
  uint8_t reserved_flags;
  uint64_t event_mask;
  uint64_t latest_render_update_generation;
  mln_map_tile_options tile;
  mln_bound_options bounds;
  mln_free_camera_options free_camera;
} mln_map_snapshot;

/** Camera result borrowed for an ordered camera-query completion. */
typedef struct mln_camera_query_result {
  uint32_t size;
  uint32_t reserved;
  uint64_t generation;
  mln_camera_options camera;
} mln_camera_query_result;

/**
 * Returns map options initialized for this C API version.
 */
MLN_API mln_map_options mln_map_options_default(void) MLN_NOEXCEPT;

/**
 * Creates a map on the runtime worker.
 *
 * Input is copied before this function returns. The completion value points to
 * one mln_map handle when status is MLN_STATUS_OK.
 */
MLN_API mln_status mln_map_create(
  mln_runtime runtime, const mln_map_options* options,
  const mln_completion* completion
) MLN_NOEXCEPT;

/** Copies the latest immutable state published by the map worker. */
MLN_API mln_status
mln_map_snapshot_get(mln_map map, mln_map_snapshot* out_snapshot) MLN_NOEXCEPT;

/**
 * Submits the sole post-creation logical extent update.
 *
 * The completion reports terminal disposition and the snapshot generation
 * published by a committed resize.
 */
MLN_API mln_status mln_map_resize(
  mln_map map, mln_logical_extent extent, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Requests a repaint for a continuous map.
 *
 * Continuous maps also invalidate automatically when style data, resources,
 * camera, or transitions change. Ask attached render targets to process the
 * latest update when mln_runtime_drain_events() reports
 * MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE. That type is the map's only
 * invalidation report, so select it in the event mask of every rendered map.
 * Repaint requests do not produce
 * MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED or
 * MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED events.
 *
 * Returns:
 * - MLN_STATUS_OK when the request was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is not live or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when map is not continuous or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_map_request_repaint(
  mln_map map, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Submits a copied per-feature-state command.
 *
 * selector->source_id, selector->feature_id, and state are copied before
 * return. state must contain one UTF-8 JSON object and is validated before
 * return. The committed command requests a map repaint.
 *
 * Feature state belongs to the map. A render session pushes it into the
 * renderer on the next render update, including the first presented frame that
 * contains the source. mln_map_get_feature_state copies this map store, not
 * the last rendered frame.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, selector is
 *   null or invalid, selector lacks MLN_FEATURE_STATE_SELECTOR_FEATURE_ID,
 *   state is empty, invalid JSON, or not an object, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_map_set_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  mln_buffer_view state, const mln_completion* completion
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, selector is
 *   null or invalid, selector lacks MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, selector is
 *   null or invalid, selector has MLN_FEATURE_STATE_SELECTOR_STATE_KEY
 *   without MLN_FEATURE_STATE_SELECTOR_FEATURE_ID, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_map_remove_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Requests one still image for a static or tile map.
 *
 * Keep servicing the selected render driver while this request is pending.
 * Submit frame demands with mln_render_session_request_frame() and drain their
 * terminal results with mln_render_session_drain_frame_results(). A
 * caller-graphics-thread driver also requires calls to
 * mln_render_session_service_driver_work() on its graphics thread. A rendered
 * frame may be acquired or read back after the completion runs.
 *
 * Returns:
 * - MLN_STATUS_OK when the request was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live.
 * - MLN_STATUS_INVALID_STATE when map is not in MLN_MAP_MODE_STATIC or
 *   MLN_MAP_MODE_TILE, or when a still-image request is already pending.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_request_still_image(
  mln_map map, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Releases a map after synchronous state preflight.
 *
 * A successful call consumes the public handle before returning. The
 * completion runs after previously accepted work is terminal, the native map
 * can no longer call the host, and map-owned callback state has been released.
 * Backend worker and graphics-resource cleanup may continue after completion.
 * The completion state follows the one-shot ownership contract.
 *
 * Returns:
 * - MLN_STATUS_OK when release was accepted and the handle was consumed.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, or completion is
 *   invalid.
 * - MLN_STATUS_INVALID_STATE when the map is already closing or still has an
 *   attached render session.
 * - MLN_STATUS_NATIVE_ERROR when teardown could not be scheduled.
 */
MLN_API mln_status
mln_map_release(mln_map map, const mln_completion* completion) MLN_NOEXCEPT;

/**
 * Queues a style URL command.
 *
 * The function copies url before returning acceptance. The completion reports
 * committed or failed application. Style loading events continue to report
 * network, decode, and parse results after a committed command.
 *
 * Returns:
 * - MLN_STATUS_OK when the command was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is not live, url is null, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_map_set_style_url(
  mln_map map, const char* url, const mln_completion* completion
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is not live, json is invalid, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_map_set_style_json(
  mln_map map, mln_buffer_view json, const mln_completion* completion
) MLN_NOEXCEPT;

/**
 * Starts an ordered copy of the last successfully parsed style document.
 *
 * The completion borrows the copied UTF-8 bytes for the callback.
 */
MLN_API mln_status mln_map_loaded_style_json(
  mln_map map, const mln_completion* completion
) MLN_NOEXCEPT;

/** Starts an ordered copy of the last requested style URL. */
MLN_API mln_status
mln_map_style_url(mln_map map, const mln_completion* completion) MLN_NOEXCEPT;

/**
 * Selects which map-originated event types this map queues.
 *
 * MapLibre Native reports map state through the observer callbacks behind these
 * events, and this mask decides which of them become queued events. An event of
 * an unselected type is never built and never queued, so it reaches no batch
 * and raises no wake flag.
 *
 * This call reads the bits in MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS and ignores
 * the rest, so MLN_RUNTIME_EVENT_MASK_ALL selects every map-originated type.
 * mln_map_snapshot_get() reports the last committed mask.
 *
 * A map that has not been narrowed selects every map-originated event type this
 * library reports, which covers types a caller's header may not declare. A new
 * mask applies to later events and keeps the events already queued.
 *
 * One unread render-update event covers every invalidation queued behind it,
 * compared against the queue tail. Leaving out a type that used to arrive
 * between two render updates makes those two updates adjacent, so they coalesce
 * into one.
 *
 * Select every event type the host reads. These types carry state a host
 * reaches no other way:
 *
 * - MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE is the map's only
 *   invalidation report. See mln_map_request_repaint().
 * - MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED and
 *   MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED report observer completion
 *   in addition to the still-image completion.
 * - MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED carries the
 *   transition identity a caller set on an animation, and
 *   MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE distinguishes a completed
 *   transition from a cancelled one. See mln_animation_options.transition_id.
 * - MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED and
 *   MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR carry native failure text.
 *
 * Style commands report application failures through their completions
 * regardless of this mask.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is not live, mask contains an unknown
 *   bit, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when the map is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_map_set_event_mask(
  mln_map map, uint64_t mask, const mln_completion* completion
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_MAP_H
