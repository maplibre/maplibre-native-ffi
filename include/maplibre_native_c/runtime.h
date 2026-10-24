/**
 * @file maplibre_native_c/runtime.h
 * Public C API declarations for runtime, resources, and events.
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

typedef enum mln_network_status : uint32_t {
  MLN_NETWORK_STATUS_ONLINE = 1,
  MLN_NETWORK_STATUS_OFFLINE = 2,
} mln_network_status;

typedef enum mln_ambient_cache_operation : uint32_t {
  MLN_AMBIENT_CACHE_OPERATION_RESET_DATABASE = 1,
  MLN_AMBIENT_CACHE_OPERATION_PACK_DATABASE = 2,
  MLN_AMBIENT_CACHE_OPERATION_INVALIDATE = 3,
  MLN_AMBIENT_CACHE_OPERATION_CLEAR = 4,
} mln_ambient_cache_operation;

typedef int64_t mln_offline_region_id;

typedef enum mln_offline_region_definition_type : uint32_t {
  MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID = 1,
  MLN_OFFLINE_REGION_DEFINITION_GEOMETRY = 2,
} mln_offline_region_definition_type;

typedef enum mln_offline_region_download_state : uint32_t {
  MLN_OFFLINE_REGION_DOWNLOAD_INACTIVE = 0,
  MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE = 1,
} mln_offline_region_download_state;

/** Offline region status snapshot. */
typedef struct mln_offline_region_status {
  /** One of mln_offline_region_download_state. */
  uint32_t download_state MLN_BINDING("enum=mln_offline_region_download_state");
  uint64_t completed_resource_count;
  uint64_t completed_resource_size;
  uint64_t completed_tile_count;
  uint64_t required_tile_count;
  uint64_t completed_tile_size;
  uint64_t required_resource_count;
  bool required_resource_count_is_precise;
  bool complete;
} mln_offline_region_status;

/**
 * Runtime event types carried by mln_runtime_event.type.
 *
 * The event type selects the meaning of mln_runtime_event.code and the
 * mln_runtime_event_payload member behind mln_runtime_event.payload_type. Event
 * type names below omit their MLN_RUNTIME_EVENT_ prefix and payload type names
 * omit their MLN_RUNTIME_EVENT_PAYLOAD_ prefix.
 *
 * Each value is also a bit index in the subscription masks: the bit for an
 * event type is 1ULL shifted left by the type value. See
 * mln_runtime_event_mask.
 *
 * - MAP_CAMERA_WILL_CHANGE: code is an mln_camera_change_mode; payload NONE.
 * - MAP_CAMERA_IS_CHANGING: code is 0; payload NONE.
 * - MAP_CAMERA_DID_CHANGE: code is an mln_camera_change_mode; payload NONE.
 *   Each MapLibre transition reports its own as it ends, so a camera delta
 *   that pans and also zooms, rotates, or pitches ends with two, and other
 *   transitions may still be animating; see
 *   mln_map_snapshot.transition_in_progress.
 * - MAP_STYLE_LOADED: code is 0; payload NONE.
 * - MAP_LOADING_STARTED: code is 0; payload NONE.
 * - MAP_LOADING_FINISHED: code is 0; payload NONE.
 * - MAP_LOADING_FAILED: code identifies the kind of failure, or is 0, and has
 * no named values; message carries the failure text; payload NONE.
 * - MAP_IDLE: code is 0; payload NONE.
 * - MAP_RENDER_ERROR: code is 0; message carries the error text; payload NONE.
 *   MapLibre reports a failure that it renders past, and a render session
 *   reports the failure that lost its target; see
 *   MLN_RENDER_SESSION_STATE_TARGET_LOST.
 * - MAP_RENDER_FRAME_STARTED: code is 0; payload NONE.
 * - MAP_RENDER_FRAME_FINISHED: code is 0; payload RENDER_FRAME.
 * - MAP_RENDER_MAP_STARTED: code is 0; payload NONE.
 * - MAP_RENDER_MAP_FINISHED: code is 0; payload RENDER_MAP.
 * - MAP_STYLE_IMAGE_MISSING: code is 0; message carries the image ID; payload
 *   NONE.
 * - MAP_TILE_ACTION: code is 0; message carries the source ID; payload
 *   TILE_ACTION.
 * - MAP_SOURCE_CHANGED: a style source changed its description, such as when
 *   its TileJSON arrives with new attribution or tile URLs, or its GeoJSON data
 *   is replaced; code is 0; message carries the source ID; payload NONE. A
 *   style query submitted after the event observes the change.
 * - OFFLINE_REGION_STATUS_CHANGED: code is 0; payload OFFLINE_REGION_STATUS.
 * - OFFLINE_REGION_RESPONSE_ERROR: code is 0; message carries the resource
 *   error text; payload OFFLINE_REGION_RESPONSE_ERROR.
 * - OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: code is 0; payload
 *   OFFLINE_REGION_TILE_COUNT_LIMIT.
 *
 * Values 9, 11, 12, and 22 are retired and stay unused.
 */
typedef enum mln_runtime_event_type : uint32_t {
  MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE = 1,
  MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING = 2,
  MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE = 3,
  MLN_RUNTIME_EVENT_MAP_STYLE_LOADED = 4,
  MLN_RUNTIME_EVENT_MAP_LOADING_STARTED = 5,
  MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED = 6,
  MLN_RUNTIME_EVENT_MAP_LOADING_FAILED = 7,
  MLN_RUNTIME_EVENT_MAP_IDLE = 8,
  MLN_RUNTIME_EVENT_MAP_RENDER_ERROR = 10,
  MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED = 13,
  MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED = 14,
  MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED = 15,
  MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED = 16,
  MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING = 17,
  MLN_RUNTIME_EVENT_MAP_TILE_ACTION = 18,
  MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED = 19,
  MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR = 20,
  MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 21,
  MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED = 23,
} mln_runtime_event_type;

/**
 * Bit values for the map and runtime event subscription masks.
 *
 * Each bit is 1ULL shifted left by the mln_runtime_event_type value it selects,
 * so a host can compute a bit from a type value it decoded from an event.
 *
 * mln_map_set_event_mask() reads the bits in
 * MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS and ignores the rest.
 * mln_runtime_set_event_mask() reads the bits in
 * MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS and ignores the rest. Both entry
 * points accept MLN_RUNTIME_EVENT_MASK_ALL and reject bits outside it. A host
 * that reads a mask, sets one bit, and writes it back keeps every other bit.
 */
typedef enum MLN_BINDING("kind=bitmask") mln_runtime_event_mask : uint64_t {
  /** Selects no event type. */
  MLN_RUNTIME_EVENT_MASK_NONE = 0,
  MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE = 1ULL
    << MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE,
  MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING = 1ULL
    << MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING,
  MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE = 1ULL
    << MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
  MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_STYLE_LOADED,
  MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_LOADING_STARTED,
  MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED,
  MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_LOADING_FAILED,
  MLN_RUNTIME_EVENT_MASK_MAP_IDLE = 1ULL << MLN_RUNTIME_EVENT_MAP_IDLE,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_ERROR,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED,
  MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING = 1ULL
    << MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING,
  MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION = 1ULL
    << MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
  MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED = 1ULL
    << MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
  MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR = 1ULL
    << MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
  MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 1ULL
    << MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
  MLN_RUNTIME_EVENT_MASK_MAP_SOURCE_CHANGED = 1ULL
    << MLN_RUNTIME_EVENT_MAP_SOURCE_CHANGED,
  /** Selects every map-originated event type this version defines. */
  MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS =
      MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE |
    MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING |
    MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE |
    MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED |
    MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED |
    MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED |
    MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED |
    MLN_RUNTIME_EVENT_MASK_MAP_IDLE | MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED |
    MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING |
    MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION |
    MLN_RUNTIME_EVENT_MASK_MAP_SOURCE_CHANGED,
  /** Selects every runtime-originated event type this version defines. */
  MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS =
      MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED |
    MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR |
    MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
  /** Selects every event type this version defines. */
  MLN_RUNTIME_EVENT_MASK_ALL = MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS |
    MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS,
} mln_runtime_event_mask;

