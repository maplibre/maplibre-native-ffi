/**
 * @file maplibre_native_c/map.h
 * Maps, the types that describe their camera and view, and offline regions.
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

/** The fields that mln_camera_options sets. */
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

/** A set of mln_camera_option_field bits. */
typedef uint32_t mln_camera_option_fields;

/** The fields that mln_animation_options sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_animation_option_field : uint32_t {
  MLN_ANIMATION_OPTION_DURATION = 1U << 0U,
  MLN_ANIMATION_OPTION_VELOCITY = 1U << 1U,
  MLN_ANIMATION_OPTION_MIN_ZOOM = 1U << 2U,
  MLN_ANIMATION_OPTION_EASING = 1U << 3U,
  MLN_ANIMATION_OPTION_TRANSITION_ID = 1U << 4U,
  MLN_ANIMATION_OPTION_MAX_DURATION = 1U << 5U,
} mln_animation_option_field;

/** A set of mln_animation_option_field bits. */
typedef uint32_t mln_animation_option_fields;

/** The fields that mln_camera_fit_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_camera_fit_option_field : uint32_t {
  MLN_CAMERA_FIT_OPTION_PADDING = 1U << 0U,
  MLN_CAMERA_FIT_OPTION_BEARING = 1U << 1U,
  MLN_CAMERA_FIT_OPTION_PITCH = 1U << 2U,
} mln_camera_fit_option_field;

/** A set of mln_camera_fit_option_field bits. */
typedef uint32_t mln_camera_fit_option_fields;

/** The fields that mln_bound_options sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_bound_option_field : uint32_t {
  MLN_BOUND_OPTION_BOUNDS = 1U << 0U,
  MLN_BOUND_OPTION_MIN_ZOOM = 1U << 1U,
  MLN_BOUND_OPTION_MAX_ZOOM = 1U << 2U,
  MLN_BOUND_OPTION_MIN_PITCH = 1U << 3U,
  MLN_BOUND_OPTION_MAX_PITCH = 1U << 4U,
  /**
   * Removes the area that limits the camera's center. It can't be combined
   * with MLN_BOUND_OPTION_BOUNDS.
   */
  MLN_BOUND_OPTION_UNBOUNDED = 1U << 5U,
} mln_bound_option_field;

/** A set of mln_bound_option_field bits. */
typedef uint32_t mln_bound_option_fields;

/** The fields that mln_free_camera_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_free_camera_option_field : uint32_t {
  MLN_FREE_CAMERA_OPTION_POSITION = 1U << 0U,
  MLN_FREE_CAMERA_OPTION_ORIENTATION = 1U << 1U,
} mln_free_camera_option_field;

/** A set of mln_free_camera_option_field bits. */
typedef uint32_t mln_free_camera_option_fields;

/** The fields that mln_projection_mode_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_projection_mode_option_field : uint32_t {
  MLN_PROJECTION_MODE_OPTION_AXONOMETRIC = 1U << 0U,
  MLN_PROJECTION_MODE_OPTION_X_SKEW = 1U << 1U,
  MLN_PROJECTION_MODE_OPTION_Y_SKEW = 1U << 2U,
} mln_projection_mode_option_field;

/** A set of mln_projection_mode_option_field bits. */
typedef uint32_t mln_projection_mode_option_fields;

/** A debug overlay that the map draws. */
typedef enum MLN_BINDING("kind=bitmask") mln_map_debug_flag : uint32_t {
  MLN_MAP_DEBUG_TILE_BORDERS = 1U << 1U,
  MLN_MAP_DEBUG_PARSE_STATUS = 1U << 2U,
  MLN_MAP_DEBUG_TIMESTAMPS = 1U << 3U,
  MLN_MAP_DEBUG_COLLISION = 1U << 4U,
  MLN_MAP_DEBUG_OVERDRAW = 1U << 5U,
  MLN_MAP_DEBUG_STENCIL_CLIP = 1U << 6U,
  MLN_MAP_DEBUG_DEPTH_BUFFER = 1U << 7U,
} mln_map_debug_flag;

/** A set of mln_map_debug_flag bits. */
typedef uint32_t mln_map_debug_flags;

/** The screen direction that north points to when the bearing is 0. */
typedef enum mln_north_orientation : uint32_t {
  MLN_NORTH_ORIENTATION_UP = 0,
  MLN_NORTH_ORIENTATION_RIGHT = 1,
  MLN_NORTH_ORIENTATION_DOWN = 2,
  MLN_NORTH_ORIENTATION_LEFT = 3,
} mln_north_orientation;

/** How the map keeps the camera within the world or within its bounds. */
typedef enum mln_constrain_mode : uint32_t {
  MLN_CONSTRAIN_MODE_NONE = 0,
  /** Keeps the world filling the view from top to bottom. */
  MLN_CONSTRAIN_MODE_HEIGHT_ONLY = 1,
  /** Keeps the world filling the view in both directions. */
  MLN_CONSTRAIN_MODE_WIDTH_AND_HEIGHT = 2,
  /** Keeps the whole view inside the bounds from mln_map_set_bounds(). */
  MLN_CONSTRAIN_MODE_SCREEN = 3,
} mln_constrain_mode;

/** The vertical direction of the image that the map renders. */
typedef enum mln_viewport_mode : uint32_t {
  MLN_VIEWPORT_MODE_DEFAULT = 0,
  /** Renders the image upside down. */
  MLN_VIEWPORT_MODE_FLIPPED_Y = 1,
} mln_viewport_mode;

/** The fields that mln_map_viewport_options sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_map_viewport_option_field : uint32_t {
  MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION = 1U << 0U,
  MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE = 1U << 1U,
  MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE = 1U << 2U,
  MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET = 1U << 3U,
} mln_map_viewport_option_field;

/** A set of mln_map_viewport_option_field bits. */
typedef uint32_t mln_map_viewport_option_fields;

/** How the map picks the level of detail of its tiles. */
typedef enum mln_tile_lod_mode : uint32_t {
  /** Uses the most detail at the center of the screen. */
  MLN_TILE_LOD_MODE_DEFAULT = 0,
  /** Uses the most detail closest to the camera. */
  MLN_TILE_LOD_MODE_DISTANCE = 1,
} mln_tile_lod_mode;

/** The fields that mln_map_tile_options sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_map_tile_option_field : uint32_t {
  MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA = 1U << 0U,
  MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS = 1U << 1U,
  MLN_MAP_TILE_OPTION_LOD_SCALE = 1U << 2U,
  MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD = 1U << 3U,
  MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT = 1U << 4U,
  MLN_MAP_TILE_OPTION_LOD_MODE = 1U << 5U,
} mln_map_tile_option_field;

/** A set of mln_map_tile_option_field bits. */
typedef uint32_t mln_map_tile_option_fields;

