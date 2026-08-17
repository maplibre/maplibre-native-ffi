/**
 * @file maplibre_native_c/callback_adapter.h
 * Public C API declarations for adapting native callbacks to host runtimes
 * that cannot run user code on a native callback thread.
 *
 * MapLibre callback contracts are synchronous: logging and resource providers
 * return an immediate decision, and borrowed request payloads expire when the
 * callback returns. This layer answers on behalf of hosts that cannot. It
 * copies borrowed payloads into native-owned records the host releases
 * explicitly, applies native-owned routing rules when a decision is needed
 * immediately, and hands records to the host through void listener functions,
 * so host user code runs on its own execution context rather than on MapLibre
 * worker, network, logging, or render threads.
 *
 * This header is not part of the maplibre_native_c.h umbrella. Include it
 * directly when a binding needs it.
 *
 * This header targets C23.
 */

#ifndef MAPLIBRE_NATIVE_C_CALLBACK_ADAPTER_H
#define MAPLIBRE_NATIVE_C_CALLBACK_ADAPTER_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c/base.h"     // IWYU pragma: export
#include "maplibre_native_c/logging.h"  // IWYU pragma: export
#include "maplibre_native_c/runtime.h"  // IWYU pragma: export
#include "maplibre_native_c/style.h"    // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

/** Rule kind that matches every resource kind. */
#define MLN_ADAPTER_RESOURCE_KIND_ANY UINT32_MAX

// This block uses line comments because its examples contain URL patterns that
// a block comment cannot carry.

/// How a rule compares its url against a request URL.
///
/// With no flags, a rule compares the complete URL byte for byte.
/// MLN_ADAPTER_URL_MATCH_GLOB reads the url as a glob pattern instead:
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
/// Confining `*` to one path segment is what makes a host pattern hold:
/// `https://*.example.com/**` matches every subdomain of example.com and never
/// `https://attacker.example/x.example.com/tile`. Use `**` wherever a pattern
/// spans path segments, as in `https://tiles.example.com/**` for one host.
typedef enum mln_adapter_url_match_flags : uint32_t {
  MLN_ADAPTER_URL_MATCH_FLAGS_NONE = 0U,
  MLN_ADAPTER_URL_MATCH_GLOB = 1U << 0U,
} mln_adapter_url_match_flags;

/**
 * One resource rewrite rule.
 *
 * The kind field matches mln_resource_kind values, or
 * MLN_ADAPTER_RESOURCE_KIND_ANY for every kind. The flags field is a bitwise OR
 * of mln_adapter_url_match_flags values choosing how url compares against the
 * request URL. A null url or an unknown flag bit makes the rule match nothing.
 *
 * A null replacement_url leaves the URL unchanged. Both strings are borrowed
 * and must outlive the rule table.
 */
typedef struct mln_adapter_resource_rewrite_rule {
  uint32_t kind;
  uint32_t flags;
  const char* url;
  const char* replacement_url;
} mln_adapter_resource_rewrite_rule;

/**
 * A borrowed table of rewrite rules.
 *
 * The rules pointer and every rule string stay valid through the terminal event
 * of the command that replaces or clears this transform.
 */
typedef struct mln_adapter_resource_rewrite_rules {
  const mln_adapter_resource_rewrite_rule* rules;
  size_t count;
} mln_adapter_resource_rewrite_rules;

/** One borrowed header supplied by an HTTP header transform rule. */
typedef struct mln_adapter_http_header {
  const char* name;
  const char* value;
} mln_adapter_http_header;

/**
 * One native-owned matching rule for an HTTP header transform.
 *
 * kind is one mln_resource_kind value or MLN_ADAPTER_RESOURCE_KIND_ANY. The
 * flags field is a bitwise OR of mln_adapter_url_match_flags values choosing
 * how url compares against the complete transformed URL. A null url or an
 * unknown flag bit makes the rule match nothing.
 *
 * The first matching rule supplies its complete header list. Every pointer
 * stays valid through the terminal event of the command that replaces or
 * clears this transform.
 */
typedef struct mln_adapter_http_header_transform_rule {
  uint32_t kind;
  uint32_t flags;
  const char* url;
  const mln_adapter_http_header* headers;
  size_t header_count;
} mln_adapter_http_header_transform_rule;

/** A borrowed table of HTTP header transform rules. */
typedef struct mln_adapter_http_header_transform_rules {
  const mln_adapter_http_header_transform_rule* rules;
  size_t count;
} mln_adapter_http_header_transform_rules;

/**
 * One resource provider rule.
 *
 * The kind field matches mln_resource_kind values, or
 * MLN_ADAPTER_RESOURCE_KIND_ANY for every kind. The flags field is a bitwise OR
 * of mln_adapter_url_match_flags values choosing how requested_url compares
 * against mln_resource_request.requested_url. A null requested_url or an
 * unknown flag bit makes the rule match nothing.
 *
 * A matching request is completed with the rule's response without reaching the
 * host. The response and its buffers are borrowed and must outlive the rule
 * table.
 */
typedef struct mln_adapter_resource_provider_rule {
  uint32_t kind;
  uint32_t flags;
  const char* requested_url;
  mln_resource_response response;
} mln_adapter_resource_provider_rule;