/**
 * Source kinds used by mln_runtime_event.source_type.
 *
 * Value 2 is retired and stays unused.
 */
typedef enum mln_runtime_event_source_type : uint32_t {
  MLN_RUNTIME_EVENT_SOURCE_RUNTIME = 0,
  MLN_RUNTIME_EVENT_SOURCE_MAP = 1,
} mln_runtime_event_source_type;

/**
 * Payload kinds used by mln_runtime_event.payload_type.
 *
 * Values 3, 8, and 9 are retired and stay unused.
 */
typedef enum mln_runtime_event_payload_type : uint32_t {
  MLN_RUNTIME_EVENT_PAYLOAD_NONE = 0,
  MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME = 1,
  MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP = 2,
  MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION = 4,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS = 5,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR = 6,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT = 7,
} mln_runtime_event_payload_type;

/** Camera change kinds reported by camera will-change and did-change events. */
typedef enum mln_camera_change_mode : uint32_t {
  /** The camera reached its new value without an animated transition. */
  MLN_CAMERA_CHANGE_MODE_IMMEDIATE = 0,
  /** The camera moved as part of an animated transition. */
  MLN_CAMERA_CHANGE_MODE_ANIMATED = 1,
} mln_camera_change_mode;

/** Render modes reported by render observer events. */
typedef enum mln_render_mode : uint32_t {
  MLN_RENDER_MODE_PARTIAL = 0,
  MLN_RENDER_MODE_FULL = 1,
} mln_render_mode;

/** Tile operations reported by tile observer events. */
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

typedef enum mln_resource_loading_method : uint32_t {
  MLN_RESOURCE_LOADING_METHOD_ALL = 0,
  MLN_RESOURCE_LOADING_METHOD_CACHE_ONLY = 1,
  MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY = 2,
} mln_resource_loading_method;

typedef enum mln_resource_priority : uint32_t {
  MLN_RESOURCE_PRIORITY_REGULAR = 0,
  MLN_RESOURCE_PRIORITY_LOW = 1,
} mln_resource_priority;

typedef enum mln_resource_usage : uint32_t {
  MLN_RESOURCE_USAGE_ONLINE = 0,
  MLN_RESOURCE_USAGE_OFFLINE = 1,
} mln_resource_usage;

typedef enum mln_resource_storage_policy : uint32_t {
  MLN_RESOURCE_STORAGE_POLICY_PERMANENT = 0,
  MLN_RESOURCE_STORAGE_POLICY_VOLATILE = 1,
} mln_resource_storage_policy;

/**
 * How a resource provider answered a request.
 *
 * - OK carries the resource's bytes.
 * - ERROR fails the request with mln_resource_response.error_reason. A tile
 *   whose reason is NOT_FOUND renders as an empty tile; any other failed tile,
 *   and any failed style, reaches the map as a loading error.
 * - NO_CONTENT reports a resource that exists but is empty. A tile renders as
 *   an empty tile.
 * - NOT_MODIFIED answers a revalidation, a request that carries prior_etag or
 *   prior_modified, and keeps the cached copy. When the request also carries
 *   prior_data, the map has not received that copy yet, and this answer
 *   delivers those bytes.
 */
typedef enum mln_resource_response_status : uint32_t {
  MLN_RESOURCE_RESPONSE_STATUS_OK = 0,
  MLN_RESOURCE_RESPONSE_STATUS_ERROR = 1,
  MLN_RESOURCE_RESPONSE_STATUS_NO_CONTENT = 2,
  MLN_RESOURCE_RESPONSE_STATUS_NOT_MODIFIED = 3,
} mln_resource_response_status;

typedef enum mln_resource_error_reason : uint32_t {
  MLN_RESOURCE_ERROR_REASON_NONE = 0,
  MLN_RESOURCE_ERROR_REASON_NOT_FOUND = 1,
  MLN_RESOURCE_ERROR_REASON_SERVER = 2,
  MLN_RESOURCE_ERROR_REASON_CONNECTION = 3,
  MLN_RESOURCE_ERROR_REASON_RATE_LIMIT = 4,
  MLN_RESOURCE_ERROR_REASON_OTHER = 5,
} mln_resource_error_reason;

/**
 * Reads MapLibre Native's process-global network status.
 *
 * On success, out_status receives a mln_network_status value.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when out_status is null.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_network_get_status(
  uint32_t* out_status MLN_BINDING("direction=out;enum=mln_network_status"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Sets MapLibre Native's process-global network status.
 *
 * MLN_NETWORK_STATUS_ONLINE allows network requests.
 * MLN_NETWORK_STATUS_OFFLINE stops new network requests until the status
 * returns to online. Runtime-scoped resource configuration is unchanged.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when status is not a mln_network_status value.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_network_set_status(
  uint32_t status MLN_BINDING("enum=mln_network_status"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/** Options used when creating a runtime. */
typedef struct mln_runtime_options {
  uint32_t size;
  /**
   * Directory root for asset:// URLs. Copied during runtime creation.
   * Empty selects `/android_asset` on Android and `.` elsewhere.
   *
   * On Android, paths under `/android_asset/` read the APK `assets/`
   * directory after mln_android_init. Other paths read the filesystem.
   * Explicit file:// URLs use their own paths, independently of this root.
   */
  mln_buffer_view asset_path MLN_BINDING("optional=empty");
  /** Cache database path. Copied during runtime creation. */
  mln_buffer_view cache_path MLN_BINDING("optional=empty");
} mln_runtime_options;

/**
 * Rendering statistics reported in MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME.
 *
 * mln_event_batch_view.event_size covers the whole event, including these
 * statistics.
 */
typedef struct mln_rendering_stats {
  /** Frame CPU encoding time in seconds. */
  double encoding_time;
  /** Frame CPU rendering time in seconds. */
  double rendering_time;
  /** Number of frames rendered by the native renderer. */
  int64_t frame_count;
  /** Draw calls executed during the most recent frame. */
  int64_t draw_call_count;
  /** Total draw calls executed by the native renderer. */
  int64_t total_draw_call_count;
} mln_rendering_stats;

/** Payload for MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED. */
typedef struct mln_runtime_event_render_frame {
  /** One of mln_render_mode. */
  uint32_t mode MLN_BINDING("enum=mln_render_mode");
  /** Whether MapLibre needs another frame after this one. */
  bool needs_repaint;
  /** Whether symbol placement changed during this frame. */
  bool placement_changed;
  mln_rendering_stats stats;
} mln_runtime_event_render_frame;

/** Payload for MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED. */
typedef struct mln_runtime_event_render_map {
  /** One of mln_render_mode. */
  uint32_t mode MLN_BINDING("enum=mln_render_mode");
} mln_runtime_event_render_map;

/** Overscaled tile identity reported in tile observer events. */
typedef struct mln_tile_id {
  uint32_t overscaled_z;
  int32_t wrap;
  uint32_t canonical_z;
  uint32_t canonical_x;
  uint32_t canonical_y;
} mln_tile_id;

/**
 * Payload for MLN_RUNTIME_EVENT_MAP_TILE_ACTION.
 *
 * The event message carries the source ID.
 */
