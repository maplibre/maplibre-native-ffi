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

/** Offline database operation token. Zero is never a valid operation ID. */
typedef uint64_t mln_offline_operation_id;

/** Offline database operation kinds reported by completion events. */
typedef enum mln_offline_operation_kind : uint32_t {
  MLN_OFFLINE_OPERATION_AMBIENT_CACHE = 1,
  MLN_OFFLINE_OPERATION_REGION_CREATE = 2,
  MLN_OFFLINE_OPERATION_REGION_GET = 3,
  MLN_OFFLINE_OPERATION_REGIONS_LIST = 4,
  MLN_OFFLINE_OPERATION_REGIONS_MERGE_DATABASE = 5,
  MLN_OFFLINE_OPERATION_REGION_UPDATE_METADATA = 6,
  MLN_OFFLINE_OPERATION_REGION_GET_STATUS = 7,
  MLN_OFFLINE_OPERATION_REGION_SET_OBSERVED = 8,
  MLN_OFFLINE_OPERATION_REGION_SET_DOWNLOAD_STATE = 9,
  MLN_OFFLINE_OPERATION_REGION_INVALIDATE = 10,
  MLN_OFFLINE_OPERATION_REGION_DELETE = 11,
  MLN_OFFLINE_OPERATION_SET_MAXIMUM_AMBIENT_CACHE_SIZE = 12,
} mln_offline_operation_kind;

/** Offline database operation result kinds reported by completion events. */
typedef enum mln_offline_operation_result_kind : uint32_t {
  MLN_OFFLINE_OPERATION_RESULT_NONE = 0,
  MLN_OFFLINE_OPERATION_RESULT_REGION = 1,
  MLN_OFFLINE_OPERATION_RESULT_OPTIONAL_REGION = 2,
  MLN_OFFLINE_OPERATION_RESULT_REGION_LIST = 3,
  MLN_OFFLINE_OPERATION_RESULT_REGION_STATUS = 4,
} mln_offline_operation_result_kind;

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
 * Runtime event types returned by mln_runtime_poll_event().
 *
 * The event type selects the meaning of mln_runtime_event.code and the struct
 * behind mln_runtime_event.payload. Event type names below omit their
 * MLN_RUNTIME_EVENT_ prefix and payload type names omit their
 * MLN_RUNTIME_EVENT_PAYLOAD_ prefix.
 *
 * - MAP_CAMERA_WILL_CHANGE: code is an mln_camera_change_mode; payload NONE.
 * - MAP_CAMERA_IS_CHANGING: code is 0; payload NONE.
 * - MAP_CAMERA_DID_CHANGE: code is an mln_camera_change_mode; payload NONE.
 * - MAP_CAMERA_TRANSITION_FINISHED: code is 0; payload
 *   CAMERA_TRANSITION_FINISHED.
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
 * - MAP_STYLE_IMAGE_MISSING: code is 0; payload STYLE_IMAGE_MISSING.
 * - MAP_TILE_ACTION: code is 0; payload TILE_ACTION.
 * - OFFLINE_REGION_STATUS_CHANGED: code is 0; payload OFFLINE_REGION_STATUS.
 * - OFFLINE_REGION_RESPONSE_ERROR: code is 0; payload
 *   OFFLINE_REGION_RESPONSE_ERROR.
 * - OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED: code is 0; payload
 *   OFFLINE_REGION_TILE_COUNT_LIMIT.
 * - OFFLINE_OPERATION_COMPLETED: code is the operation result as an mln_status
 *   value, the same value the payload reports in result_status; payload
 *   OFFLINE_OPERATION_COMPLETED.
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
  MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED = 22,
  MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED = 23,
} mln_runtime_event_type;

/** Source kinds used by mln_runtime_event.source_type. */
typedef enum mln_runtime_event_source_type : uint32_t {
  MLN_RUNTIME_EVENT_SOURCE_RUNTIME = 0,
  MLN_RUNTIME_EVENT_SOURCE_MAP = 1,
} mln_runtime_event_source_type;