/**
 * A borrowed table of provider rules.
 *
 * The rules pointer, response buffers, and rule strings stay valid through the
 * terminal event of the command that replaces or clears this provider.
 */
typedef struct mln_adapter_resource_provider_rules {
  const mln_adapter_resource_provider_rule* rules;
  size_t count;
} mln_adapter_resource_provider_rules;

/**
 * How a queued provider route compares its url against a request.
 *
 * MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB reads the url as a glob pattern, in the
 * language mln_adapter_url_match_flags describes.
 * MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL selects
 * mln_resource_request.requested_url as the compared URL instead of
 * mln_resource_request.resolved_url. Setting both matches a requested-URL glob.
 */
typedef enum mln_adapter_resource_route_flags : uint32_t {
  MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE = 0U,
  MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB = 1U << 0U,
  MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL = 1U << 1U,
} mln_adapter_resource_route_flags;

/**
 * One route a queued provider claims.
 *
 * The kind field matches mln_resource_kind values, or
 * MLN_ADAPTER_RESOURCE_KIND_ANY for every kind. The flags field is a bitwise OR
 * of mln_adapter_resource_route_flags values choosing which URL the route
 * compares and how; with no flags the route matches
 * mln_resource_request.resolved_url exactly.
 *
 * The url field is a comparison value, read literally or as a glob pattern
 * according to flags. A null url or an unknown flag bit makes the route match
 * nothing. The url pointer has the lifetime of its queued provider.
 */
typedef struct mln_adapter_queued_resource_provider_route {
  uint32_t kind;
  uint32_t flags;
  const char* url;
} mln_adapter_queued_resource_provider_route;

/**
 * A provider that copies matching requests into a native queue.
 *
 * The routes pointer and every route URL stay valid through the terminal event
 * of the command that replaces or clears this provider. queue identifies the
 * queue that receives each copied request.
 */
typedef struct mln_adapter_queued_resource_provider {
  const mln_adapter_queued_resource_provider_route* routes;
  size_t route_count;
  mln_adapter_resource_request_queue queue;
} mln_adapter_queued_resource_provider;

/**
 * A native-owned copy of a resource request.
 *
 * Every pointer field is owned by this record and stays valid until
 * mln_adapter_resource_provider_request_destroy(). The handle field carries the
 * request handle the host completes; it is an ordinary handle value the host
 * moves between execution contexts and passes to mln_resource_request_*().
 */
