/**
 * @file maplibre_native_c/callback_adapter.h
 * Public C API declarations for adapting native callbacks to host runtimes
 * that cannot run user code on a native callback thread.
 *
 * MapLibre callback contracts are synchronous: logging and resource providers
 * return an immediate decision, and borrowed request payloads expire when the
 * callback returns. Some host runtimes cannot meet that contract. A host whose
 * callbacks are delivered asynchronously and return void has no way to answer
 * synchronously, and no way to read a payload that is already gone by the time
 * its user code runs.
 *
 * This layer answers on the host's behalf. It copies borrowed payloads into
 * native-owned records the host releases explicitly, applies native-owned
 * routing rules when a decision is needed immediately, and hands records to the
 * host through void listener functions. Host user code therefore runs on its
 * own execution context rather than on MapLibre worker, network, logging, or
 * render threads.
 *
 * A host that compiles its own native code writes this adaptation there
 * instead, in whatever form its runtime prefers. This header serves hosts that
 * consume the shared library through a pure foreign-function interface and have
 * no native compilation unit of their own.
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
#include "maplibre_native_c/runtime.h"  // IWYU pragma: export
#include "maplibre_native_c/style.h"    // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

/** Rule kind that matches every resource kind. */
#define MLN_ADAPTER_RESOURCE_KIND_ANY UINT32_MAX

/**
 * One exact-URL resource rewrite rule.
 *
 * The kind field matches mln_resource_kind values, or
 * MLN_ADAPTER_RESOURCE_KIND_ANY for every kind. A null replacement_url leaves
 * the URL unchanged. Both strings are borrowed and must outlive the rule table.
 */
typedef struct mln_adapter_resource_rewrite_rule {
  uint32_t kind;
  const char* url;
  const char* replacement_url;
} mln_adapter_resource_rewrite_rule;

/**
 * A borrowed table of rewrite rules.
 *
 * The rules pointer is borrowed and must stay valid while the table is
 * registered as resource transform user data.
 */
typedef struct mln_adapter_resource_rewrite_rules {
  const mln_adapter_resource_rewrite_rule* rules;
  size_t count;
} mln_adapter_resource_rewrite_rules;

/**
 * One exact-URL resource provider rule.
 *
 * A rule matches mln_resource_request.requested_url. A matching request is
 * completed with the rule's response without reaching the host. The response
 * and its buffers are borrowed and must outlive the rule table.
 */
typedef struct mln_adapter_resource_provider_rule {
  uint32_t kind;
  const char* requested_url;
  mln_resource_response response;
} mln_adapter_resource_provider_rule;

/**
 * A borrowed table of provider rules.
 *
 * The rules pointer is borrowed and must stay valid while the table is
 * registered as resource provider user data.
 */
typedef struct mln_adapter_resource_provider_rules {
  const mln_adapter_resource_provider_rule* rules;
  size_t count;
} mln_adapter_resource_provider_rules;

/**
 * How a queued provider route compares its url against a request.
 *
 * MLN_ADAPTER_RESOURCE_ROUTE_MATCH_PREFIX compares the url against the start of
 * the request URL instead of the whole request URL.
 * MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL selects
 * mln_resource_request.requested_url as the compared URL instead of
 * mln_resource_request.resolved_url. Setting both matches a requested-URL
 * prefix.
 */
typedef enum mln_adapter_resource_route_flags : uint32_t {
  MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE = 0U,
  MLN_ADAPTER_RESOURCE_ROUTE_MATCH_PREFIX = 1U << 0U,
  MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL = 1U << 1U,
} mln_adapter_resource_route_flags;

/**
 * One route a queued provider claims.
 *
 * The kind field matches mln_resource_kind values, or
 * MLN_ADAPTER_RESOURCE_KIND_ANY for every kind. The flags field is a bitwise OR
 * of mln_adapter_resource_route_flags values choosing which URL the route
 * compares and whether it compares a prefix; with no flags the route matches
 * mln_resource_request.resolved_url exactly.
 *
 * The url field is a literal comparison value. Comparison is case-sensitive and
 * applies no glob expansion, regular expressions, URL parsing, or
 * normalization, so an empty prefix matches every URL. A null url or an unknown
 * flag bit makes the route match nothing. The url pointer is borrowed and must
 * outlive the provider.
 */