typedef struct mln_runtime_event_tile_action {
  /** One of mln_tile_operation. */
  uint32_t operation MLN_BINDING("enum=mln_tile_operation");
  mln_tile_id tile_id;
} mln_runtime_event_tile_action;

/** Payload for MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED. */
typedef struct mln_runtime_event_offline_region_status {
  mln_offline_region_id region_id;
  mln_offline_region_status status;
} mln_runtime_event_offline_region_status;

/**
 * Payload for MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR.
 *
 * A download skips a resource that fails with
 * MLN_RESOURCE_ERROR_REASON_NOT_FOUND and continues with the rest of the
 * region. A resource that fails for another reason stays outstanding.
 */
typedef struct mln_runtime_event_offline_region_response_error {
  mln_offline_region_id region_id;
  /** One of mln_resource_error_reason. */
  uint32_t reason MLN_BINDING("enum=mln_resource_error_reason");
} mln_runtime_event_offline_region_response_error;

/** Payload for MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED. */
typedef struct mln_runtime_event_offline_region_tile_count_limit {
  mln_offline_region_id region_id;
  uint64_t limit;
} mln_runtime_event_offline_region_tile_count_limit;

/**
 * Typed event payload carried inline by every event.
 *
 * mln_runtime_event.payload_type selects the member. An event whose payload
 * type is MLN_RUNTIME_EVENT_PAYLOAD_NONE carries zeroed payload bytes.
 *
 * A host that decodes a payload type this version does not define treats the
 * payload as opaque bytes and forwards them unchanged. Those bytes run from the
 * payload's offset within mln_runtime_event to
 * mln_event_batch_view.event_size.
 */
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
} mln_runtime_event_payload;

/**
 * One delivered runtime event.
 *
 * Message bytes are borrowed for the callback or delivery batch lifetime.
 *
 * Step through an array of these by mln_event_batch_view.event_size rather
 * than by the size of this struct: a later version may add a member to
 * mln_runtime_event_payload and widen the stride. Every field below, payload
 * included, keeps its offset across versions.
 */
typedef struct mln_runtime_event {
  /** One of mln_runtime_event_type. */
  uint32_t type MLN_BINDING("enum=mln_runtime_event_type");
  /** One of mln_runtime_event_source_type. */
  uint32_t source_type MLN_BINDING("enum=mln_runtime_event_source_type");
  /**
   * Source handle selected by source_type: an mln_runtime or an mln_map.
   *
   * The value names one object for the life of the process, so a host may
   * compare it against a handle it holds even after that handle is released.
   */
  uint64_t source;
  /**
   * Map snapshot generation that the event reports, or zero when source_type
   * is MLN_RUNTIME_EVENT_SOURCE_RUNTIME.
   *
   * A camera, style, loading-finished, loading-failed, or idle event follows
   * the publication of a snapshot that includes the change it reports and
   * carries that snapshot's generation, so a snapshot read after receiving it
   * shows state at least as new as the event. Every other map event carries the
   * generation the map had published last.
   *
   * An event that a map command raises carries the generation that the
   * command's completion reports. An event at or past a command's completion
   * generation was raised by that command or after it; one below it was raised
   * before the command ran. Events from one map are queued in non-decreasing
   * generation order.
   */
  uint64_t generation;
  /**
   * Secondary event detail whose meaning type selects; see
   * mln_runtime_event_type.
   */
  int32_t code;
  /** One of mln_runtime_event_payload_type. */
  uint32_t payload_type MLN_BINDING("enum=mln_runtime_event_payload_type");
  /** Borrowed UTF-8 message bytes, empty when this event carries no message. */
  mln_buffer_view message;
  /** Typed payload selected by payload_type. */
  mln_runtime_event_payload payload MLN_BINDING(
    "tag=payload_type;empty_variant=MLN_RUNTIME_EVENT_PAYLOAD_NONE"
  );
} mln_runtime_event;

/**
 * A borrowed span of runtime events in one delivery.
 *
 * Step through events by event_size. The event and message pointers remain
 * valid during the callback, or until the delivery batch is released in queue
 * mode.
 */
typedef struct mln_event_batch_view {
  uint32_t size;
  /**
   * Stride of one event in bytes, at least sizeof(mln_runtime_event) in the
   * header a caller compiled against. Index events with this value.
   */
  uint32_t event_size;
  /** Borrowed array of event_count events in queue order. */
  const mln_runtime_event* events
    MLN_BINDING("length=event_count;stride=event_size");
  /** Number of events in events. */
  size_t event_count;
} mln_event_batch_view;

/**
 * Receives selected events after their snapshots have been published.
 *
 * Map events run on the runtime's native scheduler thread. Offline events may
 * run on a MapLibre worker thread. The callback must be thread-safe and return
 * promptly. It may call any-thread C APIs, but must not wait for runtime or
 * graphics-driver work that can only progress on its current thread.
 */
MLN_BINDING("delivery=MLN_DELIVERY_TYPE_RUNTIME_EVENTS")
typedef void (*mln_runtime_event_callback)(
  void* user_data, const mln_event_batch_view* events
);

/**
 * Runtime and map event delivery. Set either callback or queue.
 *
 * Callback mode borrows the events for the call and releases user_data after
 * replacement, clear, or runtime teardown. Queue mode copies events into
 * MLN_DELIVERY_TYPE_RUNTIME_EVENTS deliveries carrying user_data, followed by
 * MLN_DELIVERY_TYPE_RETIRED when the registration retires. Queue mode requires
 * release_user_data to be null.
 */