/** When a map renders. */
typedef enum mln_map_mode : uint32_t {
  /** Renders whenever the map changes. */
  MLN_MAP_MODE_CONTINUOUS = 0,
  /** Renders only when mln_map_request_still_image() asks for an image. */
  MLN_MAP_MODE_STATIC = 1,
  /** Renders still images of single tiles, as MLN_MAP_MODE_STATIC does. */
  MLN_MAP_MODE_TILE = 2,
} mln_map_mode;

/** A size in logical pixels, and the physical pixels in each logical pixel. */
typedef struct mln_logical_extent {
  uint32_t width;
  uint32_t height;
  double scale_factor;
} mln_logical_extent;

/** A size in logical pixels, which the map's scale factor turns into pixels. */
typedef struct mln_logical_size {
  uint32_t width;
  uint32_t height;
} mln_logical_size;

/** The fields that mln_map_options sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_map_option_field : uint32_t {
  MLN_MAP_OPTION_EVENT_MASK = 1U << 0U,
  MLN_MAP_OPTION_TILE_CACHE_ENABLED = 1U << 1U,
} mln_map_option_field;

/** A set of mln_map_option_field bits. */
typedef uint32_t mln_map_option_fields;

/** Options for mln_runtime_create_map(). */
typedef struct mln_map_options {
  uint32_t size;
  mln_map_option_fields fields;
  /**
   * The map's first size, and its scale factor. Render sessions of the map
   * render at its scale factor. mln_map_resize() changes both, or
   * mln_render_session_resize() while a render session is attached.
   */
  mln_logical_extent initial_extent MLN_BINDING("required=true");
  mln_map_mode map_mode;
  /**
   * Whether the map decodes MapLibre Tiles (MLT) that use FastPFOR integer
   * encoding. Without it, such tiles fail to load.
   */
  bool fast_pfor_enabled;
  /** The events that the map reports. Without it, the map reports them all. */
  mln_runtime_event_flags event_mask
    MLN_BINDING("mask=fields;bit=MLN_MAP_OPTION_EVENT_MASK");
  /**
   * Whether the map keeps tiles in memory after they leave the view, so they
   * show again without loading. Without it, the map keeps them.
   */
  bool tile_cache_enabled
    MLN_BINDING("mask=fields;bit=MLN_MAP_OPTION_TILE_CACHE_ENABLED");
  /**
   * Fonts for the text that a style's own fonts don't draw, such as scripts
   * that need complex shaping, like Myanmar or Devanagari. A JSON array of font
   * files, each an object with a `url` and an optional `unicode-range`, as in
   * a font face of a style's `font-faces`, which every font stack uses after
   * the style's own font faces. Empty for none.
   */
  mln_buffer_view fallback_font_faces;
} mln_map_options;

/** A point on the screen, in logical pixels from the map's top-left corner. */
typedef struct mln_screen_point {
  double x;
  double y;
} mln_screen_point;

/** Distances in logical pixels from each edge of the map. */
typedef struct mln_edge_insets {
  double top;
  double left;
  double bottom;
  double right;
} mln_edge_insets;

/** A geographic coordinate, in degrees. */
typedef struct mln_lat_lng {
  double latitude;
  double longitude;
} mln_lat_lng;

/** A camera, with only the members that `fields` selects. */
typedef struct mln_camera_options {
  uint32_t size;
  mln_camera_option_fields fields;
  mln_lat_lng center MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_CENTER");
  /** The altitude of the center, in meters above sea level. */
  double center_altitude
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_CENTER_ALTITUDE");
  /** Insets from the map's edges. The camera centers within the rest. */
  mln_edge_insets padding
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_PADDING");
  /**
   * The screen point that a change to the zoom, bearing, or pitch keeps in
   * place. A `center` overrides it.
   */
  mln_screen_point anchor
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_ANCHOR");
  double zoom MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_ZOOM");
  /** The rotation from north, in degrees. */
  double bearing MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_BEARING");
  /** The tilt toward the horizon, in degrees. 0 looks straight down. */
  double pitch MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_PITCH");
  /** The roll, in degrees. */
  double roll MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_ROLL");
  /** The vertical field of view, in degrees. */
  double field_of_view MLN_BINDING("mask=fields;bit=MLN_CAMERA_OPTION_FOV");
} mln_camera_options;

/** A map's whole camera. */
typedef struct mln_camera {
  mln_lat_lng center;
  /** The altitude of the center, in meters above sea level. */
  double center_altitude;
  /** Insets from the map's edges. The camera centers within the rest. */
  mln_edge_insets padding;
  double zoom;
  /** The rotation from north, in degrees. */
  double bearing;
  /** The tilt toward the horizon, in degrees. 0 looks straight down. */
  double pitch;
  /** The roll, in degrees. */
  double roll;
  /** The vertical field of view, in degrees. */
  double field_of_view;
} mln_camera MLN_BINDING("options=mln_camera_options");

// Helpers for C callers. Each binding generates its own, so the generator
// skips these.
#ifndef MLN_BINDGEN

/**
 * Makes camera options that set every member of `camera`.
 *
 * The options don't set `anchor`.
 */
static inline mln_camera_options mln_camera_to_options(mln_camera camera) {
  mln_camera_options options;
  options.size = sizeof(options);
  options.fields = MLN_CAMERA_OPTION_CENTER |
                   MLN_CAMERA_OPTION_CENTER_ALTITUDE |
                   MLN_CAMERA_OPTION_PADDING | MLN_CAMERA_OPTION_ZOOM |
                   MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
                   MLN_CAMERA_OPTION_ROLL | MLN_CAMERA_OPTION_FOV;
  options.center = camera.center;
  options.center_altitude = camera.center_altitude;
  options.padding = camera.padding;
  options.anchor.x = 0.0;
  options.anchor.y = 0.0;
  options.zoom = camera.zoom;
  options.bearing = camera.bearing;
  options.pitch = camera.pitch;
  options.roll = camera.roll;
  options.field_of_view = camera.field_of_view;
  return options;
}

#endif  // MLN_BINDGEN

/**
 * A timing curve from (0, 0) to (1, 1), with the control points (`x1`, `y1`)
 * and (`x2`, `y2`).
 *
 * `x1` and `x2` are between 0 and 1.
 */
typedef struct mln_unit_bezier {
  double x1;
  double y1;
  double x2;
  double y2;
} mln_unit_bezier;

/**
 * How a camera change ended.
 *
 * A change that ends in more than one way reports
 * MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED if any of it was cancelled, and
 * otherwise MLN_CAMERA_TRANSITION_OUTCOME_REPLACED if any of it was replaced.
 */
