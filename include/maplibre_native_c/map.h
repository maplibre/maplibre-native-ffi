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

/** Options used when creating a map. */
typedef struct mln_map_options {
  uint32_t size;
  /**
   * Initial logical map width in UI pixels. Must be positive.
   *
   * Attaching a render session replaces this with the render target extent, and
   * mln_render_session_resize() replaces it again. Until the first attach it is
   * the viewport that camera and projection queries such as
   * mln_map_camera_for_lat_lng_bounds() and mln_map_pixel_for_lat_lng() are
   * answered against.
   */
  uint32_t width;
  /** Initial logical map height in UI pixels. See width. */
  uint32_t height;
  /**
   * UI-to-device pixel scale. Must be positive and finite.
   *
   * Unlike width and height, this is fixed for the lifetime of the map and
   * selects sprites, glyphs, and raster tiles for every frame the map renders.
   * Render targets carry their own scale factor for geometry and shaders, so
   * create the map with the scale factor you intend to render at. A render
   * session attached or resized with a different scale factor logs a warning
   * and renders styled imagery chosen for the map's density.
   */
  double scale_factor;
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
   * Set a narrow mask here to be narrow from the map's first style load, the
   * load that produces the most tile and frame events. A map reports the two
   * camera events of its initial sizing whatever this field selects, because
   * MapLibre resizes the map inside its own constructor.
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

/** Camera fields used for snapshots and camera commands. */
typedef struct mln_camera_options {
  uint32_t size;
  uint32_t fields;
  double latitude;
  double longitude;
  double center_altitude;
  mln_edge_insets padding;
  /**
   * Screen-space focal point for a camera command. This field is input-only.
   *
   * MapLibre Native applies it to camera commands and leaves it out of every
   * camera snapshot it reports, so MLN_CAMERA_OPTION_ANCHOR is set only by the
   * caller. mln_map_get_camera(), mln_map_projection_get_camera(), and the
   * mln_map_camera_for_* family leave it clear.
   */
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
   * When this field is omitted, ease, pan, zoom, rotate, and pitch transitions
   * default to zero and apply instantly, while mln_map_fly_to() derives a
   * duration from velocity instead.
   */
  double duration_ms;
  /**
   * Average flyTo velocity in screenfuls per second. Must be positive.
   * Defaults to 1.2 when omitted. Applies to mln_map_fly_to().
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
   * Each transition emits that event exactly once, whichever way it ends:
   * running to completion, being superseded by a later camera command, being
   * cancelled by mln_map_cancel_transitions(), or completing instantly as a
   * zero-duration jump. A command this API rejects -- one carrying a non-finite
   * enabled camera field, for example -- starts no transition and emits no such
   * event. The event carries no completion reason, so a host that needs to tell
   * completion from cancellation compares the resulting camera against the
   * requested one, or tracks which transition ID is current.
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
 * call returns. Completion is reported through
 * MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED. On successful completion, call
 * mln_runtime_offline_region_create_take_result() to take the snapshot.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, definition is
 *   null or invalid, metadata is null with a non-zero size, or
 *   out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_region_create_start(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size,
  mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Starts getting an offline region snapshot by ID.
 *
 * Completion is reported through MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED.
 * On successful completion, call
 * mln_runtime_offline_region_get_take_result().
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_region_get_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Starts listing offline region snapshots in the runtime database.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_regions_list_start(
  mln_runtime runtime, mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Starts merging offline regions from another database path.
 *
 * The side database may be upgraded in place by native code and must be
 * writable when native merge requires it.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live,
 *   side_database_path is null, or out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_regions_merge_database_start(
  mln_runtime runtime, const char* side_database_path,
  mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Starts updating opaque binary metadata for an offline region.
 *
 * On successful completion, call
 * mln_runtime_offline_region_update_metadata_take_result().
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, metadata is
 *   null with a non-zero size, or out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_region_update_metadata_start(
  mln_runtime runtime, mln_offline_region_id region_id, const uint8_t* metadata,
  size_t metadata_size, mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Starts getting the current completed/download status for an offline region.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_region_get_status_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Enables or disables runtime events for an offline region.
 *
 * Observer callbacks are copied into runtime events. Disabling observation also
 * discards queued events for this region.
 *
 * Completion is reported through MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_region_set_observed_start(
  mln_runtime runtime, mln_offline_region_id region_id, bool observed,
  mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Sets an offline region's native download state.
 *
 * Register observation separately with
 * mln_runtime_offline_region_set_observed_start() to receive progress and error
 * events.
 *
 * Completion is reported through MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, state is not
 *   a mln_offline_region_download_state value, or out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_region_set_download_state_start(
  mln_runtime runtime, mln_offline_region_id region_id, uint32_t state,
  mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Invalidates cached resources for an offline region.
 *
 * Completion is reported through MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_region_invalidate_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Deletes an offline region.
 *
 * Completion is reported through MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_region_delete_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Takes the snapshot result from a completed offline region create operation.
 *
 * Must only be called after the matching
 * mln_runtime_offline_region_create_start() operation has completed
 * successfully (MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED with result
 * status MLN_STATUS_OK). The caller owns the returned snapshot handle and must
 * destroy it with mln_offline_region_snapshot_destroy().
 *
 * On success, the operation entry is consumed. On failure, it remains live so
 * the caller may retry this call or discard the operation with
 * mln_runtime_offline_operation_discard(). Taking or discarding a result also
 * removes that operation's undrained completion event. The thread diagnostic
 * carries a failed operation's error text.
 *
 * Returns:
 * - MLN_STATUS_OK when the result was taken and out_region was set.
 * - MLN_STATUS_INVALID_STATE when the operation has not completed or its result
 *   kind does not match a region snapshot.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or out_region
 *   is null.
 */
MLN_API mln_status mln_runtime_offline_region_create_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot* out_region
) MLN_NOEXCEPT;

/**
 * Takes the snapshot result from a completed offline region get operation.
 *
 * Must only be called after the matching
 * mln_runtime_offline_region_get_start() operation has completed successfully.
 * The caller owns the returned snapshot handle and must destroy it with
 * mln_offline_region_snapshot_destroy().
 *
 * On success, the operation entry is consumed. On failure, it remains live so
 * the caller may retry this call or discard the operation with
 * mln_runtime_offline_operation_discard(). Taking or discarding a result also
 * removes that operation's undrained completion event. The thread diagnostic
 * carries a failed operation's error text.
 *
 * Returns:
 * - MLN_STATUS_OK when the result was taken; out_found indicates whether a
 *   region existed for the requested ID.
 * - MLN_STATUS_INVALID_STATE when the operation has not completed or its result
 *   kind does not match an optional region snapshot.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, out_region is
 *   null, or out_found is null.
 */
MLN_API mln_status mln_runtime_offline_region_get_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot* out_region, bool* out_found
) MLN_NOEXCEPT;

