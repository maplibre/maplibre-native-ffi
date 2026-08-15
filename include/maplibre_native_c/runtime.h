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
  uint32_t size;
  /** One of mln_offline_region_download_state. */
  uint32_t download_state;
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
 * - MAP_STYLE_LOADED: code is 0; payload NONE.
 * - MAP_LOADING_STARTED: code is 0; payload NONE.
 * - MAP_LOADING_FINISHED: code is 0; payload NONE.
 * - MAP_LOADING_FAILED: code is the ordinal of MapLibre Native's internal map
 *   load error kind, which this API does not name as an enum, and is 0 when the
 *   failure came from a style-loading exception raised inside a C API call.
 *   Read message for the failure text in both cases; payload NONE.
 * - MAP_IDLE: code is 0; payload NONE.
 * - MAP_RENDER_UPDATE_AVAILABLE: code is 0; payload NONE.
 * - MAP_RENDER_ERROR: code is 0; message carries the error text; payload NONE.
 * - MAP_STILL_IMAGE_FINISHED: code is 0; payload NONE.
 * - MAP_STILL_IMAGE_FAILED: code is 0; message carries the error text; payload
 *   NONE.
 * - MAP_RENDER_FRAME_STARTED: code is 0; payload NONE.
 * - MAP_RENDER_FRAME_FINISHED: code is 0; payload RENDER_FRAME.
 * - MAP_RENDER_MAP_STARTED: code is 0; payload NONE.
 * - MAP_RENDER_MAP_FINISHED: code is 0; payload RENDER_MAP.
 * - MAP_STYLE_IMAGE_MISSING: code is 0; message carries the image ID; payload
 *   NONE.
 * - MAP_TILE_ACTION: code is 0; message carries the source ID; payload
 *   TILE_ACTION.
 * - OFFLINE_REGION_STATUS_CHANGED: code is 0; payload OFFLINE_REGION_STATUS.
 * - OFFLINE_REGION_RESPONSE_ERROR: code is 0; message carries the resource
 *   error text; payload OFFLINE_REGION_RESPONSE_ERROR.
 * - OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: code is 0; payload
 *   OFFLINE_REGION_TILE_COUNT_LIMIT.
 * - COMMAND_FINISHED: code is the final mln_status; message carries a copied
 *   diagnostic; payload COMMAND_FINISHED.
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
  MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE = 9,
  MLN_RUNTIME_EVENT_MAP_RENDER_ERROR = 10,
  MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED = 11,
  MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED = 12,
  MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED = 13,
  MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED = 14,
  MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED = 15,
  MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED = 16,
  MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING = 17,
  MLN_RUNTIME_EVENT_MAP_TILE_ACTION = 18,
  MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED = 19,
  MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR = 20,
  MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED = 21,
  MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED = 23,
  MLN_RUNTIME_EVENT_COMMAND_FINISHED = 24,
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
 * points therefore accept MLN_RUNTIME_EVENT_MASK_ALL, and a host that reads a
 * mask, sets one bit, and writes it back keeps every other bit.
 */
