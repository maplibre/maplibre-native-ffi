/**
 * @file maplibre_native_c/runtime.h
 * Create runtimes, control how they load and cache resources, and receive
 * their events.
 */

#ifndef MAPLIBRE_NATIVE_C_RUNTIME_H
#define MAPLIBRE_NATIVE_C_RUNTIME_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "base.h"
#include "completion.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Whether the library may use the network. */
typedef enum mln_network_status : uint32_t {
  MLN_NETWORK_STATUS_ONLINE = 1,
  MLN_NETWORK_STATUS_OFFLINE = 2,
} mln_network_status;

/**
 * An action on a runtime's ambient cache.
 *
 * The ambient cache holds the resources that maps load, apart from the ones
 * that offline regions hold.
 */
typedef enum mln_ambient_cache_operation : uint32_t {
  /**
   * Deletes the whole database, offline regions included, and starts an empty
   * one.
   */
  MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE = 1,
  /** Shrinks the database file to free the disk space it no longer uses. */
  MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE = 2,
  /**
   * Makes the library check each cached resource for changes before it uses
   * the resource again.
   */
  MLN_AMBIENT_CACHE_OPERATION_INVALIDATE = 3,
  MLN_AMBIENT_CACHE_OPERATION_CLEAR = 4,
} mln_ambient_cache_operation;

/** The ID of an offline region in a runtime's database. */
typedef int64_t mln_offline_region_id;

/** The shape of an offline region. */
typedef enum mln_offline_region_definition_type : uint32_t {
  MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID = 1,
  MLN_OFFLINE_REGION_DEFINITION_GEOMETRY = 2,
} mln_offline_region_definition_type;

/** Whether an offline region is downloading. */
typedef enum mln_offline_region_download_state : uint32_t {
  MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE = 0,
  MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE = 1,
} mln_offline_region_download_state;

/** The download progress of an offline region. */
typedef struct mln_offline_region_status {
  mln_offline_region_download_state download_state;
  /** The resources downloaded so far, tiles included. */
  uint64_t completed_resource_count;
  /** The size of the downloaded resources in bytes, tiles included. */
  uint64_t completed_resource_size;
  uint64_t completed_tile_count;
  uint64_t required_tile_count;
  /** The size of the downloaded tiles in bytes. */
  uint64_t completed_tile_size;
  /**
   * The resources that the region needs, tiles included. Until
   * `required_resource_count_is_precise` is true, this is only a lower bound.
   */
  uint64_t required_resource_count;
  bool required_resource_count_is_precise;
  /** Whether the region has downloaded every resource it needs. */
  bool complete;
} mln_offline_region_status;

/**
 * The kind of an event.
 *
 * Only maps in MLN_MAP_MODE_CONTINUOUS raise
 * MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED, MLN_RUNTIME_EVENT_MAP_IDLE, and the
 * render frame and render map events. Offline region events come only for the
 * regions that mln_runtime_set_offline_region_observed() observes.
 */
typedef enum mln_runtime_event_type : uint32_t {
  MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE = 1,
  MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING = 2,
  MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE = 3,
  MLN_RUNTIME_EVENT_MAP_STYLE_LOADED = 4,
  /** The map started to load a style. */
  MLN_RUNTIME_EVENT_MAP_LOADING_STARTED = 5,
  /**
   * The map loaded its style and everything that the current view needs, and
   * rendered the result.
   */
  MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED = 6,
  MLN_RUNTIME_EVENT_MAP_LOADING_FAILED = 7,
  /**
   * The map rendered a complete frame and has nothing left to load, animate,
   * or redraw.
   */
  MLN_RUNTIME_EVENT_MAP_IDLE = 8,
  MLN_RUNTIME_EVENT_MAP_RENDER_ERROR = 9,
  MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED = 10,
  MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED = 11,
  MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED = 12,
  MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED = 13,
  MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING = 14,
  /** A tile moved to another step of loading. */
  MLN_RUNTIME_EVENT_MAP_TILE_ACTION = 15,
  MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED = 16,
  MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED = 17,
  /** A resource that the region needs failed to download. */
  MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR = 18,
  /**
   * The region's download became inactive because the cache file's offline
   * regions reached their limit of 6000 tiles from MapLibre's demo tile server.
   *
   * Only tiles of sources on the demo server count toward the limit: sources
   * with a `maplibre://` URL, which resolves to https://demotiles.maplibre.org.
   * Tiles from other servers never count.
   */
  MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 19,
  MLN_RUNTIME_EVENT_MAP_GLYPHS_REQUESTED = 20,
  MLN_RUNTIME_EVENT_MAP_GLYPHS_LOADED = 21,
  MLN_RUNTIME_EVENT_MAP_GLYPHS_ERROR = 22,
  MLN_RUNTIME_EVENT_MAP_SPRITE_REQUESTED = 23,
  MLN_RUNTIME_EVENT_MAP_SPRITE_LOADED = 24,
  MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR = 25,
} mln_runtime_event_type;

