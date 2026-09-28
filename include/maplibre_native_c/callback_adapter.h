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
#include "maplibre_native_c/wake.h"     // IWYU pragma: export

#ifdef __cplusplus
extern "C" {
#endif

// NOLINTBEGIN(modernize-use-using,modernize-use-trailing-return-type)

/** Rule kind that matches every resource kind. */
#define MLN_ADAPTER_RESOURCE_KIND_ANY UINT32_MAX

#include "maplibre_native_c/callback_capture_generated.h"

/** Native-owned completion copy delivered to an asynchronous host listener. */
typedef struct mln_adapter_completion_record {
  void* owner MLN_BINDING("kind=context;lifetime=owner");
  mln_completion_result result;
} mln_adapter_completion_record;

/**
 * Receives one native-owned completion record on the host listener context.
 *
 * The listener runs exactly once for each accepted submission. record is null
 * when the adapter could not copy the completion result; the listener treats
 * that as a failed completion and destroys nothing. The adapter disposes any
 * owned result when capture fails. Otherwise the listener owns the record and
 * releases it with mln_adapter_completion_record_destroy().
 */
typedef void (*mln_adapter_completion_listener)(
  void* user_data MLN_BINDING("kind=context;lifetime=owner"),
  mln_adapter_completion_record* record
    MLN_BINDING("ownership=owned;lifetime=owner;length=1")
) MLN_BINDING("thread=host;failure=contain");

/**
 * Creates a completion descriptor that copies its borrowed result before
 * notifying an asynchronous host listener.
 *
 * out_completion must point to a zeroed descriptor. element_size is used only
 * with MLN_ADAPTER_COMPLETION_COPY_FLAT and may be zero for resultless calls.
 * The caller passes the descriptor to exactly one asynchronous C API call. If
 * that call rejects the submission, the caller must pass the descriptor to
 * mln_adapter_completion_reject().
 *
 * The host owns user_data for the life of the descriptor and frees it after the
 * listener returns, or after mln_adapter_completion_reject() returns for a
 * rejected submission. Neither path invokes the listener twice.
 *
 * Returns:
 * - MLN_STATUS_OK when out_completion receives the descriptor.
 * - MLN_STATUS_INVALID_ARGUMENT when out_completion is null or not zeroed,
 *   listener is null, or copy_kind is not an
 *   mln_adapter_completion_copy_kind value.
 * - MLN_STATUS_NATIVE_ERROR when adapter state could not be allocated.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_completion_create(
  uint32_t copy_kind, size_t element_size,
  mln_adapter_completion_listener listener,
  void* user_data MLN_BINDING("kind=context;lifetime=owner"),
  mln_completion* out_completion MLN_BINDING("direction=out")
) MLN_NOEXCEPT;

/** Releases adapter state after the submitting C API rejected a completion. */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_completion_reject(
  mln_completion* completion MLN_BINDING("length=1")
) MLN_NOEXCEPT;

/**
 * Transfers an owned completion result to the host after successful decoding.
 *
 * Call this after constructing the host owner and before destroying the record.
 * A borrowed result requires no adoption; adopting its record has no effect.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_completion_record_adopt(
  mln_adapter_completion_record* record MLN_BINDING("length=1")
) MLN_NOEXCEPT;

/**
 * Releases a completion record and disposes any owned result not yet adopted.
 *
 * A host that discards a delivery or fails to construct its result owner calls
 * this directly. Native handle disposal requires no completion allocation.
 */
MLN_BINDING("execution=immediate;kind=native_pointer;ownership=owned")
MLN_API void mln_adapter_completion_record_destroy(
  mln_adapter_completion_record* record MLN_BINDING("length=1")
) MLN_NOEXCEPT;

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
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_adapter_url_match_flags : uint32_t {
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
  const char* url MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
  const char* replacement_url MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
} mln_adapter_resource_rewrite_rule;

/**
 * A borrowed table of rewrite rules.
 *
 * The rules pointer and every rule string stay valid through the terminal event
 * of the command that replaces or clears this transform.
 */
typedef struct mln_adapter_resource_rewrite_rules {
  const mln_adapter_resource_rewrite_rule* rules
    MLN_BINDING("length=count;ownership=borrowed;lifetime=owner");
  size_t count;
} mln_adapter_resource_rewrite_rules;

/** One borrowed header supplied by an HTTP header transform rule. */
typedef struct mln_adapter_http_header {
  const char* name MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
  const char* value MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
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
  const char* url MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
  const mln_adapter_http_header* headers
    MLN_BINDING("length=header_count;ownership=borrowed;lifetime=owner");
  size_t header_count;
} mln_adapter_http_header_transform_rule;

/** A borrowed table of HTTP header transform rules. */
typedef struct mln_adapter_http_header_transform_rules {
  const mln_adapter_http_header_transform_rule* rules
    MLN_BINDING("length=count;ownership=borrowed;lifetime=owner");
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
  const char* requested_url MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
  mln_resource_response response;
} mln_adapter_resource_provider_rule;

/**
 * A borrowed table of provider rules.
 *
 * The rules pointer, response buffers, and rule strings stay valid through the
 * terminal event of the command that replaces or clears this provider.
 */
typedef struct mln_adapter_resource_provider_rules {
  const mln_adapter_resource_provider_rule* rules
    MLN_BINDING("length=count;ownership=borrowed;lifetime=owner");
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
typedef enum MLN_BINDING(
  "kind=bitmask"
) mln_adapter_resource_route_flags : uint32_t {
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
  const char* url MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
} mln_adapter_queued_resource_provider_route;

/**
 * A provider that copies matching requests into a native queue.
 *
 * The routes pointer and every route URL stay valid through the terminal event
 * of the command that replaces or clears this provider. queue identifies the
 * queue that receives each copied request.
 */
typedef struct mln_adapter_queued_resource_provider {
  const mln_adapter_queued_resource_provider_route* routes
    MLN_BINDING("length=route_count;ownership=borrowed;lifetime=owner");
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
  void* owner MLN_BINDING("kind=context;lifetime=owner");
  mln_resource_request_handle handle;
  /**
   * Copy of mln_resource_request.requested_url, or the empty string when the
   * request carried none. Never null, unlike prior_etag.
   */
  const char* requested_url MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
  /**
   * Copy of mln_resource_request.resolved_url, or the empty string when the
   * request carried none. Never null, unlike prior_etag.
   */
  const char* resolved_url MLN_BINDING(
    "length=nul;encoding=utf8;ownership="
    "borrowed;lifetime=owner;nullable=true"
  );
  uint32_t kind;
  uint32_t loading_method;
  uint32_t priority;
  uint32_t usage;
  uint32_t storage_policy;
  bool has_range MLN_BINDING("kind=presence_mask");
  uint64_t range_start MLN_BINDING("mask=has_range");
  uint64_t range_end MLN_BINDING("mask=has_range");
  bool has_prior_modified MLN_BINDING("kind=presence_mask");
  int64_t prior_modified_unix_ms MLN_BINDING("mask=has_prior_modified");
  bool has_prior_expires MLN_BINDING("kind=presence_mask");
  int64_t prior_expires_unix_ms MLN_BINDING("mask=has_prior_expires");
  const char* prior_etag MLN_BINDING(
    "length=nul;encoding=utf8;ownership="
    "borrowed;lifetime=owner;nullable=true"
  );
  const uint8_t* prior_data MLN_BINDING(
    "length=prior_data_size;encoding=bytes;"
    "ownership=borrowed;lifetime=owner"
  );
  size_t prior_data_size;
} mln_adapter_queued_resource_request MLN_BINDING(
  "projection=mln_resource_request"
);

/**
 * A native-owned copy of a log record.
 *
 * The message pointer is owned by this record and stays valid until
 * mln_adapter_log_record_destroy().
 */
typedef struct mln_adapter_log_record {
  void* owner MLN_BINDING("kind=context;lifetime=owner");
  uint32_t severity;
  uint32_t event;
  int64_t code;
  const char* message MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
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
  void* release_context MLN_BINDING("kind=context;lifetime=owner");
} mln_adapter_log_callback_state;

/**
 * Creates a wake that posts integer messages through Dart native API version 2.
 *
 * post_cobject is NativeApi.postCObject and port is a Dart SendPort.nativePort.
 * Callback messages contain 0; release messages contain 1. Posting to a closed
 * port safely discards the message. The native release callback frees the
 * descriptor context after posting. A rejected owning call invokes that release
 * callback directly to release the context.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_dart_wake_create(
  void* post_cobject MLN_BINDING("kind=native_pointer;lifetime=process"),
  int64_t port, mln_wake* out_wake MLN_BINDING("direction=out")
) MLN_NOEXCEPT;

/**
 * Captures a completion and posts its token and copied result to a Dart port.
 *
 * The VM releases an undelivered result through its native-pointer message
 * finalizer. Delivered results transfer to the binding's completion decoder.
 * Rejection uses mln_adapter_completion_reject.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_dart_completion_create(
  uint32_t copy_kind, size_t element_size,
  void* post_cobject MLN_BINDING("kind=native_pointer;lifetime=process"),
  int64_t port, int64_t token,
  mln_completion* out_completion MLN_BINDING("direction=out")
) MLN_NOEXCEPT;

/** Creates a native notification port context for generated void callbacks. */
MLN_BINDING("execution=immediate;kind=native_pointer;ownership=owned")
MLN_API void* mln_adapter_dart_port_create(
  void* post_cobject MLN_BINDING("kind=native_pointer;lifetime=process"),
  int64_t port
) MLN_NOEXCEPT;

/** Returns the generated callback address for a descriptor field identifier. */
MLN_BINDING("execution=immediate;kind=native_pointer;ownership=borrowed")
MLN_API void* mln_adapter_dart_port_function(uint32_t id) MLN_NOEXCEPT;

/** Retires the context once and posts zero after its queued notifications. */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_dart_port_release(
  void* context MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/** Creates a zero-initialized native allocation arena, or returns null. */
MLN_BINDING("execution=immediate;kind=native_pointer;ownership=owned")
MLN_API void* mln_adapter_arena_create(void) MLN_NOEXCEPT;

/** Allocates aligned zeroed memory that belongs to the arena. */
MLN_BINDING("execution=immediate;kind=native_pointer;ownership=borrowed")
MLN_API void* mln_adapter_arena_allocate(
  void* arena MLN_BINDING("kind=context;lifetime=owner"), size_t size,
  size_t alignment
) MLN_NOEXCEPT;

/** Releases the arena's allocations and disposes its adopted handles. */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_arena_destroy(
  void* arena MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/** Transfers a handle on entry; failure disposes the handle immediately. */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_arena_adopt_handle(
  void* arena MLN_BINDING("kind=context;lifetime=owner"), uint64_t handle
) MLN_NOEXCEPT;

/**
 * Registers a retained callback context and consumes its arena on entry.
 *
 * Native release destroys the arena even if the Dart isolate has closed.
 * The release message contains a unique registration identifier. Identifiers
 * remain distinct when a later arena reuses the same context address.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_dart_release_register(
  void* post_cobject MLN_BINDING("kind=native_pointer;lifetime=process"),
  int64_t port, void* context MLN_BINDING("kind=context;lifetime=owner"),
  void* arena MLN_BINDING("kind=context;lifetime=owner;nullable=true"),
  uint64_t* out_registration MLN_BINDING("direction=out")
) MLN_NOEXCEPT;

/** Removes a registration, posts its identifier and releases its native arena.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_dart_release(
  void* context MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/**
 * Creates a finalizer token for one owned native handle.
 *
 * Ownership transfers to the token on entry. Explicit release destroys the
 * token; finalization disposes the native owner. Allocation failure disposes
 * the owner and returns null.
 */
MLN_BINDING("execution=immediate;kind=native_pointer;ownership=owned")
MLN_API void* mln_adapter_owner_token_create(uint64_t handle) MLN_NOEXCEPT;

/** Releases a finalizer token after explicit owner release. */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_owner_token_destroy(
  void* token MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/** Disposes the token's native owner and releases the token on any thread. */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_owner_finalize(
  void* token MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/**
 * Creates a resource-request queue with a wake for its receiver.
 *
 * out_queue must point to the null handle. The association remains immutable
 * until the queue is closed.
 *
 * Returns:
 * - MLN_STATUS_OK when out_queue receives an owned queue.
 * - MLN_STATUS_INVALID_ARGUMENT when out_queue is null or does not point to the
 *   null handle, or the wake descriptor is invalid.
 * - MLN_STATUS_NATIVE_ERROR when the queue could not be allocated.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_resource_request_queue_create(
  const mln_wake* wake MLN_BINDING("length=1"),
  mln_adapter_resource_request_queue* out_queue
    MLN_BINDING("direction=out;ownership=owned")
) MLN_NOEXCEPT;

/**
 * Acquires the oldest queued request, or null when the queue is empty.
 *
 * out_request must point to null. The caller owns a returned record and
 * releases it with mln_adapter_resource_provider_request_destroy(). The queue
 * remains ready until this drain confirms it is empty.
 *
 * Returns:
 * - MLN_STATUS_OK when out_request receives a record or the queue is empty.
 * - MLN_STATUS_INVALID_ARGUMENT when queue is null or not live, or out_request
 *   is null or does not point to null.
 * - MLN_STATUS_INVALID_STATE when the queue is closed or another drain is
 *   active.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_resource_request_queue_acquire(
  mln_adapter_resource_request_queue queue,
  mln_adapter_queued_resource_request** out_request
    MLN_BINDING("direction=out;ownership=owned;length=1")
) MLN_NOEXCEPT;

/**
 * Closes a resource-request queue.
 *
 * Pending records and their request handles are released, and the wake is
 * detached before this function returns. A null or already released queue is
 * a no-op.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_resource_request_queue_close(
  mln_adapter_resource_request_queue queue
) MLN_NOEXCEPT;

/**
 * Creates a log-record queue with a wake for its receiver.
 *
 * out_queue must point to the null handle. The association remains immutable
 * until the queue is closed.
 *
 * Returns:
 * - MLN_STATUS_OK when out_queue receives an owned queue.
 * - MLN_STATUS_INVALID_ARGUMENT when out_queue is null or does not point to the
 *   null handle, or the wake descriptor is invalid.
 * - MLN_STATUS_NATIVE_ERROR when the queue could not be allocated.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_log_queue_create(
  const mln_wake* wake MLN_BINDING("length=1"),
  mln_adapter_log_queue* out_queue MLN_BINDING("direction=out;ownership=owned")
) MLN_NOEXCEPT;

/**
 * Acquires the oldest copied log record, or null when the queue is empty.
 *
 * out_record must point to null. The caller owns a returned record and releases
 * it with mln_adapter_log_record_destroy().
 *
 * Returns:
 * - MLN_STATUS_OK when out_record receives a record or the queue is empty.
 * - MLN_STATUS_INVALID_ARGUMENT when queue is null or not live, or out_record
 *   is null or does not point to null.
 * - MLN_STATUS_INVALID_STATE when the queue is closed or another drain is
 *   active.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_log_queue_acquire(
  mln_adapter_log_queue queue,
  mln_adapter_log_record** out_record
    MLN_BINDING("direction=out;ownership=owned;length=1")
) MLN_NOEXCEPT;

/**
 * Closes a log queue.
 *
 * Pending records are released, and the wake is detached before this function
 * returns. A null or already released queue is a no-op.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_log_queue_close(
  mln_adapter_log_queue queue
) MLN_NOEXCEPT;

/**
 * The mln_log_callback implementation for a log queue.
 *
 * user_data points to an mln_adapter_log_callback_state. Each record is copied
 * into its queue, and the callback reports the state's fixed consume value.
 */
MLN_BINDING(
  "execution=immediate;callback_adapter=mln_log_callback;context_"
  "type=mln_adapter_log_callback_state"
)
MLN_API uint32_t mln_adapter_log_callback(
  void* user_data MLN_BINDING("kind=context;lifetime=owner"), uint32_t severity,
  uint32_t event, int64_t code,
  const char* message
    MLN_BINDING("encoding=utf8;lifetime=call;length=nul;ownership=borrowed")
) MLN_NOEXCEPT;

/**
 * Installs state as the process-global log callback, or clears the current
 * callback when state is null.
 *
 * Returns:
 * - MLN_STATUS_OK when the registration was installed or cleared.
 * - MLN_STATUS_INVALID_ARGUMENT when state names a queue that is not live.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_log_set_callback(
  mln_adapter_log_callback_state* state MLN_BINDING("length=1")
) MLN_NOEXCEPT;

/** Releases a log record acquired from a log queue. */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_log_record_destroy(
  void* record MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/**
 * The mln_resource_transform_callback implementation for rewrite rules.
 *
 * The user_data pointer is an mln_adapter_resource_rewrite_rules table. The
 * first matching rule replaces the URL, and a request that matches no rule
 * passes through unchanged.
 *
 * Returns:
 * - MLN_STATUS_OK when the URL was rewritten or left unchanged, including for
 *   null arguments.
 * - the status of mln_resource_transform_response_set_url() when the copy of a
 *   replacement URL fails.
 */
MLN_BINDING(
  "execution=immediate;callback_adapter=mln_resource_transform_"
  "callback;context_type=mln_adapter_resource_rewrite_rules;invokes="
  "mln_resource_transform_response_set_url"
)
MLN_API mln_status mln_adapter_resource_transform_rewrite_callback(
  void* user_data MLN_BINDING("kind=context;lifetime=owner"), uint32_t kind,
  const char* url
    MLN_BINDING("encoding=utf8;lifetime=call;length=nul;ownership=borrowed"),
  mln_resource_transform_response* out_response MLN_BINDING("direction=out")
) MLN_NOEXCEPT;

/**
 * The mln_http_header_transform_callback implementation for native rules.
 *
 * The first rule whose kind and transformed URL match supplies all its headers.
 * A request with no matching rule proceeds unchanged.
 *
 * Returns:
 * - MLN_STATUS_OK when the matching rule's headers were recorded, or no rule
 *   matched, including for null arguments.
 * - MLN_STATUS_INVALID_ARGUMENT when a rule table or header array is null with
 *   a non-zero count.
 * - the first non-OK status from mln_http_header_transform_response_set().
 */
MLN_BINDING(
  "execution=immediate;callback_adapter=mln_http_header_transform_"
  "callback;context_type=mln_adapter_http_header_transform_rules;"
  "invokes=mln_http_header_transform_response_set"
)
MLN_API mln_status mln_adapter_http_header_transform_callback(
  void* user_data MLN_BINDING("kind=context;lifetime=owner"), uint32_t kind,
  const char* url
    MLN_BINDING("encoding=utf8;lifetime=call;length=nul;ownership=borrowed"),
  mln_http_header_transform_response* out_response MLN_BINDING("direction=out")
) MLN_NOEXCEPT;

/**
 * Validates one null-terminated HTTP header from an adapter-owned rule table.
 *
 * This applies the C API's field-name, UTF-8 field-value, control-byte, and
 * transport-managed-name rules without requiring an active transform callback.
 * A diagnostic for a rejected header never includes its value.
 *
 * Returns:
 * - MLN_STATUS_OK when the header is valid.
 * - MLN_STATUS_INVALID_ARGUMENT when the name or value breaks one of those
 *   rules.
 * - MLN_STATUS_NATIVE_ERROR when an internal exception is converted to status.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_http_header_validate(
  const char* name
    MLN_BINDING("encoding=utf8;lifetime=call;length=nul;ownership=borrowed"),
  const char* value
    MLN_BINDING("encoding=utf8;lifetime=call;length=nul;ownership=borrowed")
) MLN_NOEXCEPT;

/**
 * The mln_resource_provider_callback implementation for provider rules.
 *
 * The user_data pointer is an mln_adapter_resource_provider_rules table. A
 * matching request is completed inline with the rule's response and reports
 * MLN_RESOURCE_PROVIDER_DECISION_HANDLE. Other requests pass through.
 */
MLN_BINDING(
  "execution=immediate;callback_adapter=mln_resource_provider_"
  "callback;context_type=mln_adapter_resource_provider_rules;invokes="
  "mln_resource_request_complete,mln_resource_request_release"
)
MLN_API uint32_t mln_adapter_resource_provider_rules_callback(
  void* user_data MLN_BINDING("kind=context;lifetime=owner"),
  const mln_resource_request* request MLN_BINDING("length=1"),
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
MLN_BINDING(
  "execution=immediate;callback_adapter=mln_resource_provider_callback;"
  "context_type=mln_adapter_queued_resource_provider;invokes=mln_resource_"
  "request_complete,mln_resource_request_release"
)
MLN_API uint32_t mln_adapter_queued_resource_provider_callback(
  void* user_data MLN_BINDING("kind=context;lifetime=owner"),
  const mln_resource_request* request MLN_BINDING("length=1"),
  mln_resource_request_handle handle
) MLN_NOEXCEPT;

/**
 * Releases the copied payload of a resource request acquired from a queue.
 *
 * Acquiring the record transfers its request handle to the host. The host
 * completes or releases that handle independently.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_resource_provider_request_destroy(
  void* request MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/**
 * Invokes custom geometry tile callbacks once with a retirement tile id.
 *
 * The retirement tile id uses z = UINT8_MAX, which no real tile uses, so a host
 * listener recognizes it and releases the state behind the callbacks.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_custom_geometry_callbacks_retire(
  mln_custom_geometry_source_tile_callback fetch_tile,
  mln_custom_geometry_source_tile_callback cancel_tile,
  void* user_data MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/**
 * Invokes custom MVT vector tile callbacks once with a retirement tile id.
 *
 * The retirement tile id uses z = UINT8_MAX, which no real tile uses, so a host
 * listener recognizes it and releases the state behind the callbacks.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_custom_mvt_vector_callbacks_retire(
  mln_custom_mvt_vector_source_tile_callback fetch_tile,
  mln_custom_mvt_vector_source_tile_callback cancel_tile,
  void* user_data MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/**
 * Begins an allocation-free borrowed-frame use scope.
 *
 * The scope retains its frame and session until view_end. Disposal invalidates
 * later scopes immediately and waits for existing scopes before retiring the
 * graphics resources. Explicit frame release and session abandon report BUSY
 * while a scope is active. Every successful begin requires exactly one end.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_acquired_frame_view_begin(
  mln_acquired_frame frame,
  void** out_scope MLN_BINDING("direction=out;kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/** Ends one borrowed-frame scope. Null is a no-op. */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_acquired_frame_view_end(
  void* scope MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

// NOLINTEND(modernize-use-using,modernize-use-trailing-return-type)

#ifdef __cplusplus
}
#endif

#endif  // MAPLIBRE_NATIVE_C_CALLBACK_ADAPTER_H