/** Payload kinds used by mln_runtime_event.payload_type. */
typedef enum mln_runtime_event_payload_type : uint32_t {
  MLN_RUNTIME_EVENT_PAYLOAD_NONE = 0,
  MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME = 1,
  MLN_RUNTIME_EVENT_PAYLOAD_RENDER_MAP = 2,
  MLN_RUNTIME_EVENT_PAYLOAD_STYLE_IMAGE_MISSING = 3,
  MLN_RUNTIME_EVENT_PAYLOAD_TILE_ACTION = 4,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_STATUS = 5,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_RESPONSE_ERROR = 6,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_REGION_TILE_COUNT_LIMIT = 7,
  MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED = 8,
  MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED = 9,
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

typedef struct mln_runtime_options {
  uint32_t size;
  /** No flags are currently defined. Must be zero. */
  uint32_t flags;
  /** Filesystem root for asset:// URLs. Copied during runtime creation. */
  const char* asset_path;
  /** Cache database path. Copied during runtime creation. */
  const char* cache_path;
} mln_runtime_options;

/** Rendering statistics reported in MLN_RUNTIME_EVENT_PAYLOAD_RENDER_FRAME. */
typedef struct mln_rendering_stats {
  uint32_t size;
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
  uint32_t size;
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
  uint32_t size;
  /** One of mln_render_mode. */
  uint32_t mode;
} mln_runtime_event_render_map;

/** Payload for MLN_RUNTIME_EVENT_MAP_STYLE_IMAGE_MISSING. */
typedef struct mln_runtime_event_style_image_missing {
  uint32_t size;
  /**
   * Borrowed image ID bytes. Valid until the next poll for this runtime or
   * until the runtime is destroyed.
   */
  const char* image_id;
  /** Number of bytes in image_id, excluding the trailing null terminator. */
  size_t image_id_size;
} mln_runtime_event_style_image_missing;

/** Overscaled tile identity reported in tile observer events. */
typedef struct mln_tile_id {
  uint32_t overscaled_z;
  int32_t wrap;
  uint32_t canonical_z;
  uint32_t canonical_x;
  uint32_t canonical_y;
} mln_tile_id;

/** Payload for MLN_RUNTIME_EVENT_MAP_TILE_ACTION. */
typedef struct mln_runtime_event_tile_action {
  uint32_t size;
  /** One of mln_tile_operation. */
  uint32_t operation;
  mln_tile_id tile_id;
  /**
   * Borrowed source ID bytes. Valid until the next poll for this runtime or
   * until the runtime is destroyed.
   */
  const char* source_id;
  /** Number of bytes in source_id, excluding the trailing null terminator. */
  size_t source_id_size;
} mln_runtime_event_tile_action;

/**
 * Payload for MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED.
 *
 * See mln_animation_options.transition_id for how a caller stamps an identity
 * onto a camera transition and what terminal outcomes this event covers.
 */
typedef struct mln_runtime_event_camera_transition_finished {
  uint32_t size;
  /**
   * The transition_id the caller set on the mln_animation_options that started
   * this transition.
   */
  uint64_t transition_id;
} mln_runtime_event_camera_transition_finished;

/** Payload for MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED. */
typedef struct mln_runtime_event_offline_region_status {
  uint32_t size;
  mln_offline_region_id region_id;
  mln_offline_region_status status;
} mln_runtime_event_offline_region_status;

/** Payload for MLN_RUNTIME_EVENT_OFFLINE_REGION_RESPONSE_ERROR. */
typedef struct mln_runtime_event_offline_region_response_error {
  uint32_t size;
  mln_offline_region_id region_id;
  /** One of mln_resource_error_reason. */
  uint32_t reason;
} mln_runtime_event_offline_region_response_error;

/** Payload for MLN_RUNTIME_EVENT_OFFLINE_REGION_TILE_COUNT_LIMIT_EXCEEDED. */
typedef struct mln_runtime_event_offline_region_tile_count_limit {
  uint32_t size;
  mln_offline_region_id region_id;
  uint64_t limit;
} mln_runtime_event_offline_region_tile_count_limit;

/** Payload for MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED. */
typedef struct mln_runtime_event_offline_operation_completed {
  uint32_t size;
  mln_offline_operation_id operation_id;
  /** One of mln_offline_operation_kind. */
  uint32_t operation_kind;
  /** One of mln_offline_operation_result_kind. */
  uint32_t result_kind;
  /** Async result status as a mln_status value. */
  int32_t result_status;
  /** Meaningful for MLN_OFFLINE_OPERATION_REGION_GET. */
  bool found;
} mln_runtime_event_offline_operation_completed;

/** Event payload returned by mln_runtime_poll_event(). */
typedef struct mln_runtime_event {
  uint32_t size;
  uint32_t type;
  /** One of mln_runtime_event_source_type. */
  uint32_t source_type;
  /**
   * Source handle for this event: the mln_map for map-originated events, the
   * mln_runtime for runtime-originated events, selected by source_type. Every
   * handle type is uint64_t, so this needs no cast.
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
  /** Borrowed payload selected by payload_type. Null when payload_size is 0. */
  const void* payload;
  /** Number of bytes in payload. */
  size_t payload_size;
  /** Borrowed event message bytes. Null when message_size is 0. */
  const char* message;
  /** Number of bytes in message, excluding the trailing null terminator. */
  size_t message_size;
} mln_runtime_event;

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
 * - MapLibre may invoke the callback on a worker or network thread instead of
 *   the runtime owner thread.
 * - The callback must be thread-safe, return quickly, and must not call C API
 *   functions other than mln_resource_transform_response_set_url().
 * - url and out_response are borrowed for the callback duration.
 * - Use mln_resource_transform_response_set_url() to set a replacement URL from
 *   temporary host-language storage. The helper copies the URL into C
 *   API-managed storage for the current transform invocation.
 * - A non-OK return status is treated as no rewrite and does not fail the
 *   resource request.
 * - The callback and user_data must remain valid until the transform is
 *   replaced, cleared, or the runtime is destroyed. Those calls wait for
 *   in-flight transform callbacks before returning.
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
 * The callback and user_data must be thread-safe and remain valid until the
 * transform is replaced, cleared, or the runtime is destroyed. Those calls
 * wait for in-flight callbacks before returning. url and out_response are
 * borrowed for the callback duration. Callback implementations call only
 * mln_http_header_transform_response_set() and return promptly. A non-OK
 * result discards every header collected during the invocation and lets the
 * request proceed unchanged.
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
   * It also matches requested_url when normalization rejects the URL, which a
   * tile server that requires an API key does for a canonical URL when no key
   * is configured. Native loading fails such a request outright, so keeping the
   * request reachable lets a provider serve it instead. A provider that only
   * fetches over HTTP checks the scheme before it does.
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
 * The callback runs synchronously on the thread that reaches the C API network
 * file source. That thread may be a MapLibre worker or network thread instead
 * of the runtime owner thread.
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
 * Creates a runtime handle.
 *
 * The creating thread becomes the runtime owner thread. Each owner thread may
 * hold one live runtime.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when out_runtime is null, *out_runtime is not
 *   null, or options has an unsupported size or flags.
 * - MLN_STATUS_INVALID_STATE when the current thread already owns a live
 *   runtime.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_create(
  const mln_runtime_options* options, mln_runtime* out_runtime
) MLN_NOEXCEPT;

/**
 * Registers or replaces a runtime-scoped network resource provider.
 *
 * It is invoked for requests that reach the C API network file source. Built-in
 * non-network schemes such as file, asset, mbtiles, and pmtiles are handled by
 * native MainResourceLoader before this extension point.
 *
 * This call may replace an existing provider while maps exist. The callback and
 * user_data are stored by reference and must remain valid until this call
 * returns having replaced them, mln_runtime_clear_resource_provider() returns,
 * or the runtime is destroyed. When this call returns, no in-flight request can
 * still invoke the previous provider. Requests the previous provider already
 * took a handle for keep that handle: complete and release each one as usual.
 * Native OnlineFileSource claims every remaining scheme, so a URL with a
 * scheme MapLibre does not recognize, such as jar:file:, is treated as a
 * network request, reaches this callback, and completes as an HTTP error when
 * the provider passes it through. Hosts use this extension point to serve
 * those schemes from host storage.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, provider is
 *   null, provider->size is too small, or callback is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider
) MLN_NOEXCEPT;

/**
 * Clears the runtime-scoped network resource provider.
 *
 * After this call succeeds, requests that reach the C API network file source
 * go to MapLibre's online file source. When it returns, no in-flight request
 * can still invoke the previous provider, and the C API holds no further
 * reference to its callback or user_data. Requests the previous provider
 * already took a handle for keep that handle: complete and release each one as
 * usual.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_runtime_clear_resource_provider(mln_runtime runtime) MLN_NOEXCEPT;

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
 * outstanding requests during teardown, because a handled request keeps host
 * state reachable until it is retired. Call it from a context that is not
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
 * This call may replace an existing transform while maps exist. When it
 * returns, no in-flight request can still invoke the previous transform.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, transform is
 *   null, transform->size is too small, or callback is null.
 * - MLN_STATUS_UNSUPPORTED on OpenHarmony, whose platform HTTP client cannot
 *   prevent transformed headers from following a cross-origin redirect.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform
) MLN_NOEXCEPT;

/**
 * Clears the runtime-scoped URL transform for network resources.
 *
 * After this call succeeds, network resource URLs pass through unchanged. When
 * it returns, no in-flight request can still invoke the previous transform.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_runtime_clear_resource_transform(mln_runtime runtime) MLN_NOEXCEPT;

/**
 * Registers or replaces the runtime-scoped outgoing HTTP header transform.
 *
 * The transform applies only to requests that reach the built-in HTTP client,
 * including online and offline requests and nested network-backed PMTiles
 * range requests. Cache hits, non-HTTP schemes, and provider-handled requests
 * do not invoke it. Replacement waits for in-flight callbacks before the old
 * callback and user_data become unreferenced.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, transform is
 *   null, transform->size is too small, or callback is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform
) MLN_NOEXCEPT;

/**
 * Clears the runtime-scoped outgoing HTTP header transform.
 *
 * When this call returns, no in-flight request can still invoke the previous
 * callback and the C API holds no reference to its user_data.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_runtime_clear_http_header_transform(mln_runtime runtime) MLN_NOEXCEPT;

/**
 * Starts a MapLibre ambient cache maintenance operation for this runtime.
 *
 * When runtime options omit cache_path, this operates on MapLibre's default
 * in-memory database and its effects are not durable beyond the native database
 * lifetime. Completion is reported through
 * MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or operation
 *   is not a mln_ambient_cache_operation value, or out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_run_ambient_cache_operation_start(
  mln_runtime runtime, uint32_t operation,
  mln_offline_operation_id* out_operation_id
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
 * lifetime. Completion is reported through
 * MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED.
 *
 * Returns:
 * - MLN_STATUS_OK when the operation was accepted and out_operation_id was set.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   out_operation_id is null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_set_maximum_ambient_cache_size_start(
  mln_runtime runtime, uint64_t size, mln_offline_operation_id* out_operation_id
) MLN_NOEXCEPT;

/**
 * Discards runtime-owned state for an offline database operation.
 *
 * Discarding does not cancel native database work. It drops stored results,
 * removes queued completion events for the operation, and suppresses later
 * completion delivery when the native operation is still pending.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, or
 *   operation_id is zero or unknown.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_offline_operation_discard(
  mln_runtime runtime, mln_offline_operation_id operation_id
) MLN_NOEXCEPT;

/**
 * Destroys a runtime handle.
 *
 * The runtime must no longer own live maps.
 *
 * When a resource transform is registered, this call waits for in-flight
 * transform callbacks before returning, the same way
 * mln_runtime_set_resource_transform() and
 * mln_runtime_clear_resource_transform() do. The callback contract documented
 * on mln_resource_transform_callback keeps that wait short: the callback
 * returns quickly and calls no C API function other than
 * mln_resource_transform_response_set_url().
 * Registered HTTP header transforms have the same retirement guarantee and
 * remain valid until every in-flight mln_http_header_transform_callback
 * returns.
 *
 * A registered resource provider is waited on the same way: this call blocks
 * until every in-flight mln_resource_provider_callback invocation returns,
 * matching mln_runtime_set_resource_provider() and
 * mln_runtime_clear_resource_provider(). The callback and its user_data stay
 * valid until that point and are unreferenced once this call returns, so a
 * host frees provider-owned state only after it does.
 *
 * Both waits run with no runtime-internal lock that other runtimes need, so a
 * slow callback delays only this runtime. Do not call this while holding a
 * host lock that a provider, URL transform, or HTTP header transform callback
 * also acquires; the callback
 * cannot finish, and this call cannot return.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not a live runtime
 *   handle.
 * - MLN_STATUS_INVALID_STATE when runtime still owns live maps.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the creating
 *   thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_destroy(mln_runtime runtime) MLN_NOEXCEPT;

/**
 * Advances this runtime.
 *
 * The call parks the owner thread when timeout_ms allows it, then drains the
 * owner-thread task queues. Drain the queued runtime events with
 * mln_runtime_poll_event() afterwards.
 *
 * timeout_ms sets the park bound:
 *
 * - Zero drains and returns. Hosts pumping from a frame callback pass zero and
 *   take their cadence from that callback.
 * - A positive value parks for up to that many milliseconds, then drains. Hosts
 *   that own their pump thread pass a positive value and take their cadence
 *   from the runtime's own work.
 * - A negative value parks until a wake arrives, then drains.
 *
 * The drain runs every task queued when it begins plus every task those tasks
 * enqueue, and services expired timers and ready file descriptors for the
 * runtime's own network and database work. Its duration follows the work it
 * finds and can span a full style parse, so treat it as work that runs to
 * completion rather than as a fixed-cost per-frame slice.
 *
 * The runtime holds a wake flag. These set it:
 *
 * - the owner-thread run loop receiving queued work from any thread, which
 *   covers style, tile, offline database, and resource responses;
 * - the runtime queueing a runtime event;
 * - mln_wake_source_signal() from any thread.
 *
 * A parking call returns as soon as the flag is set, and clears the flag before
 * it returns. Work that arrives during the drain sets the flag again, so the
 * next call returns right away and may find that work already done.
 *
 * A call also returns without parking while unread runtime events are queued.
 *
 * Timers and file descriptors set the flag only when they queue owner-thread
 * work, and the runtime registers none of its own on the owner-thread run loop.
 * Pass a positive timeout_ms so a call returns even when nothing sets the flag.
 *
 * A non-zero timeout_ms makes this a blocking query. Call it outside any host
 * lock that a thread signalling a wake source acquires, and outside C API
 * callbacks. Acquire a wake source with mln_runtime_wake_source_acquire() to
 * release the owner thread for host-driven work such as submitted tasks or
 * shutdown.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not a live runtime
 *   handle.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status
mln_runtime_pump(mln_runtime runtime, int64_t timeout_ms) MLN_NOEXCEPT;

/**
 * Acquires a wake source that releases this runtime's parked owner thread.
 *
 * Each call returns a distinct handle the host destroys with
 * mln_wake_source_destroy(). A wake source holds its own reference to the
 * runtime's wake state, so it stays valid after the runtime is destroyed and
 * hosts tear the two down in either order.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not a live runtime
 *   handle, out_source is null, or *out_source is not null.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_wake_source_acquire(
  mln_runtime runtime, mln_wake_source* out_source
) MLN_NOEXCEPT;

/**
 * Sets the runtime's wake flag and releases the parked owner thread.
 *
 * This function may be called from any thread. It takes one small lock and
 * returns, so a host calls it from its task submission path.
 *
 * A signal raised while the owner thread is running sets the wake flag, so the
 * next mln_runtime_pump() call returns without parking. Signalling a wake
 * source whose runtime is destroyed succeeds and does nothing, so hosts shut
 * the two down in either order.
 *
 * Returns:
 * - MLN_STATUS_OK on success, including after the runtime is destroyed.
 * - MLN_STATUS_INVALID_ARGUMENT when source is null.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_wake_source_signal(mln_wake_source source) MLN_NOEXCEPT;

/**
 * Destroys a wake source.
 *
 * This function may be called from any thread. Null is a no-op. Destroy each
 * handle exactly once, once every thread that signals it has finished.
 */
MLN_API void mln_wake_source_destroy(mln_wake_source source) MLN_NOEXCEPT;

/**
 * Pops the next queued runtime event.
 *
 * On success, *out_event is reset and *out_has_event indicates whether an event
 * was available. When an event is available, *out_event receives it.
 * Map-originated events set out_event->source_type to
 * MLN_RUNTIME_EVENT_SOURCE_MAP and out_event->source to the source map.
 * Runtime-originated events set out_event->source_type to
 * MLN_RUNTIME_EVENT_SOURCE_RUNTIME.
 *
 * When an event is available, out_event->payload points to runtime-owned
 * storage containing a struct selected by out_event->payload_type, or null when
 * the payload type is MLN_RUNTIME_EVENT_PAYLOAD_NONE. String pointers inside
 * typed payloads and out_event->message remain valid until the next
 * mln_runtime_poll_event() call for the same runtime or until the runtime is
 * destroyed. Copy those bytes before then when they must outlive that window.
 * For style-image-missing and tile-action events, out_event->message contains
 * the same ID string exposed by the typed payload.
 *
 * out_event->code carries a secondary detail whose meaning out_event->type
 * selects. mln_runtime_event_type lists the meaning for every event type.
 *
 * Destroying a map discards that map's queued events, so this function returns
 * events only for maps that are still live. Read the state a host mirrors from
 * events before destroying the map that produces them.
 *
 * Returns:
 * - MLN_STATUS_OK when the poll completed; out_has_event indicates whether an
 *   event was written to out_event.
 * - MLN_STATUS_INVALID_ARGUMENT when runtime is null or not live, out_event is
 *   null, out_has_event is null, or out_event->size is too small.
 * - MLN_STATUS_WRONG_THREAD when called from a thread other than the runtime
 *   owner thread.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_API mln_status mln_runtime_poll_event(
  mln_runtime runtime, mln_runtime_event* out_event, bool* out_has_event
) MLN_NOEXCEPT;

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_RUNTIME_H