/** An event type, as a bit in a set. */
typedef enum MLN_BINDING("kind=bitmask") mln_runtime_event_flag : uint64_t {
  MLN_RUNTIME_EVENT_FLAG_NONE = 0,
  MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_WILL_CHANGE = 1ULL
    << MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE,
  MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_IS_CHANGING = 1ULL
    << MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING,
  MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_DID_CHANGE = 1ULL
    << MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
  MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_STYLE_LOADED,
  MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_STARTED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_LOADING_STARTED,
  MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FINISHED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED,
  MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FAILED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_LOADING_FAILED,
  MLN_RUNTIME_EVENT_FLAG_MAP_IDLE = 1ULL << MLN_RUNTIME_EVENT_MAP_IDLE,
  MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_ERROR = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_ERROR,
  MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_STARTED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED,
  MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_FINISHED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED,
  MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_STARTED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED,
  MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_FINISHED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED,
  MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_IMAGE_MISSING = 1ULL
    << MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING,
  MLN_RUNTIME_EVENT_FLAG_MAP_TILE_ACTION = 1ULL
    << MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
  MLN_RUNTIME_EVENT_FLAG_MAP_SOURCE_CHANGED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED,
  MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_STATUS_CHANGED = 1ULL
    << MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
  MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_RESPONSE_ERROR = 1ULL
    << MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
  MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 1ULL
    << MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
  MLN_RUNTIME_EVENT_FLAG_MAP_GLYPHS_REQUESTED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_GLYPHS_REQUESTED,
  MLN_RUNTIME_EVENT_FLAG_MAP_GLYPHS_LOADED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_GLYPHS_LOADED,
  MLN_RUNTIME_EVENT_FLAG_MAP_GLYPHS_ERROR = 1ULL
    << MLN_RUNTIME_EVENT_MAP_GLYPHS_ERROR,
  MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_REQUESTED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_SPRITE_REQUESTED,
  MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_LOADED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_SPRITE_LOADED,
  MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_ERROR = 1ULL
    << MLN_RUNTIME_EVENT_MAP_SPRITE_ERROR,
  MLN_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS =
      MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_WILL_CHANGE |
    MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_IS_CHANGING |
    MLN_RUNTIME_EVENT_FLAG_MAP_CAMERA_DID_CHANGE |
    MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED |
    MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_STARTED |
    MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FINISHED |
    MLN_RUNTIME_EVENT_FLAG_MAP_LOADING_FAILED |
    MLN_RUNTIME_EVENT_FLAG_MAP_IDLE | MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_ERROR |
    MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_STARTED |
    MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_FRAME_FINISHED |
    MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_STARTED |
    MLN_RUNTIME_EVENT_FLAG_MAP_RENDER_MAP_FINISHED |
    MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_IMAGE_MISSING |
    MLN_RUNTIME_EVENT_FLAG_MAP_TILE_ACTION |
    MLN_RUNTIME_EVENT_FLAG_MAP_SOURCE_CHANGED |
    MLN_RUNTIME_EVENT_FLAG_MAP_GLYPHS_REQUESTED |
    MLN_RUNTIME_EVENT_FLAG_MAP_GLYPHS_LOADED |
    MLN_RUNTIME_EVENT_FLAG_MAP_GLYPHS_ERROR |
    MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_REQUESTED |
    MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_LOADED |
    MLN_RUNTIME_EVENT_FLAG_MAP_SPRITE_ERROR,
  MLN_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS =
      MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_STATUS_CHANGED |
    MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_RESPONSE_ERROR |
    MLN_RUNTIME_EVENT_FLAG_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
  MLN_RUNTIME_EVENT_FLAG_ALL = MLN_RUNTIME_EVENT_FLAG_ALL_MAP_EVENTS |
    MLN_RUNTIME_EVENT_FLAG_ALL_RUNTIME_EVENTS,
} mln_runtime_event_flag;

/** A set of mln_runtime_event_flag bits. */
typedef uint64_t mln_runtime_event_flags;

/** The kind of handle that raised an event. */
typedef enum mln_runtime_event_source_type : uint32_t {
  MLN_RUNTIME_EVENT_SOURCE_RUNTIME = 0,
  MLN_RUNTIME_EVENT_SOURCE_MAP = 1,
} mln_runtime_event_source_type;

/** The member of mln_runtime_event_payload that an event carries. */
typedef enum mln_runtime_event_payload_type : uint32_t {
  MLN_RUNTIME_EVENT_PAYLOAD_NONE = 0,
  MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME = 1,
  MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP = 2,
  MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION = 3,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS = 4,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR = 5,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT = 6,
  MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS = 7,
  MLN_RUNTIME_EVENT_PAYLOAD_SPRITE = 8,
} mln_runtime_event_payload_type;