typedef struct mln_adapter_queued_resource_provider_route {
  uint32_t kind;
  uint32_t flags;
  const char* url;
} mln_adapter_queued_resource_provider_route;

/**
 * Receives a queued request as a native-owned
 * mln_adapter_queued_resource_request, or null when the provider retires.
 *
 * The listener returns void and may be invoked from any MapLibre thread. It
 * takes ownership of the record and releases it with
 * mln_adapter_resource_provider_request_destroy() once the host has read it.
 */
typedef void (*mln_adapter_queued_resource_request_listener)(void* request);

/**
 * A provider that hands matching requests to a host listener.
 *
 * The routes pointer is borrowed and must stay valid while the provider is
 * registered.
 */
typedef struct mln_adapter_queued_resource_provider {
  const mln_adapter_queued_resource_provider_route* routes;
  size_t route_count;
  mln_adapter_queued_resource_request_listener listener;
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
  /** Copy of mln_resource_request.requested_url. */
  const char* requested_url;
  /** Copy of mln_resource_request.resolved_url. */
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
 * Receives a log record as a native-owned mln_adapter_log_record, or null when
 * the callback retires.
 *
 * The listener returns void and may be invoked from any MapLibre logging or
 * worker thread. It takes ownership of the record and releases it with
 * mln_adapter_log_record_destroy() once the host has read it.
 */
typedef void (*mln_adapter_log_record_listener)(void* record);

/**
 * Registration state for an adapted log callback.
 *
 * The consume field is the value reported to MapLibre for every dispatched
 * record, because the host cannot answer in time. The address of this struct
 * identifies the registration; it is borrowed and must stay valid until the
 * callback is replaced or cleared.
 */
typedef struct mln_adapter_log_callback_state {
  mln_adapter_log_record_listener listener;
  uint32_t consume;
} mln_adapter_log_callback_state;

/**
 * A native-owned copy of a log record.
 *
 * The message pointer is owned by this record and stays valid until
 * mln_adapter_log_record_destroy().
 */
typedef struct mln_adapter_log_record {
  void* owner;
  bool retire_callback;
  uint32_t severity;
  uint32_t event;
  int64_t code;
  const char* message;
} mln_adapter_log_record;

/**
 * Creates a token describing a handle the host has not closed yet.
 *
 * The token copies type_name and records handle so the report can name it.
 * Hosts attach the token to a finalizer so an unclosed handle can be reported
 * from a context that can no longer touch the handle itself.
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
 * The mln_log_callback implementation this layer registers.
 *
 * Copies the record, hands it to the registered listener, and reports the
 * registration's fixed consume value. The user_data pointer is the
 * mln_adapter_log_callback_state passed to mln_adapter_log_set_callback().
 */
MLN_API uint32_t mln_adapter_log_callback(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) MLN_NOEXCEPT;

/**
 * Installs state as the process-global log callback, or clears the current
 * callback when state is null.
 *
 * A registration this call replaces receives one final null record through its
 * listener once its in-flight dispatches finish.
 *
 * Returns:
 * - MLN_STATUS_OK on success.
 * - The status reported by mln_log_set_callback() or mln_log_clear_callback().
 */
MLN_API mln_status mln_adapter_log_set_callback(
  mln_adapter_log_callback_state* state
) MLN_NOEXCEPT;

/** Releases a log record delivered to a listener. */
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
 * The user_data pointer is an mln_adapter_queued_resource_provider. A request
 * matching one of the provider's routes is copied and handed to the listener,
 * and reports MLN_RESOURCE_PROVIDER_DECISION_HANDLE. Other requests pass
 * through unchanged and continue through the native loader. A request that
 * cannot be copied is completed with an error response rather than left
 * outstanding.
 */
MLN_API uint32_t mln_adapter_queued_resource_provider_callback(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) MLN_NOEXCEPT;

/** Releases a queued request record delivered to a listener. */
MLN_API void mln_adapter_resource_provider_request_destroy(
  void* request
) MLN_NOEXCEPT;

/**
 * Delivers one null record to a queued provider's listener.
 *
 * Hosts call this after the provider is no longer registered so the listener
 * can release the host-side state that backed it.
 */
MLN_API void mln_adapter_queued_resource_provider_retire(
  mln_adapter_queued_resource_provider* provider
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