typedef enum mln_runtime_event_mask : uint64_t {
  /** Selects no event type. */
  MLN_RUNTIME_EVENT_MASK_NONE = 0,
  MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE =
    1ULL << MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE,
  MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING =
    1ULL << MLN_RUNTIME_EVENT_MAP_CAMERA_IS_CHANGING,
  MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE =
    1ULL << MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
  MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED =
    1ULL << MLN_RUNTIME_EVENT_MAP_STYLE_LOADED,
  MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED =
    1ULL << MLN_RUNTIME_EVENT_MAP_LOADING_STARTED,
  MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED =
    1ULL << MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED,
  MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED =
    1ULL << MLN_RUNTIME_EVENT_MAP_LOADING_FAILED,
  MLN_RUNTIME_EVENT_MASK_MAP_IDLE = 1ULL << MLN_RUNTIME_EVENT_MAP_IDLE,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE =
    1ULL << MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR =
    1ULL << MLN_RUNTIME_EVENT_MAP_RENDER_ERROR,
  MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED =
    1ULL << MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FINISHED,
  MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED =
    1ULL << MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED =
    1ULL << MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_STARTED,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED =
    1ULL << MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED =
    1ULL << MLN_RUNTIME_EVENT_MAP_RENDER_MAP_STARTED,
  MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED =
    1ULL << MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED,
  MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING =
    1ULL << MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING,
  MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION = 1ULL
                                           << MLN_RUNTIME_EVENT_MAP_TILE_ACTION,
  MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED =
    1ULL << MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED,
  MLN_RUNTIME_EVENT_MASK_COMMAND_FINISHED =
    1ULL << MLN_RUNTIME_EVENT_COMMAND_FINISHED,
  MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED =
    1ULL << MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED,
  MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR =
    1ULL << MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR,
  MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED =
    1ULL << MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED,
  /** Selects every map-originated event type this version defines. */
  MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS =
    MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_WILL_CHANGE |
    MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_IS_CHANGING |
    MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE |
    MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED |
    MLN_RUNTIME_EVENT_MASK_MAP_LOADING_STARTED |
    MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED |
    MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FAILED |
    MLN_RUNTIME_EVENT_MASK_MAP_IDLE |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_ERROR |
    MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FINISHED |
    MLN_RUNTIME_EVENT_MASK_MAP_STILL_IMAGE_FAILED |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_STARTED |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_STARTED |
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_MAP_FINISHED |
    MLN_RUNTIME_EVENT_MASK_MAP_STYLE_IMAGE_MISSING |
    MLN_RUNTIME_EVENT_MASK_MAP_TILE_ACTION |
    MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED |
    MLN_RUNTIME_EVENT_MASK_COMMAND_FINISHED,
  /** Selects every runtime-originated event type this version defines. */
  MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS =
    MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED |
    MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_RESPONSE_ERROR |
    MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED |
    MLN_RUNTIME_EVENT_MASK_COMMAND_FINISHED,
  /** Selects every event type this version defines. */
  MLN_RUNTIME_EVENT_MASK_ALL = MLN_RUNTIME_EVENT_MASK_ALL_MAP_EVENTS |
                               MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS,
} mln_runtime_event_mask;

/** Source kinds used by mln_runtime_event.source_type. */
typedef enum mln_runtime_event_source_type : uint32_t {
  MLN_RUNTIME_EVENT_SOURCE_RUNTIME = 0,
  MLN_RUNTIME_EVENT_SOURCE_MAP = 1,
  MLN_RUNTIME_EVENT_SOURCE_PROJECTION = 2,
} mln_runtime_event_source_type;

/**
 * Payload kinds used by mln_runtime_event.payload_type.
 *
 * Values 3 and 8 are retired and no version reuses them. Value 3 was a
 * style-image-missing payload whose only content was the image ID that the
 * event message carries. Value 8 was an offline-operation completion payload;
 * operations now report completion through notification sources.
 */
typedef enum mln_runtime_event_payload_type : uint32_t {
  MLN_RUNTIME_EVENT_PAYLOAD_NONE = 0,
  MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME = 1,
  MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP = 2,
  MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION = 4,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS = 5,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR = 6,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT = 7,
  MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED = 9,
  MLN_RUNTIME_EVENT_PAYLOAD_COMMAND_FINISHED = 10,
} mln_runtime_event_payload_type;

/** Camera change kinds reported by camera will-change and did-change events. */
typedef enum mln_camera_change_mode : uint32_t {
  /** The camera reached its new value without an animated transition. */
  MLN_CAMERA_CHANGE_MODE_IMMEDIATE = 0,
  /** The camera moved as part of an animated transition. */
  MLN_CAMERA_CHANGE_MODE_ANIMATED = 1,
} mln_camera_change_mode;

/** Terminal dispositions reported by command-finished events. */
typedef enum mln_command_disposition : uint32_t {
  MLN_COMMAND_DISPOSITION_COMMITTED = 0,
  MLN_COMMAND_DISPOSITION_SUPERSEDED = 1,
  MLN_COMMAND_DISPOSITION_FAILED = 2,
  MLN_COMMAND_DISPOSITION_CANCELLED = 3,
} mln_command_disposition;

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

typedef enum mln_resource_provider_decision : uint32_t {
  MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH = 0,
  MLN_RESOURCE_PROVIDER_DECISION_HANDLE = 1,
} mln_resource_provider_decision;