typedef enum mln_camera_transition_outcome : uint32_t {
  /** The camera finished the change. */
  MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED = 0,
  /**
   * mln_map_cancel_camera_transitions(),
   * mln_map_cancel_camera_transitions_with_id(), a gesture's
   * MLN_GESTURE_PHASE_BEGIN or MLN_GESTURE_PHASE_CANCEL,
   * mln_map_set_free_camera_options(), or a resize that moved the camera back
   * inside its bounds stopped the change. A command that fails before it moves
   * the camera also reports this.
   */
  MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED = 1,
  /** The map closed before the change ended. */
  MLN_CAMERA_TRANSITION_OUTCOME_CLOSED = 2,
  /**
   * A later camera change set every field that a part of the change was still
   * animating, so that part stopped. Its other parts keep animating, and the
   * change reports this outcome when they end, unless one was cancelled.
   *
   * A later change that sets only some of those fields leaves the change
   * animating the rest. A fly's center and zoom count as one field, and so do
   * an anchored change's center and the fields that the anchor holds in place.
   */
  MLN_CAMERA_TRANSITION_OUTCOME_REPLACED = 3,
} mln_camera_transition_outcome;

/** How fast each part of a camera change moves. */
typedef struct mln_camera_velocity {
  /**
   * How fast the map's content moves on the screen, in logical pixels per
   * second.
   */
  mln_screen_point pan;
  /** In zoom levels per second. */
  double zoom;
  /** In degrees per second. */
  double bearing;
  /** In degrees per second. */
  double pitch;
} mln_camera_velocity;

/** The end of a camera change. */
typedef struct mln_camera_transition_end {
  uint32_t size;
  mln_camera_transition_outcome outcome;
  /**
   * The generation of the first map snapshot that shows where the change left
   * the camera, or 0 when the map closed first.
   */
  uint64_t generation;
  /**
   * How fast a gesture's momentum still moved when it was cancelled, which the
   * next gesture can add to its own release velocities. Zero for every other
   * end.
   */
  mln_camera_velocity velocity;
} mln_camera_transition_end;

/**
 * Receives the end of a camera change.
 *
 * Lifetimes:
 * `end` stays valid until the callback returns.
 */
typedef void (*mln_camera_transition_end_callback)(
  void* user_data, const mln_camera_transition_end* end
);

/**
 * A callback for the end of a camera change, for mln_animation_options.
 *
 * The callback runs once for each camera command, after every part of the
 * change that the command started has ended.
 */