typedef struct mln_runtime_event_handler {
  uint32_t size;
  mln_runtime_event_callback callback MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  mln_user_data_release release_user_data;
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_runtime_event_handler MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/**
 * The resource kinds that a route or rule matches, one bit per
 * mln_resource_kind value.
 * Registration requires at least one known bit. MLN_RESOURCE_KIND_MASK_ALL
 * selects every resource kind.
 */
typedef enum MLN_BINDING("kind=bitmask") mln_resource_kind_mask : uint32_t {
  MLN_RESOURCE_KIND_MASK_UNKNOWN = 1U << MLN_RESOURCE_KIND_UNKNOWN,
  MLN_RESOURCE_KIND_MASK_STYLE = 1U << MLN_RESOURCE_KIND_STYLE,
  MLN_RESOURCE_KIND_MASK_SOURCE = 1U << MLN_RESOURCE_KIND_SOURCE,
  MLN_RESOURCE_KIND_MASK_TILE = 1U << MLN_RESOURCE_KIND_TILE,
  MLN_RESOURCE_KIND_MASK_GLYPHS = 1U << MLN_RESOURCE_KIND_GLYPHS,
  MLN_RESOURCE_KIND_MASK_SPRITE_IMAGE = 1U << MLN_RESOURCE_KIND_SPRITE_IMAGE,
  MLN_RESOURCE_KIND_MASK_SPRITE_JSON = 1U << MLN_RESOURCE_KIND_SPRITE_JSON,
  MLN_RESOURCE_KIND_MASK_IMAGE = 1U << MLN_RESOURCE_KIND_IMAGE,
  MLN_RESOURCE_KIND_MASK_ALL = MLN_RESOURCE_KIND_MASK_UNKNOWN |
    MLN_RESOURCE_KIND_MASK_STYLE | MLN_RESOURCE_KIND_MASK_SOURCE |
    MLN_RESOURCE_KIND_MASK_TILE | MLN_RESOURCE_KIND_MASK_GLYPHS |
    MLN_RESOURCE_KIND_MASK_SPRITE_IMAGE | MLN_RESOURCE_KIND_MASK_SPRITE_JSON |
    MLN_RESOURCE_KIND_MASK_IMAGE,
} mln_resource_kind_mask;

// This block uses line comments because its examples contain URL patterns that
// a block comment cannot carry.

/// How a rule or route compares its url against a request URL.
///
/// With no flags, a rule compares the complete URL byte for byte.
/// MLN_URL_MATCH_GLOB reads the url as a glob pattern instead:
///
/// - `*` matches a run of any length that contains no `/`, including an empty
///   run.
/// - `**` matches a run of any length, including one that contains `/`.
/// - `?` matches one character other than `/`.
/// - `\` matches the next character literally, and a trailing `\` matches
///   itself.
///
/// Every other byte compares literally. A pattern matches the complete URL, so
/// a pattern that describes a suffix opens with a wildcard. Comparison is
/// case-sensitive either way, and applies no URL parsing or normalization.
///
/// A glob accepts at most 4096 tokens. Each literal byte, escaped byte, `?`,
/// or consecutive run of `*` bytes counts as one token. Registering a longer
/// glob returns MLN_STATUS_INVALID_ARGUMENT. Exact URL matches have no token
/// limit.
///
/// Confining `*` to one path segment is what makes a host pattern hold:
/// `https://*.example.com/**` matches every subdomain of example.com and never
/// `https://attacker.example/x.example.com/tile`. Use `**` wherever a pattern
/// spans path segments, as in `https://tiles.example.com/**` for one host.
typedef enum MLN_BINDING("kind=bitmask") mln_url_match_flags : uint32_t {
  MLN_URL_MATCH_EXACT = 0U,
  /** Reads url as a glob pattern. */
  MLN_URL_MATCH_GLOB = 1U << 0U,
} mln_url_match_flags;

/**
 * One request pattern that a resource provider claims.
 *
 * Routes compare the normalized resolved URL that native loading fetches.
 */
typedef struct mln_resource_route {
  /** The resource kinds to match. */
  uint32_t kinds MLN_BINDING("enum=mln_resource_kind_mask");
  /** A bitwise OR of mln_url_match_flags values. */
  uint32_t flags MLN_BINDING("enum=mln_url_match_flags");
  /** The URL or glob pattern to compare. */
  mln_buffer_view url;
} mln_resource_route;

/** Returns an exact-match route for every resource kind. */
MLN_API mln_resource_route mln_resource_route_default(void) MLN_NOEXCEPT;

/** One HTTP header that a header rule supplies. */
typedef struct mln_http_header {
  mln_buffer_view name;
  mln_buffer_view value;
} mln_http_header;

/**
 * One rule of a native HTTP header transform.
 *
 * The first rule whose kind and URL match supplies all its headers, including
 * an empty header list that stops the search.
 */
typedef struct mln_http_header_rule {
  /** The resource kinds to match. */
  uint32_t kinds MLN_BINDING("enum=mln_resource_kind_mask");
  /** A bitwise OR of mln_url_match_flags values. */
  uint32_t flags MLN_BINDING("enum=mln_url_match_flags");
  /** The URL or glob pattern to compare. */
  mln_buffer_view url;
  /**
   * The headers to set. Registration validates each one as
   * mln_http_header_transform_response_set_header() does.
   */
  const mln_http_header* headers MLN_BINDING("length=header_count");
  size_t header_count;
} mln_http_header_rule;

/** Returns an exact-match header rule for every resource kind, with no headers.
 */
MLN_API mln_http_header_rule mln_http_header_rule_default(void) MLN_NOEXCEPT;

/**
 * Asks the host for the URL of one network resource request.
 *
 * MapLibre waits for the answer before it schedules the request. The host
 * answers through request, inline or later from any thread:
 * mln_resource_transform_request_complete() replaces the URL, and releasing
 * request without completing it keeps the URL. The callback owns request and
 * releases it exactly once, after answering or to keep the URL.
 *
 * The callback runs on a MapLibre network thread. It must be thread-safe and
 * return promptly, and must not wait for runtime work. url is borrowed for the
 * callback. The C API invokes release_user_data after the final callback
 * returns.
 *
 * A transform in queue mode queues a copy as an
 * mln_resource_transform_delivery instead.
 */
MLN_BINDING("delivery=MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM")
typedef void (*mln_resource_transform_callback)(
  void* user_data, uint32_t kind MLN_BINDING("enum=mln_resource_kind"),
  const char* url,
  mln_resource_transform_request request MLN_BINDING("ownership=owned")
);

/**
 * Resource transform state.
 *
 * The struct itself is borrowed for the registering call. Exactly one of
 * callback and queue is set.
 */
typedef struct mln_resource_transform {
  uint32_t size;
  /** Required in callback mode; null in queue mode. */
  mln_resource_transform_callback callback;
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data exactly once for each accepted registration,
   * after replacement, clear, or runtime teardown has retired the registration
   * and every in-flight callback has returned. Requests the callback already
   * received keep their handles.
   *
   * Its threads and its handoff follow
   * mln_resource_provider.release_user_data. Null in queue mode.
   */
  mln_user_data_release release_user_data;
  /**
   * Optional. Queue delivery: each request is copied with its handle into an
   * MLN_DELIVERY_TYPE_RESOURCE_TRANSFORM delivery carrying user_data. A
   * request the queue refuses keeps its URL. After replacement, clear, or
   * runtime teardown retires the registration, the queue receives one
   * MLN_DELIVERY_TYPE_RETIRED delivery carrying user_data, after the
   * registration's last request.
   */
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_resource_transform MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

typedef struct mln_http_header_transform_response {
  uint32_t size;
  /** C API-managed callback context, which callbacks leave unchanged. */
  void* context MLN_BINDING("kind=context;lifetime=call");
} mln_http_header_transform_response MLN_BINDING("kind=callback_response");

/**
 * Sets one outgoing HTTP request header for the current transform invocation.
 *
 * The function copies name and value before returning. A later call using the
 * same case-insensitive name replaces the earlier value.
 *
 * A diagnostic for a rejected header names the header but never includes its
 * value.
 *
 * Returns:
 * - MLN_STATUS_OK when the header was recorded.
 * - MLN_STATUS_INVALID_ARGUMENT when response is null, response->size is too
 *   small, a view has null data with a nonzero size, name is not a valid HTTP
 *   field name, value is not valid UTF-8, value contains a disallowed control
 *   byte, or name identifies a header managed by MapLibre or the platform
 *   transport.
 * - MLN_STATUS_INVALID_STATE when called outside an active HTTP header
 *   transform callback.
 * - MLN_STATUS_NATIVE_ERROR when native allocation fails.
 */
MLN_API mln_status mln_http_header_transform_response_set_header(
  mln_http_header_transform_response* response, mln_buffer_view name,
  mln_buffer_view value, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Adds end-to-end headers to one outgoing HTTP request attempt.
 *
 * MapLibre invokes the callback synchronously on a worker or network thread
 * after resource URL transformation and immediately before the platform HTTP
 * transport starts the attempt. kind is one mln_resource_kind value and url is
 * the transformed URL that the transport will request.
 *
 * The callback and user_data must be thread-safe. The C API invokes
 * release_user_data after the final callback returns. url and out_response are
 * borrowed for the callback duration. Callbacks return promptly and must not
 * wait for work that only their own thread can advance. A non-OK
 * result discards every header collected during the invocation and lets the
 * request proceed unchanged.
 */
MLN_BINDING("failure=MLN_STATUS_NATIVE_ERROR")
typedef mln_status (*mln_http_header_transform_callback)(
  void* user_data, uint32_t kind MLN_BINDING("enum=mln_resource_kind"),
  const char* url,
  mln_http_header_transform_response* out_response MLN_BINDING("direction=out")
);

/**
 * HTTP header transform state: a callback, or native header rules.
 *
 * The struct itself is borrowed for the registering call, which copies every
 * rule, header, and string before it returns. With a null callback, the first
 * rule that matches a request supplies all its headers. A transform requires a
 * callback or at least one rule; clear it through
 * mln_runtime_clear_http_header_transform(). A rules transform carries no
 * user_data or release_user_data.
 */
typedef struct mln_http_header_transform {
  uint32_t size;
  /** The callback, or null to apply rules. */
  mln_http_header_transform_callback callback MLN_BINDING("nullable=true");
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data exactly once for each accepted registration,
   * after replacement, clear, or runtime teardown has retired the registration
   * and every in-flight callback has returned.
   *
   * Its threads and its handoff follow
   * mln_resource_provider.release_user_data.
   */
  mln_user_data_release release_user_data;
  /** Header rules for a transform without a callback; empty with one. */
  const mln_http_header_rule* rules MLN_BINDING("length=rule_count");
  size_t rule_count;
} mln_http_header_transform MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/** Inclusive byte range of a resource request. */
typedef struct mln_resource_range {
  /** First byte offset of the requested range. */
  uint64_t start;
  /** Last byte offset of the requested range, inclusive. */
  uint64_t end;
} mln_resource_range;

/** Field mask values for mln_resource_request_info. */
typedef enum MLN_BINDING("kind=bitmask") mln_resource_request_field : uint32_t {
  /** The request asks only for the bytes in range. */
  MLN_RESOURCE_REQUEST_RANGE = 1U << 0U,
  /** The cached copy being revalidated carries a modification time. */
  MLN_RESOURCE_REQUEST_PRIOR_MODIFIED = 1U << 1U,
  /** The cached copy being revalidated carries an expiration time. */
  MLN_RESOURCE_REQUEST_PRIOR_EXPIRES = 1U << 2U,
} mln_resource_request_field;

typedef struct mln_resource_request_info {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_resource_request_field");
  /**
   * URL entering the network layer, before tile server normalization.
   *
   * It preserves configured URI scheme aliases such as maplibre: and custom
   * schemes, and it is the logical, cache-facing identity of the request.
   * Tile coordinates, glyph ranges, and sprite suffixes are already
   * substituted.
   */
  const char* requested_url MLN_BINDING("nullable=true");
  /**
   * URL to fetch, after resource-kind normalization against the runtime's
   * tile server options and API key.
   *
   * It matches requested_url when no configured alias applies. Providers that
   * replace the built-in network stack fetch this URL.
   *
   * It also matches requested_url when normalization rejects the URL, as a tile
   * server requiring an API key does when no key is configured. Native loading
   * fails such a request, so a provider that only fetches over HTTP checks the
   * scheme before serving it.
   */
  const char* resolved_url MLN_BINDING("nullable=true");
  uint32_t kind MLN_BINDING("enum=mln_resource_kind");
  uint32_t loading_method MLN_BINDING("enum=mln_resource_loading_method");
  uint32_t priority MLN_BINDING("enum=mln_resource_priority");
  uint32_t usage MLN_BINDING("enum=mln_resource_usage");
  uint32_t storage_policy MLN_BINDING("enum=mln_resource_storage_policy");
  mln_resource_range range
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_REQUEST_RANGE");
  int64_t prior_modified_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_REQUEST_PRIOR_MODIFIED");
  int64_t prior_expires_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_REQUEST_PRIOR_EXPIRES");
  const char* prior_etag MLN_BINDING("nullable=true");
  const uint8_t* prior_data
    MLN_BINDING("length=prior_data_size;encoding=bytes");
  size_t prior_data_size;
} mln_resource_request_info;

/** Field mask values for mln_resource_response. */
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_resource_response_field : uint32_t {
  /** The response carries a modification time. */
  MLN_RESOURCE_RESPONSE_MODIFIED = 1U << 0U,
  /** The response carries an expiration time. */
  MLN_RESOURCE_RESPONSE_EXPIRES = 1U << 1U,
  /** An ERROR response carries the earliest time to retry the request. */
  MLN_RESOURCE_RESPONSE_RETRY_AFTER = 1U << 2U,
} mln_resource_response_field;

/**
 * A resource provider's answer to one request.
 *
 * A fields value with bits outside mln_resource_response_field is malformed,
 * and mln_resource_request_complete() converts it to a provider error response.
 */
typedef struct mln_resource_response {
  uint32_t size;
  uint32_t fields MLN_BINDING("enum=mln_resource_response_field");
  uint32_t status MLN_BINDING("enum=mln_resource_response_status");
  uint32_t error_reason MLN_BINDING("enum=mln_resource_error_reason");
  /** Response bytes. May be null only when byte_count is 0. */
  const uint8_t* bytes MLN_BINDING("length=byte_count;encoding=bytes");
  size_t byte_count;
  mln_buffer_view error_message MLN_BINDING("optional=empty");
  bool must_revalidate;
  int64_t modified_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_RESPONSE_MODIFIED");
  int64_t expires_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_RESPONSE_EXPIRES");
  mln_buffer_view etag MLN_BINDING("optional=empty");
  int64_t retry_after_unix_ms
    MLN_BINDING("mask=fields;bit=MLN_RESOURCE_RESPONSE_RETRY_AFTER");
} mln_resource_response;

/**
 * Intercepts a network resource request.
 *
 * The callback runs synchronously on a MapLibre thread. request and its
 * pointed-to fields are borrowed for the callback. The callback owns handle
 * and answers through it, inline or later from any thread: complete the
 * request with mln_resource_request_complete(), hand it to the built-in network
 * loader with mln_resource_request_pass_through(), or release it, which fails
 * the request. Either answer consumes the handle when it returns
 * MLN_STATUS_OK. A handle released during the callback fails the request once
 * the callback returns.
 *
 * The callback must be thread-safe and return quickly. It may call the C API,
 * but must not wait for runtime work that only its own thread can advance. The
 * C API invokes release_user_data after the final callback returns.
 *
 * A provider in queue mode queues a copy of the request with its handle as an
 * mln_resource_request_delivery, for the host to answer later.
 */
MLN_BINDING("delivery=MLN_DELIVERY_TYPE_RESOURCE_REQUEST")
typedef void (*mln_resource_provider_callback)(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle MLN_BINDING("ownership=owned")
);
/**
 * Reports that MapLibre cancelled a C API resource provider request.
 *
 * The callback runs at most once per request, on the thread that discards the
 * request, and only for a request the provider has not completed. That thread
 * is the runtime's native scheduler thread when a committed map or runtime
 * command discards the request, such as a style change or map destruction, and
 * a MapLibre worker thread otherwise; it is never a host thread. The callback
 * must be thread-safe and return quickly. It may call the C API, including
 * mln_resource_request_release(), but must not wait for runtime work that only
 * its own thread can advance.
 *
 * handle is the cancelled request, borrowed for the callback; the provider's
 * own reference to it is unchanged.
 *
 * A handler in queue mode queues a copy as an
 * mln_resource_request_cancel_delivery instead.
 */
MLN_BINDING(
  "delivery=MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL;"
  "borrowed_owner=handle"
)
typedef void (*mln_resource_request_cancel_callback)(
  void* user_data, mln_resource_request handle
);

/**
 * Cancel callback or queue state for one handled resource request.
 *
 * The struct itself is borrowed for the registering call. Exactly one of
 * callback and queue is set.
 */
typedef struct mln_resource_request_cancel_handler {
  uint32_t size;
  /** Required in callback mode; null in queue mode. */
  mln_resource_request_cancel_callback callback;
  /** Opaque host token in queue mode; native code never dereferences it. */
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data exactly once for an accepted registration,
   * after the callback can no longer run: when the callback returns, on the
   * thread that ran it, or when the request is released without the callback
   * having run, on the releasing thread. It never runs for a rejected
   * registration or for a request that was already cancelled. Null in queue
   * mode.
   */
  mln_user_data_release release_user_data;
  /**
   * Optional. Queue delivery in place of callback: a cancel is copied into one
   * MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL delivery carrying user_data, and
   * one MLN_DELIVERY_TYPE_RETIRED delivery follows where callback mode runs
   * release_user_data. The registration rejects an invalid queue handle with
   * MLN_STATUS_INVALID_ARGUMENT and a released queue with
   * MLN_STATUS_INVALID_STATE.
   */
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
} mln_resource_request_cancel_handler MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/**
 * Resource provider state.
 *
 * The struct itself is borrowed for the registering call, which copies every
 * route and string before it returns. Exactly one of callback and queue is
 * set. Routes apply in both modes: with routes, a request that matches none
 * passes through to native loading without reaching the provider. Without
 * routes, every network request reaches the provider.
 */
typedef struct mln_resource_provider {
  uint32_t size;
  /**
   * Answers each request that the routes leave to the provider.
   *
   * Required in callback mode; null in queue mode.
   */
  mln_resource_provider_callback callback;
  void* user_data MLN_BINDING("kind=context");
  /**
   * Optional. Releases user_data exactly once for each accepted registration,
   * after replacement, clear, or runtime teardown has retired the registration
   * and every in-flight callback has returned.
   *
   * It may run on a runtime, worker, network, or closing thread, and never runs
   * for a rejected registration. Another thread may retire an accepted
   * registration before its registration function returns, so the caller
   * transfers user_data before entering that function and reclaims it only
   * when the registration is rejected. Null in queue mode.
   */
  mln_user_data_release release_user_data;
  /**
   * Optional. Queue delivery: native code queues a copy of each request with
   * its handle as an MLN_DELIVERY_TYPE_RESOURCE_REQUEST delivery carrying
   * user_data. A request the queue refuses, because the queue was released or
   * the copy could not be allocated, fails without native loading, except
   * during process exit, which passes it through. After replacement, clear, or
   * runtime teardown retires the registration, the queue receives one
   * MLN_DELIVERY_TYPE_RETIRED delivery carrying user_data, after the
   * registration's last request delivery.
   */
  mln_delivery_queue queue MLN_BINDING("kind=delivery_queue");
  /** The requests that reach the provider; empty to claim every request. */
  const mln_resource_route* routes MLN_BINDING("length=route_count");
  size_t route_count;
} mln_resource_provider MLN_BINDING(
  "kind=callback_registration;release=release_user_data"
);

/**
 * Returns runtime options initialized for this C API version.
 */
MLN_API mln_runtime_options mln_runtime_options_default(void) MLN_NOEXCEPT;

/**
 * Creates a runtime.
 *
 * The call blocks until initialization completes. In the browser, call from a
 * Web Worker; the main thread cannot block. The output must point to the null
 * handle and receives ownership of the runtime on success.
 *
 * Returns:
 * - MLN_STATUS_OK when out_runtime receives an owned runtime.
 * - MLN_STATUS_INVALID_ARGUMENT when options is null, options->size is too
 *   small, a path view has null data with a nonzero size or embedded NUL,
 *   or out_runtime is null or does not point to the null handle.
 * - MLN_STATUS_WRONG_THREAD when called on the browser main thread.
 * - MLN_STATUS_NATIVE_ERROR when runtime initialization fails.
 */
MLN_BINDING("blocks=thread_start")
MLN_API mln_status mln_runtime_create(
  const mln_runtime_options* options,
  mln_runtime* out_runtime MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Registers or replaces a runtime-scoped network resource provider.
 *
 * The provider handles network requests. Built-in non-network schemes such as
 * file, asset, mbtiles, and pmtiles bypass it.
 *
 * The provider sees every network request, including one the ambient cache
 * holds a fresh copy of. MapLibre delivers the cached copy first, then asks the
 * provider to revalidate it, with prior_etag, prior_modified, and
 * prior_expires describing that copy, even before the copy expires. The
 * provider answers NOT_MODIFIED to keep it.
 *
 * The function copies the provider shape and accepts the change from any
 * thread. Execution is ordered with every other command for this runtime.
 * The completion reports the terminal command disposition.
 *
 * With a non-null release_user_data, MLN_STATUS_OK transfers responsibility for
 * releasing user_data to the C API. With a null release_user_data, the caller
 * keeps user_data valid until a committed replacement or clear command, or
 * until native runtime teardown finishes. Requests that the previous provider
 * already handled retain their request handles.
 *
 * A URL with an unrecognized scheme, such as jar:file:, reaches this callback
 * and completes as an HTTP error when the provider passes it through.
 *
 * In queue mode, no host code runs on the requesting thread: each request
 * reaches the provider's queue, and the host completes it or passes it through
 * after draining.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, provider is
 *   null, provider->size is too small, neither or both of callback and queue
 *   are set, release_user_data is set in queue mode, queue is an invalid
 *   handle, a route is invalid, or completion is invalid. A route is invalid
 *   when routes is null with a nonzero route_count, or the route's url is null
 *   or its kinds are zero or hold unknown bits,
 *   or its flags hold an unknown bit; the diagnostic names the route's index.
 * - MLN_STATUS_INVALID_STATE when runtime or queue has been released, or
 *   runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Clears the runtime-scoped network resource provider.
 *
 * The function accepts the change from any thread and orders it with every
 * other command for this runtime. The completion reports the terminal command
 * disposition. The C API invokes the previous provider's release_user_data
 * after every in-flight callback returns. Later requests use the built-in
 * network loader. Requests that the previous provider already handled retain
 * their handles.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_clear_resource_provider(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Completes a C API resource provider request and consumes its handle.
 *
 * This function may be called inline from the provider callback or later from
 * any thread. The C API copies all response bytes and strings before returning.
 *
 * MLN_STATUS_OK consumes the handle, as mln_resource_request_release() would,
 * so the caller does not release it; a request's cancel registration retires
 * with it. Any other status leaves the handle with the caller, who releases it
 * or answers again. Completion after cancellation or with null arguments does
 * not invoke MapLibre's resource callback. Malformed response contents are
 * converted to provider error responses and still complete the request.
 * Invalid string views are rejected before the request is completed.
 *
 * Returns:
 * - MLN_STATUS_OK when the response was accepted for asynchronous delivery.
 * - MLN_STATUS_INVALID_ARGUMENT when handle is an invalid handle, response is
 *   null, or a response string has null data with a nonzero size or embedded
 *   NUL.
 * - MLN_STATUS_INVALID_STATE when handle has been released or consumed, or the
 *   request was cancelled or can no longer accept a response.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_BINDING("consumes=success")
MLN_API mln_status mln_resource_request_complete(
  mln_resource_request handle, const mln_resource_response* response,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Hands an outstanding provider request to the built-in network loader and
 * consumes its handle.
 *
 * This is how a provider declines a request. Call from any thread, inline in
 * the provider callback or after it returns, or after taking a queued request.
 * Native loading starts on the requesting MapLibre thread and applies the
 * current native transforms. The request bypasses the provider when it
 * resumes.
 *
 * MLN_STATUS_OK consumes the handle, as mln_resource_request_release() would,
 * without cancelling native loading; the request's cancel registration retires
 * with it, and its cancel callback never runs. Any other status leaves the
 * handle with the caller. Completion, cancellation, release, and pass-through
 * are serialized: only an outstanding request can pass through. Cancellation
 * that wins first returns MLN_STATUS_INVALID_STATE; cancellation after
 * acceptance cancels native loading as usual.
 *
 * Returns:
 * - MLN_STATUS_OK when native loading was scheduled.
 * - MLN_STATUS_INVALID_ARGUMENT when handle is invalid.
 * - MLN_STATUS_INVALID_STATE when handle has been released or consumed, or the
 *   request was cancelled.
 * - MLN_STATUS_NATIVE_ERROR when scheduling fails; the request stays
 *   outstanding.
 */
MLN_BINDING("consumes=success")
MLN_API mln_status mln_resource_request_pass_through(
  mln_resource_request handle, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reports whether MapLibre has cancelled a C API resource provider request.
 *
 * This function may be called from any thread while the provider still owns the
 * handle. A cancelled request no longer wants a response; later completion is
 * ignored with MLN_STATUS_INVALID_STATE, and the provider releases the handle.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when handle is an invalid handle, or
 *   out_cancelled is null.
 * - MLN_STATUS_INVALID_STATE when handle has been released.
 */
MLN_API mln_status mln_resource_request_is_cancelled(
  mln_resource_request handle, bool* out_cancelled MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Registers a callback that runs when MapLibre cancels a C API resource
 * provider request.
 *
 * This function may be called from any thread while the provider still owns the
 * handle. A request accepts one registration: a later call fails and leaves the
 * first registration in place.
 *
 * MLN_STATUS_OK with out_cancelled false transfers the handler's callback,
 * user_data, and release_user_data to the C API. The callback runs at most
 * once. The handler struct itself is borrowed for the call.
 *
 * A request that is already cancelled and not completed stores nothing and
 * reports true through out_cancelled. The caller keeps user_data and handles
 * the cancellation itself, and neither callback runs. Any other request reports
 * false. This function never invokes either callback.
 *
 * mln_resource_request_release() waits for a cancel callback running on another
 * thread to return, so the callback and user_data are unused once release
 * returns. Called from inside the callback, release returns without waiting.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when handle is an invalid handle, when handler
 *   or out_cancelled is null, when handler->size is too small, when handler
 *   sets neither or both of callback and queue, or when handler->queue is an
 *   invalid handle.
 * - MLN_STATUS_INVALID_STATE when handle or handler->queue has been released,
 *   or the request already has a cancel callback.
 */
MLN_BINDING("accepted_unless=out_cancelled")
MLN_API mln_status mln_resource_request_set_cancel_callback(
  mln_resource_request handle,
  const mln_resource_request_cancel_handler* handler,
  bool* out_cancelled MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases the provider's resource request handle without answering it.
 *
 * Release a handle that mln_resource_request_complete() and
 * mln_resource_request_pass_through() did not consume, such as one whose
 * request was cancelled. Releasing a request that is not cancelled fails it
 * with an MLN_RESOURCE_ERROR_REASON_OTHER error, which reaches the map
 * asynchronously; the provider callback may release its handle inline.
 * A request handle is not a child of its runtime, whether the provider still
 * holds it or its failure is still queued, so it never makes
 * mln_runtime_release() or mln_map_release() return MLN_STATUS_INVALID_STATE.
 * Passing MLN_HANDLE_NULL is a no-op, as is passing a handle this call already
 * released. A released handle reports MLN_STATUS_INVALID_STATE from every
 * other request entry point, including from a copy another thread holds.
 */
MLN_API void mln_resource_request_release(
  mln_resource_request handle
) MLN_NOEXCEPT;

/**
 * Registers or updates a runtime-scoped URL transform for network resources.
 *
 * The transform applies to built-in network loading, including nested PMTiles
 * network range requests. It does not apply to file, asset, database, MBTiles,
 * or requests handled by a registered C API provider.
 *
 * The function copies the transform shape and accepts the change from any
 * thread. Execution is ordered with every other command for this runtime.
 * The completion reports the terminal command disposition.
 * With a non-null release_user_data, MLN_STATUS_OK transfers responsibility for
 * releasing user_data to the C API. With a null release_user_data, the caller
 * keeps user_data valid until a committed replacement or clear command, or
 * until native runtime teardown finishes.
 *
 * In queue mode, no host code runs on a MapLibre thread: each request reaches
 * the transform's queue, and the host answers its handle after draining.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, transform is
 *   null, transform->size is too small, neither or both of callback and queue
 *   are set, release_user_data is set in queue mode, queue is an invalid
 *   handle, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime or queue has been released, or
 *   runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Clears the runtime-scoped URL transform for network resources.
 *
 * The function accepts the change from any thread and orders it with every
 * other command for this runtime. The completion reports the terminal command
 * disposition. The C API invokes the previous transform's release_user_data
 * after every in-flight callback returns. Later network resource URLs pass
 * through unchanged; requests the previous transform already received wait
 * for their answers.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_clear_resource_transform(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Answers a resource transform request with a replacement URL.
 *
 * Call from any thread, inline in the transform callback or later. The C API
 * copies url before returning. A request takes one answer, and the caller still
 * releases it afterward. A request that MapLibre cancelled while it waited
 * accepts the answer and discards it.
 *
 * Returns:
 * - MLN_STATUS_OK when the answer was accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when request is an invalid handle, url is
 *   empty, or url has null data with a nonzero size or contains embedded NUL.
 * - MLN_STATUS_INVALID_STATE when request has been released or already
 *   answered.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_resource_transform_request_complete(
  mln_resource_transform_request request, mln_buffer_view url,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Releases a resource transform request.
 *
 * Release each request exactly once. A request released without an answer
 * keeps its URL. Passing MLN_HANDLE_NULL is a no-op, as is passing a handle
 * this call already released.
 */
MLN_API void mln_resource_transform_request_release(
  mln_resource_transform_request request
) MLN_NOEXCEPT;

/**
 * Registers or replaces the runtime-scoped outgoing HTTP header transform.
 *
 * The transform applies only to requests that reach the built-in HTTP client,
 * including online and offline requests and nested network-backed PMTiles
 * range requests. Cache hits, non-HTTP schemes, and provider-handled requests
 * do not invoke it.
 *
 * The function copies the transform shape and accepts the change from any
 * thread. Execution is ordered with every other command for this runtime.
 * The completion reports the terminal command disposition.
 * With a non-null release_user_data, MLN_STATUS_OK transfers responsibility for
 * releasing user_data to the C API. With a null release_user_data, the caller
 * keeps user_data valid until a committed replacement or clear command, or
 * until native runtime teardown finishes.
 *
 * A transform without a callback adds headers through its rules, so a host
 * that cannot run code on a MapLibre thread can use it.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, transform is
 *   null, transform->size is too small, neither a callback nor rules are set,
 *   a callback comes with rules, a rules
 *   transform carries user_data or release_user_data, a rule is invalid, or
 *   completion is invalid. A rule is invalid when rules is null with a nonzero
 *   rule_count, its url is null, its kinds are zero or hold unknown bits, its
 *   flags hold an unknown bit, its headers are null with a nonzero
 *   header_count, or a header breaks a rule of
 *   mln_http_header_transform_response_set_header(). The diagnostic names the
 *   rule's index and never includes a header value.
 * - MLN_STATUS_UNSUPPORTED on OpenHarmony and in the browser, whose HTTP
 *   clients cannot prevent transformed headers from following a cross-origin
 *   redirect. A resource provider serves those requests instead.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Clears the runtime-scoped outgoing HTTP header transform.
 *
 * The function accepts the change from any thread and orders it with every
 * other command for this runtime. The completion reports the terminal command
 * disposition. The C API invokes the previous transform's release_user_data
 * after every in-flight callback returns.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_clear_http_header_transform(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts a MapLibre ambient cache maintenance operation for this runtime.
 *
 * When runtime options omit cache_path, this operates on MapLibre's default
 * in-memory database and its effects are not durable beyond the native database
 * lifetime. The completion reports the terminal status and carries no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, operation is
 *   not an mln_ambient_cache_operation value, or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK when the maintenance operation finished.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_run_ambient_cache_operation(
  mln_runtime runtime,
  uint32_t operation MLN_BINDING("enum=mln_ambient_cache_operation"),
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts a change to this runtime's maximum ambient cache size.
 *
 * size is the ambient cache budget in bytes. MapLibre evicts ambient resources
 * to fit the new budget, so lowering it discards cached resources. Offline
 * regions are not ambient and are unaffected.
 *
 * When runtime options omit cache_path, this operates on MapLibre's default
 * in-memory database and its effects are not durable beyond the native database
 * lifetime. The completion reports the terminal status and carries no value.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when acceptance fails.
 *
 * Completes with:
 * - MLN_STATUS_OK when the budget was applied.
 * - MLN_STATUS_NATIVE_ERROR when the database reports a failure.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_maximum_ambient_cache_size(
  mln_runtime runtime, uint64_t size, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Starts release of a runtime after checking its children.
 *
 * The call rejects a runtime that has live children and leaves it open. A
 * runtime's children are its maps, including one that is still creating.
 * Resource request handles are not children: one that a provider still holds,
 * or has released with its failure still queued, leaves this call free to
 * succeed. A successful call consumes the public handle before returning.
 * Previously accepted work and native teardown continue in submission order;
 * callback user data remains native-owned until its release callback runs.
 * This function may be called from any thread.
 *
 * The completion runs after every earlier accepted submission, including
 * released maps' teardown and each submission's completion and release
 * callback, has finished. The runtime's worker has exited, including its
 * thread-local cleanup, and runtime-owned callback and resource state has
 * retired. The completion must not block. Shared native services remain
 * outside this guarantee; release completion does not permit unloading the
 * library. A host that outlives its runtimes may pass
 * a discarding completion.
 *
 * Items that the runtime's registrations already pushed to a delivery queue
 * stay in that queue, so the completion can run before the host drains them.
 *
 * Teardown may allocate. Allocation failure after acceptance terminates the
 * process. A runtime whose teardown is slow delays other runtimes' release
 * completions.
 *
 * A process may exit at any point, including while runtimes and maps are live
 * and their work is in flight. Render sessions may stay live too, once their
 * graphics calls have ended as described below. Native work may continue until
 * the operating system ends the process. Once exit begins, native code
 * dispatches no further callback that the host registered through this API,
 * release callbacks included. A callback that native code dispatched before
 * exit began may still start or be running after it. A completion or release
 * callback that is still pending when exit begins never runs, so code that
 * runs after exit begins must not wait for one. Plugin code is native code,
 * not a host callback in this sense; see plugin.h.
 *
 * When exit begins depends on the platform. On Windows, it begins when the
 * operating system ends the process's other threads, after every exit handler
 * and static destructor that the host registered has run. Elsewhere, it begins
 * when the C runtime runs the exit handler that the library registers when it
 * first creates a runtime or installs the log callback; exit handlers and
 * static destructors that the host registered before that run after it. A
 * process that ends without running exit handlers, such as through _exit(),
 * ends native threads with it.
 *
 * A host that tears down state its callbacks use before exit begins must stop
 * native callbacks into that state first. That covers a language runtime that
 * shuts down before the C runtime's exit handlers run, such as an interpreter
 * that finalizes first; on Windows, every exit handler and static destructor
 * that the host registered; and elsewhere, those that the host registered
 * after the library's exit handler. Releasing each runtime and waiting for its
 * release completion stops its callbacks.
 *
 * Before the process calls exit() or returns from main, the host must end the
 * graphics calls of every render session it attached. To end them, abandon
 * the session, or detach it and wait for the detach completion.
 * A host that drives a session on its own graphics thread stops driver service
 * first. Abandon returns once the session's in-flight driver call has ended,
 * so an exit path can end a session that is mid-frame. A disposed session's
 * graphics calls have ended once its wake release callbacks have run. A browser
 * WebGPU readback whose copy is still mapping is the one exception to both
 * paths: the library releases its own staging buffer when the graphics
 * thread's event loop delivers the map; see mln_render_session_abandon().
 *
 * Returns:
 * - MLN_STATUS_OK when the handle was consumed.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime has been released, or the runtime has
 *   a live child or is already closing.
 * - MLN_STATUS_NATIVE_ERROR when the completion could not be allocated.
 */
MLN_BINDING("execution=lifecycle")
MLN_API mln_status mln_runtime_release(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Consumes a runtime handle without observing its asynchronous retirement.
 *
 * A successful call consumes the handle before returning. A failed call
 * retains caller ownership. Retirement waits for live children and accepted
 * work. Existing children retain their own cleanup obligations; dispose or
 * release them to let the runtime finish retiring. Teardown may
 * allocate; allocation failure after acceptance terminates the process.
 *
 * Returns:
 * - MLN_STATUS_OK when the handle was consumed.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is already
 *   closing.
 */
MLN_API mln_status mln_runtime_dispose(
  mln_runtime runtime, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Selects how runtime and map events reach the host.
 *
 * MLN_STATUS_OK transfers the handler's user_data to the runtime. A rejected
 * call leaves it with the caller. Events queued before replacement stay on
 * their original queue. Before a handler is set, events are dropped. Set a
 * handler before creating maps or observing offline regions to receive their
 * first events.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime or handler is invalid, or the
 *   completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime or the handler's queue is released.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_set_event_handler(
  mln_runtime runtime, const mln_runtime_event_handler* handler,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Stops event delivery and retires the previous handler.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime or completion is invalid.
 * - MLN_STATUS_INVALID_STATE when runtime is released.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_BINDING("execution=operation")
MLN_API mln_status mln_runtime_clear_event_handler(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Selects which runtime-scoped event types this runtime queues.
 *
 * A runtime queues an offline event when this mask selects its type. Region
 * status, response error, and tile count limit events also require the region
 * to be observed with mln_runtime_set_offline_region_observed(), so this
 * mask narrows that subscription rather than replacing it.
 *
 * A runtime that has not been narrowed selects every runtime-scoped event type
 * this library reports, which covers types a caller's header may not
 * declare. A new mask applies to later events and keeps the events already
 * queued.
 *
 * This call accepts only bits in MLN_RUNTIME_EVENT_MASK_ALL. It reads the bits
 * in MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS and ignores map-originated bits.
 * mln_runtime_get_event_mask() reports the value last set, so a host reads it,
 * changes one bit, and writes it back.
 *
 * Changing this mask does not affect one-shot completion or result ownership.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or mask
 *   holds a bit outside MLN_RUNTIME_EVENT_MASK_ALL.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_runtime_set_event_mask(
  mln_runtime runtime, uint64_t mask MLN_BINDING("enum=mln_runtime_event_mask"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Reports which runtime-scoped event types this runtime queues.
 *
 * The value is the mask last set, including bits outside
 * MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS that this runtime ignores. A
 * runtime that has not been narrowed reports MLN_RUNTIME_EVENT_MASK_ALL as this
 * library defines it.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is an invalid handle, or out_mask
 *   is null.
 * - MLN_STATUS_INVALID_STATE when runtime has been released or is closing.
 * - MLN_STATUS_NATIVE_ERROR when native code fails.
 */
MLN_API mln_status mln_runtime_get_event_mask(
  mln_runtime runtime,
  uint64_t* out_mask MLN_BINDING("direction=out;enum=mln_runtime_event_mask"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RUNTIME_H