/**
 * Reads MapLibre Native's process-global network status.
 *
 * On success, out_status receives a mln_network_status value.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when out_status is null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_network_status_get(uint32_t* out_status) MLN_NOEXCEPT;

/**
 * Sets MapLibre Native's process-global network status.
 *
 * MLN_NETWORK_STATUS_ONLINE allows HTTP and HTTPS requests and wakes native
 * subscribers when transitioning from offline. MLN_NETWORK_STATUS_OFFLINE makes
 * MapLibre's online source stop starting network requests until reachability
 * returns. Runtime-scoped resource configuration is unchanged.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when status is not a mln_network_status value.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_network_status_set(uint32_t status) MLN_NOEXCEPT;

/** Options used when creating a runtime. */
typedef struct mln_runtime_options {
  uint32_t size;
  /** No flags are currently defined. Must be zero. */
  uint32_t flags;
  /** Filesystem root for asset:// URLs. Copied during runtime creation. */
  const char* asset_path;
  /** Cache database path. Copied during runtime creation. */
  const char* cache_path;
  /**
   * Runtime-scoped event types this runtime queues, as a bitwise OR of
   * mln_runtime_event_mask values.
   *
   * This field is always read, so set it explicitly.
   * MLN_RUNTIME_EVENT_MASK_ALL selects every event type this library reports
   * and is the value mln_runtime_options_default() populates.
   * MLN_RUNTIME_EVENT_MASK_NONE queues none. See
   * mln_runtime_set_event_mask().
   */
  uint64_t event_mask;
  /**
   * Receiver-scoped source for runtime-event readiness and inherited operation
   * completion. The runtime retains the source association until destruction.
   */
  mln_notification_source notification_source;
} mln_runtime_options;

/**
 * Rendering statistics reported in MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME.
 *
 * This struct has no size field, because it is a member of
 * mln_runtime_event_payload. mln_runtime_event_batch_view.event_size covers the
 * whole event, including its payload.
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
  uint32_t mode;
  /** Whether MapLibre needs another frame after this one. */
  bool needs_repaint;
  /** Whether symbol placement changed during this frame. */
  bool placement_changed;
  mln_rendering_stats stats;
} mln_runtime_event_render_frame;

/** Payload for MLN_RUNTIME_EVENT_MAP_RENDER_MAP_FINISHED. */
typedef struct mln_runtime_event_render_map {
  /** One of mln_render_mode. */
  uint32_t mode;
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
  uint32_t operation;
  mln_tile_id tile_id;
} mln_runtime_event_tile_action;

/**
 * Payload for MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED.
 *
 * See mln_animation_options.transition_id for how a caller stamps an identity
 * onto a camera transition and what terminal outcomes this event covers.
 */
typedef struct mln_runtime_event_camera_transition_finished {
  /**
   * The transition_id the caller set on the mln_animation_options that started
   * this transition.
   */
  uint64_t transition_id;
} mln_runtime_event_camera_transition_finished;

/** Payload for MLN_RUNTIME_EVENT_COMMAND_FINISHED. */
typedef struct mln_runtime_event_command_finished {
  uint64_t command_id;
  /** One of mln_command_disposition. */
  uint32_t disposition;
  uint32_t reserved;
  /**
   * The map snapshot generation the commit published, or zero when the command
   * committed no generation. A later mln_map_snapshot_get() that reports this
   * generation or a newer one observes the commit.
   */
  uint64_t generation;
} mln_runtime_event_command_finished;

/** Payload for MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED. */
typedef struct mln_runtime_event_offline_region_status {
  mln_offline_region_id region_id;
  /**
   * Region status. This member keeps its own size field, because
   * mln_offline_region_status is also the output struct of
   * mln_runtime_offline_region_get_status_take_result().
   */
  mln_offline_region_status status;
} mln_runtime_event_offline_region_status;