/**
 * Takes the region list from a completed offline regions list operation.
 *
 * Must only be called after the matching
 * mln_runtime_offline_regions_list_start() operation has completed
 * successfully. The caller owns the returned list handle and must destroy it
 * with mln_offline_region_list_destroy().
 *
 * On success, the operation entry is consumed. On failure, it remains live so
 * the caller may retry this call or discard the operation with
 * mln_runtime_offline_operation_discard(). Taking or discarding a result also
 * removes that operation's undrained completion event. The thread diagnostic
 * carries a failed operation's error text.
 *
 * Returns:
 * - MLN_STATUS_OK when the result was taken and out_regions was set.
 * - MLN_STATUS_INVALID_STATE when the operation has not completed or its result
 *   kind does not match a region list.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_regions is null.
 */
MLN_API mln_status mln_runtime_offline_regions_list_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list* out_regions
) MLN_NOEXCEPT;

/**
 * Takes the region list from a completed offline database merge operation.
 *
 * Must only be called after the matching
 * mln_runtime_offline_regions_merge_database_start() operation has completed
 * successfully. The caller owns the returned list handle and must destroy it
 * with mln_offline_region_list_destroy().
 *
 * On success, the operation entry is consumed. On failure, it remains live so
 * the caller may retry this call or discard the operation with
 * mln_runtime_offline_operation_discard(). Taking or discarding a result also
 * removes that operation's undrained completion event. The thread diagnostic
 * carries a failed operation's error text.
 *
 * Returns:
 * - MLN_STATUS_OK when the result was taken and out_regions was set.
 * - MLN_STATUS_INVALID_STATE when the operation has not completed or its result
 *   kind does not match a region list.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_regions is null.
 */
MLN_API mln_status mln_runtime_offline_regions_merge_database_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list* out_regions
) MLN_NOEXCEPT;

/**
 * Takes the snapshot result from a completed offline region update-metadata
 * operation.
 *
 * Must only be called after the matching
 * mln_runtime_offline_region_update_metadata_start() operation has completed
 * successfully. The caller owns the returned snapshot handle and must destroy
 * it with mln_offline_region_snapshot_destroy().
 *
 * On success, the operation entry is consumed. On failure, it remains live so
 * the caller may retry this call or discard the operation with
 * mln_runtime_offline_operation_discard(). Taking or discarding a result also
 * removes that operation's undrained completion event. The thread diagnostic
 * carries a failed operation's error text.
 *
 * Returns:
 * - MLN_STATUS_OK when the result was taken and out_region was set.
 * - MLN_STATUS_INVALID_STATE when the operation has not completed or its result
 *   kind does not match a region snapshot.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or out_region
 *   is null.
 */