/** Whether a camera change jumps or animates. */
typedef enum mln_camera_change_mode : uint32_t {
  MLN_CAMERA_CHANGE_MODE_IMMEDIATE = 0,
  MLN_CAMERA_CHANGE_MODE_ANIMATED = 1,
} mln_camera_change_mode;

/**
 * Whether a frame drew the map complete, or lacked resources that are still
 * loading.
 */
typedef enum mln_render_mode : uint32_t {
  MLN_RENDER_MODE_PARTIAL = 0,
  MLN_RENDER_MODE_FULL = 1,
} mln_render_mode;

/** A step in loading a tile. */
typedef enum mln_tile_operation : uint32_t {
  MLN_TILE_OPERATION_REQUESTED_FROM_CACHE = 0,
  MLN_TILE_OPERATION_REQUESTED_FROM_NETWORK = 1,
  MLN_TILE_OPERATION_LOAD_FROM_NETWORK = 2,
  MLN_TILE_OPERATION_LOAD_FROM_CACHE = 3,
  MLN_TILE_OPERATION_START_PARSE = 4,
  MLN_TILE_OPERATION_END_PARSE = 5,
  MLN_TILE_OPERATION_ERROR = 6,
  MLN_TILE_OPERATION_CANCELLED = 7,
  MLN_TILE_OPERATION_NULL = 8,
} mln_tile_operation;

/** The kind of a resource that the library loads. */
typedef enum mln_resource_kind : uint32_t {
  MLN_RESOURCE_KIND_UNKNOWN = 0,
  MLN_RESOURCE_KIND_STYLE = 1,
  MLN_RESOURCE_KIND_SOURCE = 2,
  MLN_RESOURCE_KIND_TILE = 3,
  MLN_RESOURCE_KIND_GLYPHS = 4,
  MLN_RESOURCE_KIND_SPRITE_IMAGE = 5,
  MLN_RESOURCE_KIND_SPRITE_JSON = 6,
  MLN_RESOURCE_KIND_IMAGE = 7,
} mln_resource_kind;

/** Where a request may load its resource from. */
typedef enum mln_resource_loading_method : uint32_t {
  /** The cache or the network. */
  MLN_RESOURCE_LOADING_METHOD_ALL = 0,
  MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY = 1,
  MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY = 2,
} mln_resource_loading_method;

/** The priority of a resource request. */
typedef enum mln_resource_priority : uint32_t {
  MLN_RESOURCE_PRIORITY_REGULAR = 0,
  MLN_RESOURCE_PRIORITY_LOW = 1,
} mln_resource_priority;

/** Whether a request loads a resource for a map or an offline region. */
typedef enum mln_resource_usage : uint32_t {
  MLN_RESOURCE_USAGE_ONLINE = 0,
  MLN_RESOURCE_USAGE_OFFLINE = 1,
} mln_resource_usage;

/** Whether the library keeps a loaded resource in its cache. */
typedef enum mln_resource_storage_policy : uint32_t {
  MLN_RESOURCE_STORAGE_POLICY_PERMANENT = 0,
  MLN_RESOURCE_STORAGE_POLICY_VOLATILE = 1,
} mln_resource_storage_policy;

/** The outcome of a resource request. */
typedef enum mln_resource_response_status : uint32_t {
  MLN_RESOURCE_RESPONSE_STATUS_OK = 0,
  MLN_RESOURCE_RESPONSE_STATUS_ERROR = 1,
  /** The resource exists and is empty. */
  MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT = 2,
  /**
   * The earlier copy that the request's `prior_` fields describe is current.
   */
  MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED = 3,
} mln_resource_response_status;

/** Why a resource request failed. */
typedef enum mln_resource_error_reason : uint32_t {
  MLN_RESOURCE_ERROR_REASON_NONE = 0,
  MLN_RESOURCE_ERROR_REASON_NOT_FOUND = 1,
  /** The server reported an error. */
  MLN_RESOURCE_ERROR_REASON_SERVER = 2,
  /** The server couldn't be reached. */
  MLN_RESOURCE_ERROR_REASON_CONNECTION = 3,
  MLN_RESOURCE_ERROR_REASON_RATE_LIMIT = 4,
  MLN_RESOURCE_ERROR_REASON_OTHER = 5,
} mln_resource_error_reason;