/** Payload for MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR. */
typedef struct mln_runtime_event_offline_region_response_error {
  mln_offline_region_id region_id;
  /** One of mln_resource_error_reason. */
  uint32_t reason;
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
 * mln_runtime_event_batch_view.event_size.
 */
typedef union mln_runtime_event_payload {
  mln_runtime_event_render_frame render_frame;
  mln_runtime_event_render_map render_map;
  mln_runtime_event_tile_action tile_action;
  mln_runtime_event_offline_region_status offline_region_status;
  mln_runtime_event_offline_region_response_error offline_region_response_error;
  mln_runtime_event_offline_region_tile_count_limit
    offline_region_tile_count_limit;
  mln_runtime_event_camera_transition_finished camera_transition_finished;
  mln_runtime_event_command_finished command_finished;
} mln_runtime_event_payload;

/**
 * One drained runtime event.
 *
 * Events have a fixed stride and hold no pointers, so a host can copy a whole
 * batch with one memory copy.
 *
 * Step through an array of these by
 * mln_runtime_event_batch_view.event_size rather than by the size of this
 * struct: a later version may add a member to
 * mln_runtime_event_payload and widen the stride. Every field below, payload
 * included, keeps its offset across versions.
 */
typedef struct mln_runtime_event {
  /** One of mln_runtime_event_type. */
  uint32_t type;
  /** One of mln_runtime_event_source_type. */
  uint32_t source_type;
  /**
   * Source handle selected by source_type: an mln_runtime, mln_map, or
   * mln_projection. Every handle type is uint64_t, so this needs no cast.
   *
   * The value names one object for the life of the process, so a host may
   * compare it against a handle it holds even after that handle is released.
   */
  uint64_t source;
  /**
   * Secondary event detail whose meaning type selects. Depending on type it
   * carries an mln_camera_change_mode, an mln_status, a MapLibre Native error
   * ordinal, or 0. See mln_runtime_event_type for the per-type meaning.
   */
  int32_t code;
  /** One of mln_runtime_event_payload_type. */
  uint32_t payload_type;
  /**
   * Byte offset of this event's message inside
   * mln_runtime_event_batch_view.messages. Zero when message_size is 0.
   */
  uint32_t message_offset;
  /**
   * Number of message bytes, excluding the trailing null terminator that
   * follows them in the arena. Zero when this event carries no message.
   */
  uint32_t message_size;
  /** Typed payload selected by payload_type. */
  mln_runtime_event_payload payload;
} mln_runtime_event;

/**
 * A borrowed view of one owned runtime-event batch.
 *
 * Step through events by event_size. The event and message pointers remain
 * valid until the event-batch handle is released.
 */
typedef struct mln_runtime_event_batch_view {
  uint32_t size;
  /**
   * Stride of one event in bytes, at least sizeof(mln_runtime_event) in the
   * header a caller compiled against. Index events with this value.
   */
  uint32_t event_size;
  /** Borrowed array of event_count events in queue order. */
  const mln_runtime_event* events;
  /** Number of events in events. */
  size_t event_count;
  /**
   * Borrowed message arena holding every event's message bytes, each followed
   * by a null terminator. Null when messages_size is 0.
   */
  const char* messages;
  /** Number of bytes in messages, including every terminator. */
  size_t messages_size;
  /** Events still queued after this batch was drained. */
  size_t remaining_count;
} mln_runtime_event_batch_view;

typedef struct mln_resource_transform_response {
  uint32_t size;
  /** Replacement URL. Null or empty keeps the original URL. Copied on return.
   */
  const char* url;
  /** C API-managed callback context. Callback implementations leave unchanged.
   */
  void* context;
} mln_resource_transform_response;

/**
 * Copies a replacement URL into C API-managed storage for the current callback.
 *
 * Use this helper inside mln_resource_transform_callback implementations to set
 * out_response->url from temporary host-language storage. The copied URL stays
 * valid until the current resource transform invocation finishes. Empty input
 * clears the replacement URL.
 *
 * Returns MLN_STATUS_INVALID_ARGUMENT when response is null, response->size is
 * too small, url is null with a non-zero size, or url contains embedded NUL.
 * Returns MLN_STATUS_INVALID_STATE when called outside a resource transform
 * callback.
 */
MLN_API mln_status mln_resource_transform_response_set_url(
  mln_resource_transform_response* response, const char* url, size_t url_size
) MLN_NOEXCEPT;

/**
 * Rewrites a network resource URL.
 *
 * This callback can only replace the request URL. It cannot mutate headers,
 * bodies, cache policy, or convert a request into an error.
 *
 * Callback invocations follow these rules:
 *
 * - MapLibre may invoke the callback on a runtime, worker, or network thread.
 * - The callback must be thread-safe, return quickly, and must not call C API
 *   functions other than mln_resource_transform_response_set_url().
 * - url and out_response are borrowed for the callback duration.
 * - Use mln_resource_transform_response_set_url() to set a replacement URL from
 *   temporary host-language storage. The helper copies the URL into C
 *   API-managed storage for the current transform invocation.
 * - A non-OK return status is treated as no rewrite and does not fail the
 *   resource request.
 * - The callback and user_data stay valid through the command-finished event
 *   for the command that replaces or clears them. A committed event guarantees
 *   that every invocation using the previous registration has returned.
 */
typedef mln_status (*mln_resource_transform_callback)(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
);

typedef struct mln_resource_transform {
  uint32_t size;
  mln_resource_transform_callback callback;
  void* user_data;
} mln_resource_transform;

typedef struct mln_http_header_transform_response {
  uint32_t size;
  /** C API-managed callback context. Callback implementations leave unchanged.
   */
  void* context;
} mln_http_header_transform_response;

/**
 * Sets one outgoing HTTP request header for the current transform invocation.
 *
 * The function copies name and value before returning. A later call using the
 * same case-insensitive name replaces the earlier value.
 *
 * Returns MLN_STATUS_INVALID_ARGUMENT when response is null, response->size is
 * too small, a pointer is null with a non-zero size, name is not a valid HTTP
 * field name, value is not valid UTF-8, value contains a disallowed control
 * byte, or name identifies a header managed by MapLibre or the platform
 * transport. A diagnostic for a rejected header names the header but never
 * includes its value.
 * Returns MLN_STATUS_INVALID_STATE when called outside an active HTTP header
 * transform callback.
 * Returns MLN_STATUS_NATIVE_ERROR when native allocation fails.
 */
MLN_API mln_status mln_http_header_transform_response_set(
  mln_http_header_transform_response* response, const char* name,
  size_t name_size, const char* value, size_t value_size
) MLN_NOEXCEPT;

/**
 * Adds end-to-end headers to one outgoing HTTP request attempt.
 *
 * MapLibre invokes the callback synchronously on a worker or network thread
 * after resource URL transformation and immediately before the platform HTTP
 * transport starts the attempt. kind is one mln_resource_kind value and url is
 * the transformed URL that the transport will request.
 *
 * The callback and user_data must be thread-safe. They stay valid through the
 * command-finished event for the command that replaces or clears them. A
 * committed event guarantees that every invocation using the previous
 * registration has returned. url and out_response are borrowed for the
 * callback duration. Callback implementations call only
 * mln_http_header_transform_response_set() and return promptly. A non-OK result
 * discards every header collected during the invocation and lets the request
 * proceed unchanged.
 */
typedef mln_status (*mln_http_header_transform_callback)(
  void* user_data, uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
);

typedef struct mln_http_header_transform {
  uint32_t size;
  mln_http_header_transform_callback callback;
  void* user_data;
} mln_http_header_transform;

typedef struct mln_resource_request {
  uint32_t size;
  /**
   * URL entering the network layer, before tile server normalization.
   *
   * It preserves configured URI scheme aliases such as maplibre: and custom
   * schemes, and it is the logical, cache-facing identity of the request.
   * Tile coordinates, glyph ranges, and sprite suffixes are already
   * substituted.
   */
  const char* requested_url;
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
  const char* resolved_url;
  uint32_t kind;
  uint32_t loading_method;
  uint32_t priority;
  uint32_t usage;
  uint32_t storage_policy;
  bool has_range;
  uint64_t range_start;
  uint64_t range_end;
  bool has_prior_modified;
  int64_t prior_modified_unix_ms;
  bool has_prior_expires;
  int64_t prior_expires_unix_ms;
  const char* prior_etag;
  const uint8_t* prior_data;
  size_t prior_data_size;
} mln_resource_request;

typedef struct mln_resource_response {
  uint32_t size;
  uint32_t status;
  uint32_t error_reason;
  /** Response bytes. May be null only when byte_count is 0. */
  const uint8_t* bytes;
  size_t byte_count;
  const char* error_message;
  bool must_revalidate;
  bool has_modified;
  int64_t modified_unix_ms;
  bool has_expires;
  int64_t expires_unix_ms;
  const char* etag;
  bool has_retry_after;
  int64_t retry_after_unix_ms;
} mln_resource_response;

/**
 * Intercepts a network resource request.
 *
 * The callback runs synchronously on the MapLibre thread that reaches the C API
 * network file source.
 *
 * Request handling follows these rules:
 *
 * - request and its pointed-to fields are borrowed for the callback duration.
 * - MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH lets native OnlineFileSource
 *   handle the request.
 * - After returning PASS_THROUGH, the provider must not retain, complete, or
 *   release the handle.
 * - MLN_RESOURCE_PROVIDER_DECISION_HANDLE lets the provider complete the
 *   request through the handle inline or later.
 * - A callback that returns HANDLE may release the handle during the callback.
 *   The C API defers that release until the callback returns.
 * - Unknown decision values produce a provider error response. The C API
 *   releases the provided handle and does not pass the request through.
 * - The C API copies completion data, and mln_resource_request_complete() may
 *   be called from any thread.
 * - Providers must release handled request handles after they no longer need to
 *   complete or observe cancellation.
 * - The callback must be thread-safe, return quickly, and must not call map or
 *   runtime C API functions.
 * - The callback may call resource request handle functions for the provided
 *   handle.
 * - The callback and user_data stay valid through the command-finished event
 *   for the command that replaces or clears them. A committed event guarantees
 *   that every invocation using the previous registration has returned.
 */
typedef uint32_t (*mln_resource_provider_callback)(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
);

typedef struct mln_resource_provider {
  uint32_t size;
  mln_resource_provider_callback callback;
  void* user_data;
} mln_resource_provider;

/**
 * Returns runtime options initialized for this C API version.
 */
MLN_API mln_runtime_options mln_runtime_options_default(void) MLN_NOEXCEPT;

/**
 * Starts runtime creation on a new core-owned worker.
 *
 * The function copies the options before it returns. The returned operation
 * uses options->notification_source and may be inspected from any thread.
 * Take the runtime handle with mln_runtime_create_take_result() after the
 * operation completes successfully.
 */
MLN_API mln_status mln_runtime_create_start(
  const mln_runtime_options* options, mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Takes the runtime handle from a completed runtime-creation operation.
 *
 * The output must point to the null handle. A successful take transfers the
 * runtime handle exactly once.
 */
MLN_API mln_status mln_runtime_create_take_result(
  mln_operation operation, mln_runtime* out_runtime
) MLN_NOEXCEPT;

/**
 * Registers or replaces a runtime-scoped network resource provider.
 *
 * It is invoked for requests that reach the C API network file source. Built-in
 * non-network schemes such as file, asset, mbtiles, and pmtiles are handled by
 * native MainResourceLoader before this extension point.
 *
 * The function copies the provider shape and accepts the change from any
 * thread. Execution is ordered with every other command for this runtime.
 * out_command_id must point to zero and receives the accepted command's ID.
 *
 * The callback and user_data remain borrowed. They must stay valid until the
 * command-finished event for this command. A committed event means that no
 * in-flight request can invoke the previous provider. Requests that the
 * previous provider already handled retain their request handles.
 *
 * Native OnlineFileSource claims every remaining scheme. A URL with an
 * unrecognized scheme, such as jar:file:, reaches this callback and completes
 * as an HTTP error when the provider passes it through.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, provider is
 *   null, provider->size is too small, callback is null, or out_command_id does
 *   not point to zero.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_runtime_set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider,
  uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Clears the runtime-scoped network resource provider.
 *
 * The function accepts the change from any thread and orders it with every
 * other command for this runtime. out_command_id must point to zero. The
 * callback and user_data remain valid until this command's command-finished
 * event. A committed event means that no in-flight request can invoke the
 * previous provider and later requests use MapLibre's online file source.
 * Requests that the previous provider already handled retain their handles.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_command_id does not point to zero.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_runtime_clear_resource_provider(
  mln_runtime runtime, uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Completes a C API resource provider request.
 *
 * This function may be called inline from the provider callback or later from
 * any thread. The C API copies all response bytes and strings before returning.
 *
 * Completion is one-shot. A second completion, completion after cancellation,
 * or completion with null arguments returns a non-OK status and does not invoke
 * MapLibre's resource callback. Malformed response contents are converted to
 * provider error responses and still consume the completion.
 *
 * Returns:
 * - MLN_STATUS_OK when the response was accepted for asynchronous delivery.
 * - MLN_STATUS_INVALID_ARGUMENT when handle or response is null.
 * - MLN_STATUS_INVALID_STATE when the request was cancelled, already completed,
 *   or can no longer accept a response.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_resource_request_complete(
  mln_resource_request_handle handle, const mln_resource_response* response
) MLN_NOEXCEPT;

/**
 * Reports whether MapLibre has cancelled a C API resource provider request.
 *
 * This function may be called from any thread while the provider still owns the
 * handle. A cancelled request no longer wants a response; later completion is
 * ignored with MLN_STATUS_INVALID_STATE.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when handle or out_cancelled is null.
 */
MLN_API mln_status mln_resource_request_cancelled(
  mln_resource_request_handle handle, bool* out_cancelled
) MLN_NOEXCEPT;

/**
 * Releases the provider's reference to a resource request handle.
 *
 * Release the handle exactly once after completing the request or deciding not
 * to complete it. A provider callback that returns
 * MLN_RESOURCE_PROVIDER_DECISION_HANDLE may release the handle inline. Passing
 * MLN_HANDLE_NULL is a no-op, as is passing a handle this call already
 * released. A released handle reports MLN_STATUS_INVALID_ARGUMENT from every
 * other request entry point, including from a copy another thread holds.
 */
MLN_API void mln_resource_request_release(
  mln_resource_request_handle handle
) MLN_NOEXCEPT;

/**
 * Blocks until a resource request is completed or released.
 *
 * Hosts that hand a request to another execution context use this to drain
 * outstanding requests during teardown. Call it from a context that is not
 * responsible for retiring the request.
 *
 * Returns:
 * - MLN_STATUS_OK once the request is retired, including when it already was.
 * - MLN_STATUS_INVALID_ARGUMENT when handle is MLN_HANDLE_NULL.
 */
MLN_API mln_status mln_resource_request_wait_until_retired(
  mln_resource_request_handle handle
) MLN_NOEXCEPT;

/**
 * Registers or updates a runtime-scoped URL transform for network resources.
 *
 * It is forwarded to MapLibre's OnlineFileSource, so it applies wherever native
 * OnlineFileSource applies transforms, including nested PMTiles network range
 * requests. It does not apply to file, asset, database, MBTiles, or registered
 * C API provider responses intercepted before OnlineFileSource.
 *
 * The function copies the transform shape and accepts the change from any
 * thread. Execution is ordered with every other command for this runtime.
 * out_command_id must point to zero and receives the accepted command's ID.
 * The callback and user_data remain borrowed until this command's
 * command-finished event. A committed event means that no in-flight request can
 * invoke the previous transform.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, transform is
 *   null, transform->size is too small, callback is null, or out_command_id
 *   does not point to zero.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_runtime_set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform,
  uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Clears the runtime-scoped URL transform for network resources.
 *
 * The function accepts the change from any thread and orders it with every
 * other command for this runtime. out_command_id must point to zero. The
 * callback and user_data remain valid until this command's command-finished
 * event. A committed event means that network resource URLs pass through
 * unchanged and no in-flight request can invoke the previous transform.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_command_id does not point to zero.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_runtime_clear_resource_transform(
  mln_runtime runtime, uint64_t* out_command_id
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
 * out_command_id must point to zero and receives the accepted command's ID.
 * The callback and user_data remain borrowed until this command's
 * command-finished event. A committed event means that no in-flight request can
 * invoke the previous transform.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, transform is
 *   null, transform->size is too small, callback is null, or out_command_id
 *   does not point to zero.
 * - MLN_STATUS_UNSUPPORTED on OpenHarmony and in the browser, whose HTTP
 *   clients cannot prevent transformed headers from following a cross-origin
 *   redirect. A resource provider serves those requests instead.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_runtime_set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform,
  uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Clears the runtime-scoped outgoing HTTP header transform.
 *
 * The function accepts the change from any thread and orders it with every
 * other command for this runtime. out_command_id must point to zero. The
 * callback and user_data remain valid until this command's command-finished
 * event. A committed event means that no in-flight request can invoke the
 * previous transform.
 *
 * Returns:
 * - MLN_STATUS_OK when the command is accepted.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_command_id does not point to zero.
 * - MLN_STATUS_INVALID_STATE when the runtime is closing.
 * - MLN_STATUS_NATIVE_ERROR when command acceptance fails.
 */
MLN_API mln_status mln_runtime_clear_http_header_transform(
  mln_runtime runtime, uint64_t* out_command_id
) MLN_NOEXCEPT;

/**
 * Starts a MapLibre ambient cache maintenance operation for this runtime.
 *
 * When runtime options omit cache_path, this operates on MapLibre's default
 * in-memory database and its effects are not durable beyond the native database
 * lifetime. The returned operation uses the runtime's notification source.
 * out_operation must point to the null handle.
 */
MLN_API mln_status mln_runtime_run_ambient_cache_operation_start(
  mln_runtime runtime, uint32_t operation, mln_operation* out_operation
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
 * lifetime. The returned operation uses the runtime's notification source.
 * out_operation must point to the null handle.
 */
MLN_API mln_status mln_runtime_set_maximum_ambient_cache_size_start(
  mln_runtime runtime, uint64_t size, mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Starts an ordered runtime barrier.
 *
 * The operation completes after every command and operation that committed
 * before it has reached a terminal disposition.
 */
MLN_API mln_status mln_runtime_barrier_start(
  mln_runtime runtime, mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Starts committed runtime close.
 *
 * The call synchronously rejects a runtime that has live or pending children
 * and leaves it open. Once accepted, the runtime rejects new work, retires its
 * handle, stops and joins its worker, then completes the operation. Callback
 * registrations remain valid until operation completion. This function may be
 * called from any thread.
 */
MLN_API mln_status mln_runtime_close_start(
  mln_runtime runtime, mln_operation* out_operation
) MLN_NOEXCEPT;

/**
 * Drains this runtime's queued events into a new owned batch.
 *
 * Events arrive in queue order. max_events bounds the drain; zero drains every
 * queued event. A positive value drains at most that many events. The batch
 * view reports how many events remained queued.
 *
 * The returned handle owns copied event records and their message arena. Later
 * drains and runtime destruction leave the batch readable. Release each batch
 * with mln_event_batch_release().
 *
 * This queue operation may be called from any thread. One drain lease may be
 * active for a runtime event queue. A concurrent second drain returns
 * MLN_STATUS_INVALID_STATE.
 *
 * The map and runtime subscription masks suppress unselected events before
 * their payloads, messages, queue records, and notification are produced.
 *
 * Returns:
 * - MLN_STATUS_OK when out_batch receives an owned batch, including an empty
 *   batch.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or out_batch
 *   is null or does not point to the null handle.
 * - MLN_STATUS_INVALID_STATE when another drain is active.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_drain_events(
  mln_runtime runtime, size_t max_events, mln_event_batch* out_batch
) MLN_NOEXCEPT;

/** Borrows the event and message view stored by an owned event batch. */
MLN_API mln_status mln_event_batch_get(
  mln_event_batch batch, mln_runtime_event_batch_view* out_view
) MLN_NOEXCEPT;

/** Releases an owned event batch. A null handle is a no-op. */
MLN_API void mln_event_batch_release(mln_event_batch batch) MLN_NOEXCEPT;

/**
 * Selects which runtime-scoped event types this runtime queues.
 *
 * A runtime queues an offline event when this mask selects its type. Region
 * status, response error, and tile count limit events also require the region
 * to be observed with mln_runtime_offline_region_set_observed_start(), so this
 * mask narrows that subscription rather than replacing it.
 *
 * A runtime that has not been narrowed selects every runtime-scoped event type
 * this library reports, which covers types a caller's header may not
 * declare. A new mask applies to later events and keeps the events already
 * queued.
 *
 * This call reads the bits in MLN_RUNTIME_EVENT_MASK_ALL_RUNTIME_EVENTS and
 * ignores the rest, so MLN_RUNTIME_EVENT_MASK_ALL selects every runtime-scoped
 * type. mln_runtime_get_event_mask() reports the value last
 * set, so a host reads it, changes one bit, and writes it back.
 *
 * Changing this mask does not affect operation completion or result ownership.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not a live runtime
 *   handle, or mask holds a bit outside MLN_RUNTIME_EVENT_MASK_ALL.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_runtime_set_event_mask(mln_runtime runtime, uint64_t mask) MLN_NOEXCEPT;

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
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not a live runtime
 *   handle, or out_mask is null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_get_event_mask(
  mln_runtime runtime, uint64_t* out_mask
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RUNTIME_H