MLN_API mln_status mln_runtime_offline_region_update_metadata_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot* out_region
) MLN_NOEXCEPT;

/**
 * Takes the status struct from a completed offline region get-status operation.
 *
 * Must only be called after the matching
 * mln_runtime_offline_region_get_status_start() operation has completed
 * successfully. The caller provides a pre-allocated mln_offline_region_status
 * struct which is filled by this function.
 *
 * On success, the operation entry is consumed. On failure, it remains live so
 * the caller may retry this call or discard the operation with
 * mln_runtime_offline_operation_discard(). Taking or discarding a result also
 * removes that operation's undrained completion event. The thread diagnostic
 * carries a failed operation's error text.
 *
 * Returns:
 * - MLN_STATUS_OK when the result was taken and out_status was filled.
 * - MLN_STATUS_INVALID_STATE when the operation has not completed or its result
 *   kind does not match a region status.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or out_status
 *   is null.
 */
MLN_API mln_status mln_runtime_offline_region_get_status_take_result(
  mln_runtime runtime, mln_offline_operation_id operation_id,
  mln_offline_region_status* out_status
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
 * Returns map options initialized for this C API version.
 */
MLN_API mln_map_options mln_map_options_default(void) MLN_NOEXCEPT;

/**
 * Creates a map handle on the runtime owner thread.
 *
 * On success, the runtime owner thread becomes the map owner thread.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, out_map is
 *   null, *out_map is not null, or options are invalid, which includes an
 *   event_mask bit outside MLN_RUNTIME_EVENT_MASK_ALL.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_create(
  mln_runtime runtime, const mln_map_options* options, mln_map* out_map
) MLN_NOEXCEPT;

/**
 * Copies the map's current logical viewport size and its pixel ratio.
 *
 * The size starts at mln_map_options.width and height, and follows the
 * attach and resize rules documented there. The scale factor is
 * mln_map_options.scale_factor, fixed for the lifetime of the map and
 * independent of any render target's scale factor; compare the two before
 * attaching or resizing a render session to keep them in agreement.
 *
 * This is a state snapshot. All three out-parameters are required.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, or out_width,
 *   out_height, or out_scale_factor is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_get_size(
  mln_map map, uint32_t* out_width, uint32_t* out_height,
  double* out_scale_factor
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
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live.
 * - MLN_STATUS_INVALID_STATE when map is not in MLN_MAP_MODE_CONTINUOUS.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_request_repaint(mln_map map) MLN_NOEXCEPT;

/**
 * Requests one still image for a static or tile map.
 *
 * Pump the runtime and drain runtime events for this map until
 * MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED or
 * MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED is reported. Those two types are the
 * only completion reports, so select both in the map's event mask. While the
 * request is pending, process each
 * MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE event from this map. Render
 * targets use mln_render_session_render_update(). Surface targets present
 * directly. A render-update
 * call can report a result other than MLN_RENDER_RESULT_RENDERED before the
 * next update is available; keep pumping and draining in that case. After
 * MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED, use the latest successful texture
 * update when the host needs image bytes or a backend texture.
 *
 * Returns:
 * - MLN_STATUS_OK when the request was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live.
 * - MLN_STATUS_INVALID_STATE when map is not in MLN_MAP_MODE_STATIC or
 *   MLN_MAP_MODE_TILE, or when a still-image request is already pending.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_request_still_image(mln_map map) MLN_NOEXCEPT;

/**
 * Destroys a map handle on its owner thread.
 *
 * The map must not have an attached render session.
 *
 * Destruction also discards this map's queued events, including queued style
 * loading failures. There is no flush and no terminal event, so the last state
 * a host mirrored from events can stay behind the map's final state. Snapshot
 * whatever state the host needs while the map is still live, and let teardown
 * proceed without awaiting an event for this map. A batch that a host already
 * drained holds copies, so it stays readable after this call.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not a live map handle.
 * - MLN_STATUS_INVALID_STATE when map still has an attached render target
 *   session.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_destroy(mln_map map) MLN_NOEXCEPT;

/**
 * Loads a style URL through MapLibre Native style APIs.
 *
 * This is a map command. The return status reports synchronous acceptance or
 * failure. Later native success and failure are reported through runtime
 * events. A URL that is unreachable, malformed, or serves invalid style
 * content is still accepted synchronously; every such failure arrives later as
 * a style loading-failed event, so hosts report style URL errors from the event
 * stream rather than from this return status.
 *
 * Returns:
 * - MLN_STATUS_OK when the load request was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null, not live, or url is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when a synchronous native error is reported or an
 *   internal exception is converted to status.
 */
MLN_API mln_status
mln_map_set_style_url(mln_map map, const char* url) MLN_NOEXCEPT;

/**
 * Loads inline style JSON through MapLibre Native style APIs.
 *
 * This is a map command. The return status reports synchronous acceptance or
 * failure. Later native success and failure are reported through runtime
 * events. Malformed JSON can fail synchronously and still enqueue a
 * loading-failed event.
 *
 * Returns:
 * - MLN_STATUS_OK when the load request was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null, not live, or json is empty or
 *   has a nonzero size with a null data pointer.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when a synchronous native error is reported or an
 *   internal exception is converted to status.
 */
MLN_API mln_status
mln_map_set_style_json(mln_map map, mln_buffer_view json) MLN_NOEXCEPT;

/**
 * Copies the style document this map's style was last parsed from.
 *
 * This is a state snapshot of the loaded document, not a serialization of the
 * live style. The bytes are the document the style loader last parsed
 * successfully: the bytes passed to mln_map_set_style_json(), or the response
 * body fetched for mln_map_set_style_url(). Runtime mutations through the
 * style APIs, such as adding a layer or setting a paint property, do not change
 * it, and a failed parse leaves the previously parsed document in place.
 *
 * A copy of the document is byte-for-byte identical to the bytes that were
 * passed to mln_map_set_style_json(), so a host may hand it back to that
 * function unchanged.
 *
 * out_json may be null only when json_capacity is 0, which is a size probe that
 * reports the required length and succeeds. *out_json_size receives the byte
 * length before the capacity is checked, so a caller learns the size from a
 * call that could not fit the document. The bytes are not null-terminated, so
 * an exact-length buffer is sufficient.
 *
 * A reported size of 0 means no document has been parsed: no style has been
 * loaded yet, or every load so far failed to parse. A parsed document is never
 * empty.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including a size probe.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, out_json is null
 *   with non-zero capacity, json_capacity is too small for a non-null buffer,
 *   or out_json_size is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_loaded_style_json(
  mln_map map, uint8_t* out_json, size_t json_capacity, size_t* out_json_size
) MLN_NOEXCEPT;

/**
 * Copies the URL this map's style was last requested from.
 *
 * Unlike mln_map_copy_loaded_style_json(), this is live rather than load-time
 * state: mln_map_set_style_url() records the URL when the request is made,
 * before the response arrives or the document parses, and
 * mln_map_set_style_json() clears it. The document reports what was last parsed
 * while the URL reports what was last requested, so the two can disagree while
 * a load is in flight or after one fails.
 *
 * out_url may be null only when url_capacity is 0, which is a size probe that
 * reports the required length and succeeds. *out_url_size receives the byte
 * length before the capacity is checked. The bytes are not null-terminated, so
 * an exact-length buffer is sufficient.
 *
 * A reported size of 0 means no URL bytes are available. That covers a style
 * loaded from inline JSON, a map that has loaded no style, and a URL load
 * requested with an empty string, which mln_map_set_style_url() accepts. These
 * cases are not distinguishable through this entry point.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including a size probe.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not live, out_url is null
 *   with non-zero capacity, url_capacity is too small for a non-null buffer, or
 *   out_url_size is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_map_copy_style_url(
  mln_map map, char* out_url, size_t url_capacity, size_t* out_url_size
) MLN_NOEXCEPT;

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
 * mln_map_get_event_mask() reports the value last set, so a host reads it,
 * changes one bit, and writes it back.
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
 *   MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED are the only reports that a
 *   still-image request finished. See mln_map_request_still_image().
 * - MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED carries the
 *   transition identity a caller set on an animation, and
 *   MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE distinguishes a completed
 *   transition from a cancelled one. See mln_animation_options.transition_id.
 * - MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED and
 *   MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR carry native failure text.
 *
 * mln_map_set_style_url() and mln_map_set_style_json() report a style failure
 * that MapLibre raises inside the call through their return status and a thread
 * diagnostic, whatever this mask selects.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not a live map handle, or
 *   mask holds a bit outside MLN_RUNTIME_EVENT_MASK_ALL.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_map_set_event_mask(mln_map map, uint64_t mask) MLN_NOEXCEPT;

/**
 * Reports which map-originated event types this map queues.
 *
 * The value is the mask last set, including bits outside
 * MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS that this map ignores. A map that has
 * not been narrowed reports MLN_RUNTIME_EVENT_MASK_ALL as this library defines
 * it.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when map is null or not a live map handle, or
 *   out_mask is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the map owner
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_map_get_event_mask(mln_map map, uint64_t* out_mask) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_MAP_H