/** Reads whether the library may use the network. */
MLN_API mln_status mln_network_get_status(
  mln_network_status* out_status MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Tells the library whether it may use the network.
 *
 * While the status is MLN_NETWORK_STATUS_OFFLINE, the library starts no
 * network requests. The requests that wait start when the status returns to
 * MLN_NETWORK_STATUS_ONLINE. The status starts as online and applies to every
 * runtime in the process.
 */
MLN_API mln_status mln_network_set_status(
  mln_network_status status, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Options for mln_runtime_create(). */
typedef struct mln_runtime_options {
  uint32_t size;
  /**
   * The directory that `asset://` URLs load from. Empty uses the working
   * directory, or the app's assets on Android.
   */
  mln_buffer_view asset_path;
  /**
   * The database file that holds the ambient cache and offline regions. Empty
   * keeps them in memory.
   */
  mln_buffer_view cache_path;
} mln_runtime_options;

/** Statistics about the frames that a map renders. */
typedef struct mln_rendering_stats {
  /** The time that the frame took to encode, in milliseconds. */
  double encoding_time_ms;
  /** The time that the frame took to render, in milliseconds. */
  double rendering_time_ms;
  /** The frames rendered so far. */
  int64_t frame_count;
  /** The draw calls in the frame. */
  int64_t draw_call_count;
  /** The draw calls in every frame so far. */
  int64_t total_draw_call_count;
} mln_rendering_stats;

/** The details of MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED. */
typedef struct mln_runtime_event_render_frame {
  mln_render_mode mode;
  bool needs_repaint;
  bool placement_changed;
  mln_rendering_stats stats;
} mln_runtime_event_render_frame;

/** The details of MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED. */
typedef struct mln_runtime_event_render_map {
  mln_render_mode mode;
} mln_runtime_event_render_map;

/**
 * The ID of a tile.
 *
 * `canonical_x` and `canonical_y` place the tile in the source's grid at zoom
 * `canonical_z`. `overscaled_z` is the zoom that the map draws the tile at,
 * and is greater when the source has no tiles that deep. `wrap` says which
 * copy of the world holds the tile: zero for the main one, negative to the
 * west, and positive to the east.
 */
typedef struct mln_tile_id {
  uint32_t overscaled_z;
  int32_t wrap;
  uint32_t canonical_z;
  uint32_t canonical_x;
  uint32_t canonical_y;
} mln_tile_id;

/** The details of MLN_RUNTIME_EVENT_MAP_TILE_ACTION. */
typedef struct mln_runtime_event_tile_action {
  mln_tile_operation operation;
  mln_tile_id tile_id;
} mln_runtime_event_tile_action;

/**
 * The details of a glyph event.
 *
 * The event covers the glyphs from `first` to `last`, both included.
 */
typedef struct mln_runtime_event_glyphs {
  /** The fonts that the glyphs come from, in order of preference. */
  const mln_buffer_view* font_stack MLN_BINDING("length=font_count");
  size_t font_count;
  uint32_t first;
  uint32_t last;
  bool font_face;
} mln_runtime_event_glyphs;

/**
 * The details of a sprite event.
 *
 * `id` and `url` are both empty when the style has no sprite.
 */
typedef struct mln_runtime_event_sprite {
  mln_buffer_view id;
  mln_buffer_view url;
} mln_runtime_event_sprite;

/** The details of MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED. */
typedef struct mln_runtime_event_offline_region_status {
  mln_offline_region_id region_id;
  mln_offline_region_status status;
} mln_runtime_event_offline_region_status;

/** The details of MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR. */
typedef struct mln_runtime_event_offline_region_response_error {
  mln_offline_region_id region_id;
  mln_resource_error_reason reason;
} mln_runtime_event_offline_region_response_error;

/**
 * The details of MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED.
 */
typedef struct mln_runtime_event_offline_region_tile_count_limit {
  mln_offline_region_id region_id;
  /**
   * The number of demo server tiles that the cache file's offline regions may
   * hold together.
   */
  uint64_t limit;
} mln_runtime_event_offline_region_tile_count_limit;

/** The details of an event, in the member that its `payload_type` names. */
typedef union mln_runtime_event_payload {
  mln_runtime_event_render_frame render_frame
    MLN_BINDING("variant=MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME");
  mln_runtime_event_render_map render_map
    MLN_BINDING("variant=MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP");
  mln_runtime_event_tile_action tile_action
    MLN_BINDING("variant=MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION");
  mln_runtime_event_offline_region_status offline_region_status
    MLN_BINDING("variant=MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS");
  mln_runtime_event_offline_region_response_error offline_region_response_error
    MLN_BINDING(
      "variant=MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR"
    );
  mln_runtime_event_offline_region_tile_count_limit
    offline_region_tile_count_limit MLN_BINDING(
      "variant=MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT"
    );
  mln_runtime_event_glyphs glyphs
    MLN_BINDING("variant=MLN_RUNTIME_EVENT_PAYLOAD_GLYPHS");
  mln_runtime_event_sprite sprite
    MLN_BINDING("variant=MLN_RUNTIME_EVENT_PAYLOAD_SPRITE");
} mln_runtime_event_payload;

/** An event that a runtime or one of its maps raised. */
typedef struct mln_runtime_event {
  mln_runtime_event_type type;
  mln_runtime_event_source_type source_type;
  /** The runtime or map that raised the event, as `source_type` says. */
  uint64_t source;
  /**
   * For a map event, a map snapshot with this generation or a later one
   * includes what the event reports. Zero for a runtime event.
   */
  uint64_t generation;
  /**
   * The mln_camera_change_mode for MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE
   * and MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE.
   */
  uint32_t code;
  mln_runtime_event_payload_type payload_type;
  /**
   * The error for an event that reports a failure, the image ID for
   * MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING, or the source ID for
   * MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED and MLN_RUNTIME_EVENT_MAP_TILE_ACTION.
   */
  mln_buffer_view message;
  mln_runtime_event_payload payload MLN_BINDING(
    "tag=payload_type;empty_variant=MLN_RUNTIME_EVENT_PAYLOAD_NONE"
  );
} mln_runtime_event;

/**
 * A batch of events.
 *
 * Lifetimes:
 * Step through `events` by `event_size` bytes. A newer library's events can be
 * larger than the mln_runtime_event in these headers.
 */
typedef struct mln_event_batch_view {
  uint32_t size;
  uint32_t event_size;
  const mln_runtime_event* events
    MLN_BINDING("length=event_count;stride=event_size");
  size_t event_count;
} mln_event_batch_view MLN_BINDING("kind=slice");

/**
 * Receives a batch of events.
 *
 * Lifetimes:
 * `events` and the data it points to stay valid until the callback returns.
 */
MLN_BINDING("capture=manual")
typedef void (*mln_runtime_event_callback)(
  void* user_data, const mln_event_batch_view* events
);

/** An event callback, for mln_runtime_set_event_handler(). */
typedef struct mln_runtime_event_handler {
  uint32_t size;
  mln_runtime_event_callback callback;
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_runtime_event_handler MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** A resource kind, as a bit in a set. */
typedef enum MLN_BINDING("kind=bitmask") mln_resource_kind_flag : uint32_t {
  MLN_RESOURCE_KIND_FLAG_UNKNOWN = 1U << MLN_RESOURCE_KIND_UNKNOWN,
  MLN_RESOURCE_KIND_FLAG_STYLE = 1U << MLN_RESOURCE_KIND_STYLE,
  MLN_RESOURCE_KIND_FLAG_SOURCE = 1U << MLN_RESOURCE_KIND_SOURCE,
  MLN_RESOURCE_KIND_FLAG_TILE = 1U << MLN_RESOURCE_KIND_TILE,
  MLN_RESOURCE_KIND_FLAG_GLYPHS = 1U << MLN_RESOURCE_KIND_GLYPHS,
  MLN_RESOURCE_KIND_FLAG_SPRITE_IMAGE = 1U << MLN_RESOURCE_KIND_SPRITE_IMAGE,
  MLN_RESOURCE_KIND_FLAG_SPRITE_JSON = 1U << MLN_RESOURCE_KIND_SPRITE_JSON,
  MLN_RESOURCE_KIND_FLAG_IMAGE = 1U << MLN_RESOURCE_KIND_IMAGE,
  MLN_RESOURCE_KIND_FLAG_ALL = MLN_RESOURCE_KIND_FLAG_UNKNOWN |
    MLN_RESOURCE_KIND_FLAG_STYLE | MLN_RESOURCE_KIND_FLAG_SOURCE |
    MLN_RESOURCE_KIND_FLAG_TILE | MLN_RESOURCE_KIND_FLAG_GLYPHS |
    MLN_RESOURCE_KIND_FLAG_SPRITE_IMAGE | MLN_RESOURCE_KIND_FLAG_SPRITE_JSON |
    MLN_RESOURCE_KIND_FLAG_IMAGE,
} mln_resource_kind_flag;

/** A set of mln_resource_kind_flag bits. */
typedef uint32_t mln_resource_kind_flags;

// This block uses line comments because its examples contain URL patterns that
// a block comment cannot carry.

/// How a URL pattern matches a request's URL.
///
/// A pattern matches the whole URL, query included.
typedef enum MLN_BINDING("kind=bitmask") mln_url_match_flag : uint32_t {
  MLN_URL_MATCH_EXACT = 0U,
  /// The URL matches the pattern as a glob. `*` matches any characters except
  /// `/`, `**` matches any characters, and `?` matches one character except
  /// `/`. `\` makes the next character match only itself. For example,
  /// `https://*.example.com/tiles/**` matches every URL under `/tiles/` on
  /// each subdomain of `example.com`.
  MLN_URL_MATCH_GLOB = 1U << 0U,
} mln_url_match_flag;

/** A set of mln_url_match_flag bits. */
typedef uint32_t mln_url_match_flags;

/** The requests that a resource provider serves. */
typedef struct mln_resource_route {
  uint32_t size;
  /** The kinds of resource that the route covers. Zero covers every kind. */
  mln_resource_kind_flags kinds;
  mln_url_match_flags flags;
  mln_buffer_view url;
} mln_resource_route;

/**
 * An HTTP header.
 *
 * `name` follows the rules of
 * mln_http_header_transform_response_set_header().
 */
typedef struct mln_http_header {
  mln_buffer_view name MLN_BINDING("required=true");
  mln_buffer_view value;
} mln_http_header;

/** Headers to add to the HTTP requests whose URL matches a pattern. */
typedef struct mln_http_header_rule {
  uint32_t size;
  /** The kinds of resource that the rule covers. Zero covers every kind. */
  mln_resource_kind_flags kinds;
  mln_url_match_flags flags;
  mln_buffer_view url;
  const mln_http_header* headers MLN_BINDING("length=header_count");
  size_t header_count;
} mln_http_header_rule;

/**
 * Rewrites the URL of a network request before the library sends it.
 *
 * The request waits until you answer with
 * mln_resource_transform_request_complete() or
 * mln_resource_transform_request_fail(). To keep `url`, complete with it.
 * Releasing `request` without an answer fails the request. You can answer now
 * or later, from any thread.
 *
 * Lifetimes:
 * `url` stays valid until the callback returns. The callback owns `request`.
 */
typedef void (*mln_resource_transform_callback)(
  void* user_data, mln_resource_kind kind, mln_buffer_view url,
  mln_resource_transform_request request MLN_BINDING("ownership=owned")
);

/** The fields that mln_resource_transform_error sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_resource_transform_error_field : uint32_t {
  MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER = 1U << 0U,
} mln_resource_transform_error_field;

/** A set of mln_resource_transform_error_field bits. */
typedef uint32_t mln_resource_transform_error_fields;

/** Why a resource transform fails a request. */
typedef struct mln_resource_transform_error {
  uint32_t size;
  mln_resource_transform_error_fields fields;
  /** Must not be MLN_RESOURCE_ERROR_REASON_NONE. */
  mln_resource_error_reason reason MLN_BINDING("required=true");
  mln_buffer_view message;
  /** When to retry, for MLN_RESOURCE_ERROR_REASON_RATE_LIMIT. */
  int64_t retry_after_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_TRANSFORM_ERROR_RETRY_AFTER");
} mln_resource_transform_error;

/** A resource transform callback, for mln_runtime_set_resource_transform(). */
typedef struct mln_resource_transform {
  uint32_t size;
  mln_resource_transform_callback callback;
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_resource_transform MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** The headers that an HTTP header transform callback sets. */
typedef struct mln_http_header_transform_response {
  uint32_t size;
  void* context MLN_BINDING("kind=context;lifetime=call");
} mln_http_header_transform_response MLN_BINDING("kind=callback_response");

/**
 * Sets a header on the request that an HTTP header transform callback handles.
 *
 * Header names ignore letter case, and setting a header again replaces its
 * value. `name` can't be a header that the library or the platform manages:
 * `Accept-Encoding`, `Connection`, `Content-Length`, `Host`,
 * `If-Modified-Since`, `If-None-Match`, `Keep-Alive`, `Proxy-Authorization`,
 * `Proxy-Connection`, `Range`, `TE`, `Trailer`, `Transfer-Encoding`, `Upgrade`,
 * or `User-Agent`.
 */
MLN_API mln_status mln_http_header_transform_response_set_header(
  mln_http_header_transform_response* response, mln_buffer_view name,
  mln_buffer_view value, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets the headers of an HTTP request before the library sends it.
 *
 * `url` is the request's URL after any resource transform. The request waits
 * for the callback, so return quickly.
 *
 * Threads:
 * Runs on a library thread.
 *
 * Lifetimes:
 * `url` and `out_response` stay valid until the callback returns.
 */
MLN_BINDING("failure=MLN_STATUS_NATIVE_ERROR")
typedef mln_status (*mln_http_header_transform_callback)(
  void* user_data, mln_resource_kind kind, mln_buffer_view url,
  mln_http_header_transform_response* out_response MLN_BINDING("direction=out")
);

/**
 * Headers to add to HTTP requests, for mln_runtime_set_http_header_transform().
 *
 * Set either `callback` or `rules`, not both.
 */
typedef struct mln_http_header_transform {
  uint32_t size;
  mln_http_header_transform_callback callback MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  /** The first rule that matches a request adds its headers, and no other. */
  const mln_http_header_rule* rules MLN_BINDING("length=rule_count");
  size_t rule_count;
} mln_http_header_transform MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** The bytes from `start` to `end` of a resource, both included. */
typedef struct mln_resource_range {
  uint64_t start;
  uint64_t end;
} mln_resource_range;

/** The fields that mln_resource_request_info sets. */
typedef enum MLN_BINDING("kind=bitmask") mln_resource_request_field : uint32_t {
  MLN_RESOURCE_REQUEST_RANGE = 1U << 0U,
  MLN_RESOURCE_REQUEST_PRIOR_MODIFIED = 1U << 1U,
  MLN_RESOURCE_REQUEST_PRIOR_EXPIRES = 1U << 2U,
} mln_resource_request_field;

/** A set of mln_resource_request_field bits. */
typedef uint32_t mln_resource_request_fields;

/**
 * A request that a resource provider serves.
 *
 * When the cache holds an earlier copy of the resource, the `prior_` fields
 * describe it.
 */
typedef struct mln_resource_request_info {
  uint32_t size;
  mln_resource_request_fields fields;
  /** The URL as the style names it. */
  mln_buffer_view requested_url;
  /**
   * The URL to load, after the library expands any shorthand in
   * `requested_url` and before any resource transform. Routes match this URL.
   */
  mln_buffer_view resolved_url;
  mln_resource_kind kind;
  mln_resource_loading_method loading_method;
  mln_resource_priority priority;
  mln_resource_usage usage;
  mln_resource_storage_policy storage_policy;
  /** The only bytes of the resource that the request wants. */
  mln_resource_range range
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_REQUEST_RANGE");
  int64_t prior_modified_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_REQUEST_PRIOR_MODIFIED");
  int64_t prior_expires_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_REQUEST_PRIOR_EXPIRES");
  /** Empty when the earlier copy has no ETag. */
  mln_buffer_view prior_etag;
  const uint8_t* prior_data
    MLN_BINDING("length=prior_data_size;encoding=bytes");
  size_t prior_data_size;
} mln_resource_request_info;

/** The fields that mln_resource_response sets. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_resource_response_field : uint32_t {
  MLN_RESOURCE_RESPONSE_MODIFIED = 1U << 0U,
  MLN_RESOURCE_RESPONSE_EXPIRES = 1U << 1U,
  MLN_RESOURCE_RESPONSE_RETRY_AFTER = 1U << 2U,
} mln_resource_response_field;

/** A set of mln_resource_response_field bits. */
typedef uint32_t mln_resource_response_fields;

/** The answer to a resource request. */
typedef struct mln_resource_response {
  uint32_t size;
  mln_resource_response_fields fields;
  mln_resource_response_status status;
  /** Why the request failed, for MLN_RESOURCE_RESPONSE_STATUS_ERROR. */
  mln_resource_error_reason error_reason;
  /** The resource, for MLN_RESOURCE_RESPONSE_STATUS_OK. */
  const uint8_t* bytes MLN_BINDING("length=byte_count;encoding=bytes");
  size_t byte_count;
  /** A description of the error, for MLN_RESOURCE_RESPONSE_STATUS_ERROR. */
  mln_buffer_view error_message;
  /**
   * Whether the library must request the resource again before it reuses this
   * copy.
   */
  bool must_revalidate;
  int64_t modified_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_RESPONSE_MODIFIED");
  int64_t expires_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_RESPONSE_EXPIRES");
  /** Empty when the resource has no ETag. */
  mln_buffer_view etag;
  /** When to retry, for MLN_RESOURCE_RESPONSE_STATUS_ERROR. */
  int64_t retry_after_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_RESPONSE_RETRY_AFTER");
} mln_resource_response;

/**
 * Serves a network request in place of the library.
 *
 * Answer with mln_resource_request_complete(), or hand the request back to the
 * library with mln_resource_request_pass_through(). Either can come now or
 * later, from any thread. A request released without an answer fails.
 *
 * Lifetimes:
 * `request` and the data it points to stay valid until the callback returns.
 * The callback owns `handle`.
 */
typedef void (*mln_resource_provider_callback)(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle MLN_BINDING("ownership=owned")
);
/**
 * Reports that the library no longer needs a resource request.
 *
 * The request no longer accepts an answer, so stop working on it and release
 * it.
 *
 * Lifetimes:
 * The callback borrows `handle` from its owner.
 */
MLN_BINDING("borrowed_owner=handle")
typedef void (*mln_resource_request_cancel_callback)(
  void* user_data, mln_resource_request handle
);

/** A cancel callback, for mln_resource_request_set_cancel_callback(). */
typedef struct mln_resource_request_cancel_handler {
  uint32_t size;
  mln_resource_request_cancel_callback callback;
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_resource_request_cancel_handler MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** A resource provider, for mln_runtime_set_resource_provider(). */
typedef struct mln_resource_provider {
  uint32_t size;
  mln_resource_provider_callback callback;
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
  /**
   * The requests that the provider serves. Without routes, it serves every
   * network request.
   */
  const mln_resource_route* routes MLN_BINDING("length=route_count");
  size_t route_count;
} mln_resource_provider MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/**
 * Creates a runtime.
 *
 * A runtime holds maps and the resource loading, cache, and offline regions
 * that they share. In a browser, create runtimes on a Web Worker, not on the
 * main thread.
 */
MLN_BINDING("blocks=thread_start")
MLN_API mln_status mln_runtime_create(
  const mln_runtime_options* options,
  mln_runtime* out_runtime MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Serves the runtime's network requests through a callback.
 *
 * The provider receives the network requests that its routes match, and the
 * library loads the rest as usual. A provider can serve URL schemes that the
 * library can't load itself. The provider replaces the one set before.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_runtime_set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Removes the runtime's resource provider, so the library loads every request
 * itself.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_runtime_clear_resource_provider(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Answers a resource request.
 *
 * Returns:
 * - MLN_STATUS_INVALID_STATE when the library cancelled the request.
 *
 * Lifetimes:
 * A successful call ends `handle`. After a failed one, the caller still owns
 * it.
 */
MLN_BINDING("consumes=success")
MLN_API mln_status mln_resource_request_complete(
  mln_resource_request handle, const mln_resource_response* response,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Hands a resource request back to the library to load as usual.
 *
 * Returns:
 * - MLN_STATUS_INVALID_STATE when the library cancelled the request.
 *
 * Lifetimes:
 * A successful call ends `handle`. After a failed one, the caller still owns
 * it.
 */
MLN_BINDING("consumes=success")
MLN_API mln_status mln_resource_request_pass_through(
  mln_resource_request handle, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Reads whether the library cancelled a resource request. */
MLN_API mln_status mln_resource_request_is_cancelled(
  mln_resource_request handle, bool* out_cancelled MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Calls a callback when the library cancels a resource request.
 *
 * The callback never runs after the request is answered. A request takes one
 * cancel callback. When the library has already cancelled the request, the
 * call reports that instead and the callback never runs.
 *
 * Lifetimes:
 * When `out_cancelled` is true, the library never calls the handler's
 * `release_user_data`, and the caller still owns its `user_data`.
 */
MLN_BINDING("accepted_unless=out_cancelled")
MLN_API mln_status mln_resource_request_set_cancel_callback(
  mln_resource_request handle,
  const mln_resource_request_cancel_handler* handler,
  bool* out_cancelled MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Releases a resource request. */
MLN_API void mln_resource_request_release(
  mln_resource_request handle
) MLN_NOEXCEPT;

/**
 * Rewrites the URLs of the runtime's network requests through a callback.
 *
 * The transform replaces the one set before.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_runtime_set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Stops rewriting the URLs of the runtime's network requests. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_runtime_clear_resource_transform(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Answers a resource transform request with the URL to load instead.
 *
 * `url` must not be empty.
 */
MLN_BINDING("consumes=success")
MLN_API mln_status mln_resource_transform_request_complete(
  mln_resource_transform_request request, mln_buffer_view url,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Fails a resource transform request without sending it.
 *
 * The library retries a request that failed for a reason it retries, such as
 * MLN_RESOURCE_ERROR_REASON_CONNECTION, and the retry calls the transform
 * again.
 */
MLN_BINDING("consumes=success")
MLN_API mln_status mln_resource_transform_request_fail(
  mln_resource_transform_request request,
  const mln_resource_transform_error* error, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a resource transform request.
 *
 * Releasing a request without an answer fails it with
 * MLN_RESOURCE_ERROR_REASON_OTHER.
 */
MLN_API void mln_resource_transform_request_release(
  mln_resource_transform_request request
) MLN_NOEXCEPT;

/**
 * Adds headers to the runtime's HTTP requests.
 *
 * The transform replaces the one set before. In a browser or on OpenHarmony,
 * serve the requests with mln_runtime_set_resource_provider() instead.
 *
 * Returns:
 * - MLN_STATUS_UNSUPPORTED when the library runs in a browser or on
 *   OpenHarmony.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_runtime_set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Stops adding headers to the runtime's HTTP requests. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_runtime_clear_http_header_transform(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Runs an action on the runtime's ambient cache. */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_run_ambient_cache_operation(
  mln_runtime runtime, mln_ambient_cache_operation operation,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Limits the size of the runtime's ambient cache.
 *
 * `size` is in bytes. Offline regions count toward it, but the library evicts
 * only ambient cache resources to stay within it.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_maximum_ambient_cache_size(
  mln_runtime runtime, uint64_t size, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a runtime.
 *
 * Release the runtime's maps first.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_runtime_release(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a runtime in any state, without waiting.
 *
 * The runtime finishes its teardown after its maps are released.
 */
MLN_API mln_status mln_runtime_dispose(
  mln_runtime runtime, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sends the events of the runtime and its maps to a callback.
 *
 * The callback replaces the one set before.
 */
MLN_BINDING("execution=command")
MLN_API mln_status mln_runtime_set_event_handler(
  mln_runtime runtime, const mln_runtime_event_handler* handler,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Stops sending the events of the runtime and its maps to a callback. */
MLN_BINDING("execution=command")
MLN_API mln_status mln_runtime_clear_event_handler(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Selects the runtime's own events that reach the event callback.
 *
 * Each map selects its events with mln_map_set_event_mask() instead. A runtime
 * starts with every event selected.
 */
MLN_API mln_status mln_runtime_set_event_mask(
  mln_runtime runtime, mln_runtime_event_flags mask,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Reads the runtime's event mask. */
MLN_API mln_status mln_runtime_get_event_mask(
  mln_runtime runtime,
  mln_runtime_event_flags* out_mask MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RUNTIME_H