typedef struct mln_adapter_queued_resource_request {
  void* owner;
  mln_resource_request_handle handle;
  /**
   * Copy of mln_resource_request.requested_url, or the empty string when the
   * request carried none. Never null, unlike prior_etag.
   */
  const char* requested_url;
  /**
   * Copy of mln_resource_request.resolved_url, or the empty string when the
   * request carried none. Never null, unlike prior_etag.
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
} mln_adapter_queued_resource_request;

/**
 * A native-owned copy of a log record.
 *
 * The message pointer is owned by this record and stays valid until
 * mln_adapter_log_record_destroy().
 */
typedef struct mln_adapter_log_record {
  void* owner;
  uint32_t severity;
  uint32_t event;
  int64_t code;
  const char* message;
} mln_adapter_log_record;

/**
 * Registration state for an adapted log callback.
 *
 * The callback copies records into queue and reports consume to MapLibre. The
 * address of this struct identifies the registration. When release_user_data is
 * non-null, a successful install transfers responsibility for release_context
 * to the adapter, which releases it after the registration is replaced or
 * cleared. The struct must remain valid until that release callback runs.
 */
typedef struct mln_adapter_log_callback_state {
  mln_adapter_log_queue queue;
  uint32_t consume;
  mln_log_callback_release release_user_data;
  void* release_context;
} mln_adapter_log_callback_state;

/**
 * Creates a token describing a handle the host has not closed yet.
 *
 * The token copies type_name and records handle so the report can name them.
 * Hosts attach the token to a finalizer, which can no longer touch the handle
 * itself.
 *
 * Returns null when the token cannot be allocated.
 */
MLN_API void* mln_adapter_handle_leak_token_create(
  const char* type_name, uint64_t handle
) MLN_NOEXCEPT;

/** Releases a leak token without reporting it. */
MLN_API void mln_adapter_handle_leak_token_destroy(void* token) MLN_NOEXCEPT;

/** Reports a leaked handle on stderr and releases the token. */
MLN_API void mln_adapter_handle_leak_report(void* token) MLN_NOEXCEPT;

/**
 * Creates a resource-request queue associated with one notification source.
 *
 * out_queue must point to the null handle. The association remains immutable
 * until the queue is closed.
 */
MLN_API mln_status mln_adapter_resource_request_queue_create(
  mln_notification_source source, mln_adapter_resource_request_queue* out_queue
) MLN_NOEXCEPT;

/**
 * Acquires the oldest queued request, or null when the queue is empty.
 *
 * out_request must point to null. The caller owns a returned record and
 * releases it with mln_adapter_resource_provider_request_destroy(). The queue
 * remains ready until this drain confirms it is empty.
 */
MLN_API mln_status mln_adapter_resource_request_queue_acquire(
  mln_adapter_resource_request_queue queue,
  mln_adapter_queued_resource_request** out_request
) MLN_NOEXCEPT;

/**
 * Closes a resource-request queue.
 *
 * Pending records and their request handles are released, and the notification
 * endpoint is detached before this function returns. A null or already
 * released queue is a no-op.
 */
MLN_API void mln_adapter_resource_request_queue_close(
  mln_adapter_resource_request_queue queue
) MLN_NOEXCEPT;

/**
 * Creates a log-record queue associated with one notification source.
 *
 * out_queue must point to the null handle. The association remains immutable
 * until the queue is closed.
 */
MLN_API mln_status mln_adapter_log_queue_create(
  mln_notification_source source, mln_adapter_log_queue* out_queue
) MLN_NOEXCEPT;

/**
 * Acquires the oldest copied log record, or null when the queue is empty.
 *
 * out_record must point to null. The caller owns a returned record and releases
 * it with mln_adapter_log_record_destroy().
 */
MLN_API mln_status mln_adapter_log_queue_acquire(
  mln_adapter_log_queue queue, mln_adapter_log_record** out_record
) MLN_NOEXCEPT;

/**
 * Closes a log queue.
 *
 * Pending records are released, and the notification endpoint is detached
 * before this function returns. A null or already released queue is a no-op.
 */
MLN_API void mln_adapter_log_queue_close(
  mln_adapter_log_queue queue
) MLN_NOEXCEPT;

/**
 * The mln_log_callback implementation for a log queue.
 *
 * user_data points to an mln_adapter_log_callback_state. Each record is copied
 * into its queue, and the callback reports the state's fixed consume value.
 */
MLN_API uint32_t mln_adapter_log_callback(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) MLN_NOEXCEPT;

/**
 * Installs state as the process-global log callback, or clears the current
 * callback when state is null.
 */
MLN_API mln_status mln_adapter_log_set_callback(
  mln_adapter_log_callback_state* state
) MLN_NOEXCEPT;

/** Releases a log record acquired from a log queue. */
MLN_API void mln_adapter_log_record_destroy(void* record) MLN_NOEXCEPT;

/**
 * The mln_resource_transform_callback implementation for rewrite rules.
 *
 * The user_data pointer is an mln_adapter_resource_rewrite_rules table. The
 * first matching rule replaces the URL, and a request that matches no rule
 * passes through unchanged.
 */
MLN_API mln_status mln_adapter_resource_transform_rewrite_callback(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* out_response
) MLN_NOEXCEPT;

/**
 * The mln_http_header_transform_callback implementation for native rules.
 *
 * The first rule whose kind and transformed URL match supplies all its headers.
 * A request with no matching rule proceeds unchanged. The callback returns the
 * first non-OK status from mln_http_header_transform_response_set().
 */
MLN_API mln_status mln_adapter_http_header_transform_callback(
  void* user_data, uint32_t kind, const char* url,
  mln_http_header_transform_response* out_response
) MLN_NOEXCEPT;

/**
 * Validates one null-terminated HTTP header from an adapter-owned rule table.
 *
 * This applies the C API's field-name, UTF-8 field-value, control-byte, and
 * transport-managed-name rules without requiring an active transform callback.
 * A diagnostic for a rejected header never includes its value.
 */
MLN_API mln_status mln_adapter_http_header_validate(
  const char* name, const char* value
) MLN_NOEXCEPT;

/**
 * The mln_resource_provider_callback implementation for provider rules.
 *
 * The user_data pointer is an mln_adapter_resource_provider_rules table. A
 * matching request is completed inline with the rule's response and reports
 * MLN_RESOURCE_PROVIDER_DECISION_HANDLE. Other requests pass through.
 */
MLN_API uint32_t mln_adapter_resource_provider_rules_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) MLN_NOEXCEPT;

/**
 * The mln_resource_provider_callback implementation for queued providers.
 *
 * user_data points to an mln_adapter_queued_resource_provider. A request
 * matching one route is copied into the provider's queue and reports
 * MLN_RESOURCE_PROVIDER_DECISION_HANDLE. Other requests pass through. A request
 * that cannot be copied is completed with an error response.
 */
MLN_API uint32_t mln_adapter_queued_resource_provider_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) MLN_NOEXCEPT;

/**
 * Releases the copied payload of a resource request acquired from a queue.
 *
 * Acquiring the record transfers its request handle to the host. The host
 * completes or releases that handle independently.
 */
MLN_API void mln_adapter_resource_provider_request_destroy(
  void* request
) MLN_NOEXCEPT;

/**
 * Invokes custom geometry tile callbacks once with a retirement tile id.
 *
 * The retirement tile id uses z = UINT8_MAX, which no real tile uses, so a host
 * listener recognizes it and releases the state behind the callbacks.
 */
MLN_API void mln_adapter_custom_geometry_callbacks_retire(
  mln_custom_geometry_source_tile_callback fetch_tile,
  mln_custom_geometry_source_tile_callback cancel_tile, void* user_data
) MLN_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_CALLBACK_ADAPTER_H