typedef struct mln_camera_transition_handler {
  mln_camera_transition_end_callback callback MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_camera_transition_handler MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** How a camera change animates. */
typedef struct mln_animation_options {
  mln_animation_option_fields fields;
  double duration_ms
    MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_DURATION");
  /**
   * The average speed of a fly animation, in screens per second.
   * `duration_ms` overrides it.
   */
  double velocity MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_VELOCITY");
  /** The zoom level at the peak of a fly animation. */
  double min_zoom MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_MIN_ZOOM");
  /**
   * The longest a fly animation may take. A fly animation whose duration, set
   * or derived from `velocity`, is longer moves the camera at once instead,
   * and so does one too slow for the library to time, without this field.
   * Other animations ignore it.
   */
  double max_duration_ms
    MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_MAX_DURATION");
  mln_unit_bezier easing
    MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_EASING");
  /**
   * An ID that mln_map_cancel_camera_transitions_with_id() cancels the change
   * by. Several changes can share one.
   */
  uint64_t transition_id
    MLN_BINDING("mask=fields;bit=MLN_ANIMATION_OPTION_TRANSITION_ID");
  mln_camera_transition_handler end_handler;
} mln_animation_options;

/** How a camera command moves the camera. */
typedef enum mln_camera_update_mode : uint32_t {
  /** Moves the camera at once. */
  MLN_CAMERA_UPDATE_MODE_JUMP = 0,
  /** Animates the camera straight to the target. */
  MLN_CAMERA_UPDATE_MODE_EASE = 1,
  /** Zooms out, travels to the target, and zooms back in. */
  MLN_CAMERA_UPDATE_MODE_FLY = 2,
} mln_camera_update_mode;

/**
 * Where a camera change falls in a gesture that the user makes.
 *
 * MLN_GESTURE_PHASE_BEGIN and MLN_GESTURE_PHASE_UPDATE mark a gesture in
 * progress, and MLN_GESTURE_PHASE_END and MLN_GESTURE_PHASE_CANCEL end it.
 */
typedef enum mln_gesture_phase : uint32_t {
  /** The change isn't part of a gesture. */
  MLN_GESTURE_PHASE_NONE = 0,
  /**
   * The gesture starts. This cancels every running camera animation before
   * the change applies.
   */
  MLN_GESTURE_PHASE_BEGIN = 1,
  /**
   * The gesture continues. To start a gesture that keeps the running camera
   * animations, start it with this phase instead of MLN_GESTURE_PHASE_BEGIN.
   */
  MLN_GESTURE_PHASE_UPDATE = 2,
  MLN_GESTURE_PHASE_END = 3,
  /**
   * The platform cancelled the gesture. This also cancels every running camera
   * animation after the change applies.
   */
  MLN_GESTURE_PHASE_CANCEL = 4,
} mln_gesture_phase;

/** How a part of a gesture's momentum slows from its release speed. */
typedef enum mln_momentum_decay_model : uint32_t {
  /**
   * The speed halves every `half_life_ms`, until it drops to `stop_speed`.
   * UIScrollView's normal deceleration is a half-life of 346 ms, and its fast
   * deceleration 69 ms.
   */
  MLN_MOMENTUM_DECAY_MODEL_FRICTION = 0,
  /**
   * The part eases `distance` over `duration_ms`, along `easing`, as a camera
   * animation does. The caller plans the glide, for example from the
   * platform's own fling physics, such as Android's OverScroller.
   */
  MLN_MOMENTUM_DECAY_MODEL_EASE = 1,
} mln_momentum_decay_model;

/** The fields that mln_momentum_decay sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_momentum_decay_field : uint32_t {
  MLN_MOMENTUM_DECAY_MODEL = 1U << 0U,
  MLN_MOMENTUM_DECAY_HALF_LIFE_MS = 1U << 1U,
  MLN_MOMENTUM_DECAY_STOP_SPEED = 1U << 2U,
  MLN_MOMENTUM_DECAY_DISTANCE = 1U << 3U,
  MLN_MOMENTUM_DECAY_DURATION_MS = 1U << 4U,
  MLN_MOMENTUM_DECAY_EASING = 1U << 5U,
  MLN_MOMENTUM_DECAY_MIN_SPEED = 1U << 6U,
  MLN_MOMENTUM_DECAY_MAX_SPEED = 1U << 7U,
  MLN_MOMENTUM_DECAY_MAX_DISTANCE = 1U << 8U,
} mln_momentum_decay_field;

/** A set of mln_momentum_decay_field bits. */
typedef uint32_t mln_momentum_decay_fields;

/**
 * How one part of a gesture's momentum slows from its release speed.
 *
 * Speeds and distances are in the part's unit: logical pixels for pan, zoom
 * levels for zoom, and degrees for bearing and pitch. A setting whose bit
 * isn't set takes the part's default, which mln_gesture_momentum lists.
 * `min_speed` is finite and not negative, and every other number is finite
 * and positive. A friction decay can serve every release, as a preset that
 * matches a platform's scrolling. An eased glide takes its direction from the
 * part's velocity, and its distance and duration from the caller's plan.
 */
typedef struct mln_momentum_decay {
  mln_momentum_decay_fields fields;
  mln_momentum_decay_model model
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_MODEL");
  /** For MLN_MOMENTUM_DECAY_MODEL_FRICTION, how long the speed takes to halve.
   */
  double half_life_ms
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_HALF_LIFE_MS");
  /** For MLN_MOMENTUM_DECAY_MODEL_FRICTION, the speed at which it ends. */
  double stop_speed
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_STOP_SPEED");
  /** Required for MLN_MOMENTUM_DECAY_MODEL_EASE. */
  double distance MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_DISTANCE");
  /** Required for MLN_MOMENTUM_DECAY_MODEL_EASE. */
  double duration_ms
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_DURATION_MS");
  /**
   * For MLN_MOMENTUM_DECAY_MODEL_EASE, the share of `distance` covered over
   * the share of `duration_ms`. Its control points are between 0 and 1, so the
   * glide never passes its distance. Defaults to (0, 0, 0.25, 1), the curve
   * that camera animations ease along by default. Android's scroll fling
   * follows (0.175, 0.5, 0.35, 1).
   */
  mln_unit_bezier easing
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_EASING");
  /** A slower release starts no momentum. */
  double min_speed MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_MIN_SPEED");
  /** For MLN_MOMENTUM_DECAY_MODEL_FRICTION, a faster release counts as this. */
  double max_speed MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_MAX_SPEED");
  /**
   * The farthest the part moves. A friction release that would move farther
   * starts slower, so it ends sooner, and a planned glide moves at most this
   * far over its duration.
   */
  double max_distance
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_DECAY_MAX_DISTANCE");
} mln_momentum_decay;

/** The fields that mln_pan_momentum sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_pan_momentum_field : uint32_t {
  MLN_PAN_MOMENTUM_VELOCITY = 1U << 0U,
} mln_pan_momentum_field;

/** A set of mln_pan_momentum_field bits. */
typedef uint32_t mln_pan_momentum_fields;

/** How a gesture's pan keeps moving the map's content after release. */
typedef struct mln_pan_momentum {
  mln_pan_momentum_fields fields;
  /**
   * How fast the map's content moved on the screen at release, in logical
   * pixels per second. The pan moves only when this is set.
   */
  mln_screen_point velocity
    MLN_BINDING("mask=fields;bit=MLN_PAN_MOMENTUM_VELOCITY");
  mln_momentum_decay decay;
} mln_pan_momentum;

/** The fields that mln_momentum_snap sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_momentum_snap_field : uint32_t {
  MLN_MOMENTUM_SNAP_SPACING = 1U << 0U,
  MLN_MOMENTUM_SNAP_OFFSET = 1U << 1U,
  MLN_MOMENTUM_SNAP_TOLERANCE = 1U << 2U,
  MLN_MOMENTUM_SNAP_DURATION_MS = 1U << 3U,
} mln_momentum_snap_field;

/** A set of mln_momentum_snap_field bits. */
typedef uint32_t mln_momentum_snap_fields;

/**
 * Where a part settles after a gesture that changed it.
 *
 * The targets are every `spacing` from `offset`, in the part's unit. Bearing
 * targets wrap around the circle. Once the part's glide ends, or right away
 * when it doesn't move, the part eases to the nearest target if it is within
 * `tolerance` of it, and otherwise stays where it is. A setting whose bit
 * isn't set takes the part's default, which mln_gesture_momentum lists.
 */
typedef struct mln_momentum_snap {
  mln_momentum_snap_fields fields;
  /** Positive. */
  double spacing MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_SNAP_SPACING");
  /** Defaults to 0. */
  double offset MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_SNAP_OFFSET");
  /** Not negative. */
  double tolerance MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_SNAP_TOLERANCE");
  /** Zero settles without easing. Defaults to 300. */
  double duration_ms
    MLN_BINDING("mask=fields;bit=MLN_MOMENTUM_SNAP_DURATION_MS");
} mln_momentum_snap;

/** The fields that mln_anchored_momentum sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_anchored_momentum_field : uint32_t {
  MLN_ANCHORED_MOMENTUM_VELOCITY = 1U << 0U,
  MLN_ANCHORED_MOMENTUM_ANCHOR = 1U << 1U,
  MLN_ANCHORED_MOMENTUM_SNAP = 1U << 2U,
} mln_anchored_momentum_field;

/** A set of mln_anchored_momentum_field bits. */
typedef uint32_t mln_anchored_momentum_fields;

/**
 * How a gesture's zoom, bearing, or pitch keeps changing after release.
 */
typedef struct mln_anchored_momentum {
  mln_anchored_momentum_fields fields;
  /**
   * How fast the value changed at release: zoom levels per second for zoom,
   * and degrees per second for bearing and pitch. The part moves only when
   * this is set.
   */
  double velocity MLN_BINDING("mask=fields;bit=MLN_ANCHORED_MOMENTUM_VELOCITY");
  /**
   * The screen point that the change keeps in place. Without one, the change
   * keeps the center of the view in place.
   */
  mln_screen_point anchor
    MLN_BINDING("mask=fields;bit=MLN_ANCHORED_MOMENTUM_ANCHOR");
  mln_momentum_decay decay;
  /**
   * Where the part settles. Without this, it stays where the gesture and its
   * glide leave it.
   */
  mln_momentum_snap snap
    MLN_BINDING("mask=fields;bit=MLN_ANCHORED_MOMENTUM_SNAP");
} mln_anchored_momentum;

/**
 * How a gesture keeps moving the camera after the user lets go.
 *
 * A part moves only when its velocity is set, and each part slows on its own.
 * Every part defaults to MLN_MOMENTUM_DECAY_MODEL_FRICTION with a half-life of
 * 140 ms, and these other defaults:
 *
 * - Pan stops at 10 logical pixels per second, needs a release of at least
 *   200, and counts a release faster than 10000 as 10000.
 * - Zoom stops at 0.01 levels per second and moves at most 2.5 levels. Its
 *   snap settles on the nearest whole level.
 * - Bearing stops at 1 degree per second and needs a release of at least 60.
 *   Its snap settles on north within 7 degrees.
 * - Pitch stops at 1 degree per second and needs a release of at least 5. Its
 *   snap settles flat within 7 degrees.
 */
typedef struct mln_gesture_momentum {
  mln_pan_momentum pan;
  mln_anchored_momentum zoom;
  mln_anchored_momentum bearing;
  mln_anchored_momentum pitch;
} mln_gesture_momentum;

/** The fields that mln_camera_delta sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_camera_delta_field : uint32_t {
  MLN_CAMERA_DELTA_OFFSET = 1U << 0U,
  MLN_CAMERA_DELTA_SCALE = 1U << 1U,
  MLN_CAMERA_DELTA_BEARING = 1U << 2U,
  MLN_CAMERA_DELTA_PITCH = 1U << 3U,
  MLN_CAMERA_DELTA_ANCHOR = 1U << 4U,
  MLN_CAMERA_DELTA_MOMENTUM = 1U << 5U,
} mln_camera_delta_field;

/** A set of mln_camera_delta_field bits. */
typedef uint32_t mln_camera_delta_fields;

/** A change to the camera from where it is now, for gestures. */
typedef struct mln_camera_delta {
  uint32_t size;
  mln_camera_delta_fields fields;
  /** Moves the map's content this far on the screen, as a drag does. */
  mln_screen_point offset
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_OFFSET");
  /** Multiplies the map's scale. 2 zooms in by one level. */
  double scale MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_SCALE");
  /** Degrees to add to the bearing. */
  double bearing MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_BEARING");
  /** Degrees to add to the pitch. */
  double pitch MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_PITCH");
  /**
   * The screen point that `scale`, `bearing`, and `pitch` keep in place. It
   * needs one of them, and it combines with `offset` only in a delta without
   * a duration.
   */
  mln_screen_point anchor
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_ANCHOR");
  /**
   * How the gesture keeps moving after this delta. Only a delta in the
   * MLN_GESTURE_PHASE_END phase carries it.
   */
  mln_gesture_momentum momentum
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_DELTA_MOMENTUM");
  mln_animation_options animation;
  mln_gesture_phase gesture_phase;
  uint32_t reserved MLN_BINDING("kind=reserved");
} mln_camera_delta;

/**
 * A camera change for mln_map_update_camera().
 *
 * A jump uses only the end handler of `animation`, and the handler runs as
 * soon as the camera moves.
 */
typedef struct mln_camera_update {
  uint32_t size;
  mln_camera_update_mode mode;
  mln_camera_options camera;
  mln_animation_options animation;
  mln_gesture_phase gesture_phase;
  uint32_t reserved MLN_BINDING("kind=reserved");
} mln_camera_update;

/** Options for finding the camera that fits an area. */
typedef struct mln_camera_fit_options {
  uint32_t size;
  mln_camera_fit_option_fields fields;
  /**
   * Space to leave between the area and each edge of the map. The left and
   * right insets must add up to less than the map's width, and the top and
   * bottom to less than its height.
   */
  mln_edge_insets padding
    MLN_BINDING("mask=fields;bit=MLN_CAMERA_FIT_OPTION_PADDING");
  /** The rotation from north, in degrees. */
  double bearing MLN_BINDING("mask=fields;bit=MLN_CAMERA_FIT_OPTION_BEARING");
  /** The tilt toward the horizon, in degrees. */
  double pitch MLN_BINDING("mask=fields;bit=MLN_CAMERA_FIT_OPTION_PITCH");
} mln_camera_fit_options;

/**
 * A camera change that fits an area, for mln_map_fit_camera_to_lat_lng_bounds()
 * and the other fit commands.
 *
 * A jump uses only the end handler of `animation`, as in mln_camera_update.
 */
typedef struct mln_camera_fit_update {
  uint32_t size;
  mln_camera_update_mode mode;
  mln_camera_fit_options fit;
  mln_animation_options animation;
} mln_camera_fit_update;

/** A vector in three dimensions. */
typedef struct mln_vec3 {
  double x;
  double y;
  double z;
} mln_vec3;

/** A rotation, as a quaternion. */
typedef struct mln_quaternion {
  double x;
  double y;
  double z;
  double w;
} mln_quaternion;

/** The camera's position and orientation in three dimensions. */
typedef struct mln_free_camera_options {
  uint32_t size;
  mln_free_camera_option_fields fields;
  /**
   * The camera's position in Web Mercator units, where the world is 1 wide,
   * (0, 0) is its northwest corner, and (1, 1) is its southeast corner. `z` is
   * the altitude in the same units.
   */
  mln_vec3 position
    MLN_BINDING("mask=fields;bit=MLN_FREE_CAMERA_OPTION_POSITION");
  /**
   * The camera's rotation from looking straight down with north at the top of
   * the screen.
   */
  mln_quaternion orientation
    MLN_BINDING("mask=fields;bit=MLN_FREE_CAMERA_OPTION_ORIENTATION");
} mln_free_camera_options;

/**
 * A map camera's whole position and orientation in three dimensions.
 *
 * Its members mean the same as those of mln_free_camera_options.
 */
typedef struct mln_free_camera {
  mln_vec3 position;
  mln_quaternion orientation;
} mln_free_camera;

/** The type of an mln_feature_id. */
typedef enum mln_feature_id_type : uint32_t {
  MLN_FEATURE_ID_TYPE_STRING = 1,
  MLN_FEATURE_ID_TYPE_UINT = 2,
  MLN_FEATURE_ID_TYPE_INT = 3,
  MLN_FEATURE_ID_TYPE_DOUBLE = 4,
} mln_feature_id_type;

/** The string or number of an mln_feature_id. */
typedef union mln_feature_id_data {
  mln_buffer_view string_value
    MLN_BINDING("variant=MLN_FEATURE_ID_TYPE_STRING");
  uint64_t uint_value MLN_BINDING("variant=MLN_FEATURE_ID_TYPE_UINT");
  int64_t int_value MLN_BINDING("variant=MLN_FEATURE_ID_TYPE_INT");
  /** A finite number. */
  double double_value MLN_BINDING("variant=MLN_FEATURE_ID_TYPE_DOUBLE");
} mln_feature_id_data;

/**
 * The ID of a feature in a source, as the source's data gives it.
 *
 * The map compares feature IDs as text. A number names the same feature as
 * the string of its shortest decimal form, so 7, 7.0, and "7" name one
 * feature.
 */
typedef struct mln_feature_id {
  mln_feature_id_type type;
  mln_feature_id_data data MLN_BINDING("tag=type");
} mln_feature_id;

/** The fields that mln_feature_state_selector sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_feature_state_selector_field : uint32_t {
  MLN_FEATURE_STATE_SELECTOR_FEATURE_ID = 1U << 0U,
} mln_feature_state_selector_field;

/** A set of mln_feature_state_selector_field bits. */
typedef uint32_t mln_feature_state_selector_fields;

/** The features whose state to set, read, or remove. */
typedef struct mln_feature_state_selector {
  uint32_t size;
  mln_feature_state_selector_fields fields;
  mln_buffer_view source_id MLN_BINDING("required=true");
  /** The source layer, for a vector source. Empty for other sources. */
  mln_buffer_view source_layer_id;
  /**
   * The feature's ID. Setting or reading state needs it. Without it,
   * mln_map_remove_feature_state() removes the state of every feature in the
   * source layer.
   */
  mln_feature_id feature_id
    MLN_BINDING("mask=fields;bit=MLN_FEATURE_STATE_SELECTOR_FEATURE_ID");
  /**
   * The one key that mln_map_remove_feature_state() removes. It needs
   * `feature_id`. Empty removes every key.
   */
  mln_buffer_view state_key;
} mln_feature_state_selector;

/** A rectangle of coordinates, from its southwest to its northeast corner. */
typedef struct mln_lat_lng_bounds {
  mln_lat_lng southwest;
  mln_lat_lng northeast;
} mln_lat_lng_bounds;

/** Limits on the camera, for mln_map_set_bounds(). */
typedef struct mln_bound_options {
  uint32_t size;
  mln_bound_option_fields fields;
  /** The area that the camera's center stays within. */
  mln_lat_lng_bounds bounds
    MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_BOUNDS");
  double min_zoom MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_MIN_ZOOM");
  double max_zoom MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_MAX_ZOOM");
  /** The lowest pitch, in degrees. */
  double min_pitch MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_MIN_PITCH");
  /** The highest pitch, in degrees. */
  double max_pitch MLN_BINDING("mask=fields;bit=MLN_BOUND_OPTION_MAX_PITCH");
} mln_bound_options;

/**
 * All the limits on a map's camera.
 *
 * Its members mean the same as those of mln_bound_options.
 */
typedef struct mln_camera_bounds {
  /**
   * The area that the camera's center stays within. When `unbounded` is true,
   * it covers the whole world.
   */
  mln_lat_lng_bounds bounds;
  /** Whether no area limits the camera's center. */
  bool unbounded;
  double min_zoom;
  double max_zoom;
  /** The lowest pitch, in degrees. */
  double min_pitch;
  /** The highest pitch, in degrees. */
  double max_pitch;
} mln_camera_bounds;

/** An offline region that covers a rectangle of the map. */
typedef struct mln_offline_tile_pyramid_region_definition {
  /**
   * The style whose resources the region holds. Maps use the region when they
   * load the same style URL.
   */
  mln_buffer_view style_url;
  mln_lat_lng_bounds bounds;
  double min_zoom;
  /** INFINITY covers every zoom level that the style's sources have. */
  double max_zoom;
  /** The pixel ratio of the images to download, such as 2 for dense screens. */
  float pixel_ratio;
  /** Whether the region holds the glyphs of CJK ideographs. */
  bool include_ideographs;
} mln_offline_tile_pyramid_region_definition;

/**
 * An offline region that covers a GeoJSON geometry.
 *
 * The members other than `geometry` mean the same as those of
 * mln_offline_tile_pyramid_region_definition.
 */
typedef struct mln_offline_geometry_region_definition {
  mln_buffer_view style_url;
  mln_buffer_view geometry MLN_BINDING("encoding=json;required=true");
  double min_zoom;
  double max_zoom;
  float pixel_ratio;
  bool include_ideographs;
} mln_offline_geometry_region_definition;

/** The rectangle or geometry of an offline region definition. */
typedef union mln_offline_region_definition_data {
  mln_offline_tile_pyramid_region_definition tile_pyramid
    MLN_BINDING("variant=MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID");
  mln_offline_geometry_region_definition geometry
    MLN_BINDING("variant=MLN_OFFLINE_REGION_DEFINITION_GEOMETRY");
} mln_offline_region_definition_data;

/** The area, zoom range, and style that an offline region covers. */
typedef struct mln_offline_region_definition {
  uint32_t size;
  mln_offline_region_definition_type type;
  mln_offline_region_definition_data data MLN_BINDING("tag=type");
} mln_offline_region_definition;

/** An offline region in the runtime's cache file. */
typedef struct mln_offline_region_info {
  mln_offline_region_id id;
  mln_offline_region_definition definition;
  const uint8_t* metadata MLN_BINDING("length=metadata_size;encoding=bytes");
  size_t metadata_size;
} mln_offline_region_info;

/**
 * Creates an offline region in the runtime's cache file.
 *
 * `metadata` holds any bytes to store with the region, such as its name. The
 * region doesn't download until mln_runtime_set_offline_region_download_state()
 * makes its download active.
 */
MLN_BINDING("execution=operation;result=mln_offline_region_info")
MLN_API mln_status mln_runtime_create_offline_region(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata MLN_BINDING("length=metadata_size;encoding=bytes"),
  size_t metadata_size, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the offline region `region_id`.
 *
 * The result is null when the cache file has no region with that ID.
 */
MLN_BINDING("execution=query;result=mln_offline_region_info;nullable=true")
MLN_API mln_status mln_runtime_get_offline_region(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Lists the offline regions in the runtime's cache file. */
MLN_BINDING("execution=query;result=mln_offline_region_info;shape=array")
MLN_API mln_status mln_runtime_list_offline_regions(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies the offline regions of another cache file into the runtime's cache
 * file.
 *
 * A region whose definition and metadata match one that the runtime's file
 * already has isn't copied again. The result lists the runtime's copy of each
 * region in the other file.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when `side_database_path` isn't a readable
 *   database file.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the merge would exceed the tile limit that
 *   MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED describes.
 */
MLN_BINDING("execution=operation;result=mln_offline_region_info;shape=array")
MLN_API mln_status mln_runtime_merge_offline_regions(
  mln_runtime runtime, mln_buffer_view side_database_path,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Replaces the metadata of the offline region `region_id`.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the cache file has no region `region_id`.
 */
MLN_BINDING("execution=operation;result=mln_offline_region_info")
MLN_API mln_status mln_runtime_update_offline_region_metadata(
  mln_runtime runtime, mln_offline_region_id region_id,
  const uint8_t* metadata MLN_BINDING("length=metadata_size;encoding=bytes"),
  size_t metadata_size, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the download progress of the offline region `region_id`.
 *
 * While the download is active, the counts cover what it has visited. Once it's
 * inactive, they cover every resource the region holds, including those that
 * only earlier versions of its style listed, so the completed counts can exceed
 * the required counts.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the cache file has no region `region_id`.
 */
MLN_BINDING("execution=query;result=mln_offline_region_status")
MLN_API mln_status mln_runtime_get_offline_region_status(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Selects the events that the runtime reports about the download of the
 * offline region `region_id`.
 *
 * `events` holds bits of MLN_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS and replaces
 * the region's previous selection. MLN_RUNTIME_EVENT_FLAG_NONE stops the
 * region's events.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the cache file has no region `region_id`.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_offline_region_observed(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_runtime_event_flags events, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts or pauses the download of the offline region `region_id`.
 *
 * Setting the state that the download already has does nothing. A download
 * stops when the runtime closes.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the cache file has no region `region_id`.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_offline_region_download_state(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_region_download_state state, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Makes maps check each resource of the offline region `region_id` with the
 * server before they use it.
 *
 * The region keeps its resources. Starting its download again fetches the
 * invalidated resources for offline use. If the style or a TileJSON document
 * is then missing or invalid, the download reports an offline region response
 * error and goes on from the stored copy. Resources that the current style no
 * longer lists stay in the region until it's deleted.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the cache file has no region `region_id`.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_invalidate_offline_region(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Deletes the offline region `region_id` from the runtime's cache file.
 *
 * Completes with:
 * - MLN_STATUS_NOT_FOUND when the cache file has no region `region_id`.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_delete_offline_region(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** A point in the Web Mercator projection, in meters. */
typedef struct mln_projected_meters {
  double northing;
  double easting;
} mln_projected_meters;

/** Settings that skew the map or draw it without perspective. */
typedef struct mln_projection_mode_options {
  uint32_t size;
  mln_projection_mode_option_fields fields;
  /** Whether the map draws without perspective. */
  bool axonometric
    MLN_BINDING("mask=fields;bit=MLN_PROJECTION_MODE_OPTION_AXONOMETRIC");
  double x_skew
    MLN_BINDING("mask=fields;bit=MLN_PROJECTION_MODE_OPTION_X_SKEW");
  double y_skew
    MLN_BINDING("mask=fields;bit=MLN_PROJECTION_MODE_OPTION_Y_SKEW");
} mln_projection_mode_options;

/**
 * A map's whole projection mode.
 *
 * Its members mean the same as those of mln_projection_mode_options.
 */
typedef struct mln_projection_mode {
  bool axonometric;
  double x_skew;
  double y_skew;
} mln_projection_mode;

/** How the map orients and constrains its view. */
typedef struct mln_map_viewport_options {
  uint32_t size;
  mln_map_viewport_option_fields fields;
  mln_north_orientation north_orientation
    MLN_BINDING("mask=fields;bit=MLN_MAP_VIEWPORT_OPTION_NORTH_ORIENTATION");
  mln_constrain_mode constrain_mode
    MLN_BINDING("mask=fields;bit=MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE");
  mln_viewport_mode viewport_mode
    MLN_BINDING("mask=fields;bit=MLN_MAP_VIEWPORT_OPTION_VIEWPORT_MODE");
  mln_edge_insets frustum_offset
    MLN_BINDING("mask=fields;bit=MLN_MAP_VIEWPORT_OPTION_FRUSTUM_OFFSET");
} mln_map_viewport_options;

/**
 * A map's whole viewport.
 *
 * Its members mean the same as those of mln_map_viewport_options.
 */
typedef struct mln_map_viewport {
  mln_north_orientation north_orientation;
  mln_constrain_mode constrain_mode;
  mln_viewport_mode viewport_mode;
  mln_edge_insets frustum_offset;
} mln_map_viewport;

/**
 * A projection that a style's
 * [projection](https://maplibre.org/maplibre-style-spec/projection/) names.
 */
typedef enum mln_projection_type : uint32_t {
  MLN_PROJECTION_TYPE_MERCATOR = 0,
  /** Draws the map as a globe. */
  MLN_PROJECTION_TYPE_VERTICAL_PERSPECTIVE = 1,
  /**
   * Draws the map as a globe at low zooms, and hands over to Mercator between
   * zoom 11 and 12.
   */
  MLN_PROJECTION_TYPE_GLOBE = 2,
} mln_projection_type;

/**
 * A style's projection at one zoom: `from` turning into `to`.
 *
 * A zeroed definition is Mercator.
 */
typedef struct mln_projection_definition {
  mln_projection_type from;
  mln_projection_type to;
  /**
   * How far `from` has turned into `to`, from 0 to 1. Ignored when `from`
   * equals `to`. A value outside that range holds at the nearer end.
   */
  double transition;
} mln_projection_definition;

/** One view of a map: its camera, size, and projection. */
typedef struct mln_map_view {
  uint32_t size;
  mln_camera camera;
  mln_logical_extent extent;
  mln_projection_mode projection_mode;
  mln_map_viewport viewport;
  /** The style's projection at the camera's zoom. */
  mln_projection_definition projection;
} mln_map_view;

/**
 * How the map picks the tiles to load.
 *
 * The `lod_` members lower the level of detail away from the camera, so the
 * map loads fewer tiles when the pitch is high.
 */
typedef struct mln_map_tile_options {
  uint32_t size;
  mln_map_tile_option_fields fields;
  /**
   * How many zoom levels lower the map also loads tiles for, so it can show a
   * coarse map while the current tiles load. 0 loads only the current tiles.
   */
  uint32_t prefetch_zoom_delta
    MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_PREFETCH_ZOOM_DELTA");
  /** The radius around the view point, in tiles, that keeps full detail. */
  double lod_min_radius
    MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_MIN_RADIUS");
  /** Scales the distance to the view point. Above 1 lowers the detail. */
  double lod_scale MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_SCALE");
  /** The pitch, in degrees, above which the map lowers the detail. */
  double lod_pitch_threshold
    MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_PITCH_THRESHOLD");
  /** Zoom levels to add for picking the detail. Below 0 loads fewer tiles. */
  double lod_zoom_shift
    MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_ZOOM_SHIFT");
  /** MLN_TILE_LOD_MODE_DISTANCE ignores `lod_min_radius`. */
  mln_tile_lod_mode lod_mode
    MLN_BINDING("mask=fields;bit=MLN_MAP_TILE_OPTION_LOD_MODE");
} mln_map_tile_options;

/**
 * All the settings for how a map picks the tiles to load.
 *
 * Its members mean the same as those of mln_map_tile_options.
 */
typedef struct mln_map_tile_settings {
  uint32_t prefetch_zoom_delta;
  double lod_min_radius;
  double lod_scale;
  /** The pitch, in degrees, above which the map lowers the detail. */
  double lod_pitch_threshold;
  double lod_zoom_shift;
  mln_tile_lod_mode lod_mode;
} mln_map_tile_settings;

/** A map's state, as mln_map_get_snapshot() reads it. */
typedef struct mln_map_snapshot {
  uint32_t size;
  mln_map_debug_flags debug_flags;
  /** Grows with each change to the map's state. */
  uint64_t generation;
  /** Whether the style and every resource that the view needs have loaded. */
  bool fully_loaded;
  bool rendering_stats_view_enabled;
  bool gesture_in_progress;
  bool transition_in_progress;
  mln_runtime_event_flags event_mask;
  /** Counts the render updates that the map has made for its render session. */
  uint64_t latest_render_update_generation;
  mln_map_tile_settings tile;
  mln_camera_bounds bounds;
  mln_free_camera free_camera;
  mln_map_view view;
  /** Whether the map draws as a globe, alone or blending into Mercator. */
  bool globe_rendering;
  /** The setting from mln_map_set_globe_free_bearing_enabled(). */
  bool globe_free_bearing_enabled;
} mln_map_snapshot;

/** A map's camera, with the snapshot generation that it matches. */
typedef struct mln_camera_query_result {
  uint64_t generation;
  mln_camera camera;
} mln_camera_query_result;

/**
 * Creates a map on `runtime`.
 *
 * Asynchronous calls on the map before the completion runs wait for it, and
 * complete with MLN_STATUS_INVALID_STATE when creation fails.
 *
 * Returns:
 * - MLN_STATUS_INVALID_ARGUMENT when the `fallback_font_faces` of `options`
 *   isn't an array of font files.
 *
 * Lifetimes:
 * `out_map` receives the handle before the call returns, while the map is
 * still being built.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_runtime_create_map(
  mln_runtime runtime, const mln_map_options* options,
  mln_map* out_map MLN_BINDING("direction=out"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Copies the map's state.
 *
 * The copy doesn't wait for pending commands. To read the camera after they
 * apply, use mln_map_get_camera().
 *
 * Returns:
 * - MLN_STATUS_INVALID_STATE until mln_runtime_create_map() completes with
 *   MLN_STATUS_OK.
 */
MLN_BINDING("execution=snapshot")
MLN_API mln_status mln_map_get_snapshot(
  mln_map map, mln_map_snapshot* out_snapshot MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets the map's size and scale factor.
 *
 * The map keeps its loaded tiles. A map with a render session takes its size
 * and scale factor from the session, so resize the session with
 * mln_render_session_resize() instead.
 *
 * Returns:
 * - MLN_STATUS_INVALID_STATE when the map has a render session.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_STATE when a render session attached to the map before
 *   the resize applied.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_resize(
  mln_map map, mln_logical_extent extent, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Renders a new frame of a continuous map, even when nothing changed. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_request_repaint(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Holds back the map's changes from rendering until
 * mln_map_end_command_group(), so they show in the same frame.
 *
 * Groups nest, and the changes render when the outermost group ends.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_begin_command_group(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Ends the group that mln_map_begin_command_group() began. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_end_command_group(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Merges `state` into the state of a feature.
 *
 * `state` is a JSON object. Its keys replace the same keys in the feature's
 * state, and the other keys stay.
 *
 * The map keeps the state until it is removed, its source leaves the style, or
 * a style loads. mln_map_remove_style_source() removes the state of the
 * source's features, and a style that loads removes the state of every source,
 * so a later source with the same ID starts with no state. To set state for a
 * new style's sources, wait until mln_map_set_style_url() or
 * mln_map_set_style_json() completes. The style load removes state set before
 * then.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  mln_buffer_view state MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the state of a feature, as a JSON object.
 *
 * The object is empty when the feature has no state, including after its
 * source leaves the style or a style loads, as mln_map_set_feature_state()
 * describes.
 */
MLN_BINDING("execution=query;result=mln_buffer_view;encoding=json")
MLN_API mln_status mln_map_get_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Removes the feature state that `selector` selects. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_remove_feature_state(
  mln_map map, const mln_feature_state_selector* selector,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Renders a still image of a static or tile map.
 *
 * The map's render session draws the image once every resource that the image
 * needs has loaded.
 *
 * Completes with:
 * - MLN_STATUS_CANCELLED when the map is released before the image is done.
 * - MLN_STATUS_NATIVE_ERROR when the style or a resource that the image needs
 *   fails to load.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_map_request_still_image(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases the map.
 *
 * Detach the map's render session first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_map_release(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases the map in any state, and finishes in the background.
 *
 * Use it from destructors and finalizers. Otherwise, use mln_map_release().
 */
MLN_API mln_status
mln_map_dispose(mln_map map, mln_diagnostic* out_diagnostic) MLN_NOEXCEPT;

/**
 * Loads the map's style from `url`.
 *
 * The command completes once the style document has loaded and parsed. The
 * style's sources, sprites, and glyphs then load in the background.
 *
 * The map's style is loaded from the time a style document parses until the
 * next call to this function. While the style isn't loaded, the commands that
 * change it complete with MLN_STATUS_INVALID_STATE. To change a new map's
 * style without a style document, first load an empty style, such as
 * `{"version":8,"sources":{},"layers":[]}`.
 *
 * Completes with:
 * - MLN_STATUS_NATIVE_ERROR when the style document fails to load or isn't a
 *   valid style.
 * - MLN_STATUS_CANCELLED when the map is released, or this function or
 *   mln_map_set_style_json() loads another style, before the style document
 *   parses.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_url(
  mln_map map, mln_buffer_view url, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Loads the map's style from `json`.
 *
 * The command completes once the style has parsed, and the map's style is then
 * loaded, as mln_map_set_style_url() describes. The style's sources, sprites,
 * and glyphs load in the background.
 *
 * Completes with:
 * - MLN_STATUS_INVALID_ARGUMENT when `json` isn't a valid style. The map keeps
 *   its current style.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_style_json(
  mln_map map, mln_buffer_view json MLN_BINDING("encoding=json"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the JSON of the style that the map loaded.
 *
 * The JSON doesn't include the changes made to the style since it loaded.
 */
MLN_BINDING("execution=query;result=mln_buffer_view;encoding=json")
MLN_API mln_status mln_map_get_loaded_style_json(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reads the URL of the map's style.
 *
 * The URL is empty when the style came from mln_map_set_style_json().
 */
MLN_BINDING("execution=query;result=mln_buffer_view")
MLN_API mln_status mln_map_get_style_url(
  mln_map map, const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Selects the events that the map reports. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_map_set_event_mask(
  mln_map map, mln_runtime_event_flags mask, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_MAP_H
