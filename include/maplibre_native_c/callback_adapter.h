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
  mln_completion* out_completion MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
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

/**
 * Native-owned copy of one deferred callback call.
 *
 * A deferred callback typedef declares the result an adapter may return at
 * once. The adapter copies the call's arguments into arguments, laid out as the
 * generated arguments record for callback, and returns that result to
 * MapLibre. When the result accepts a decision, the record owns the decision
 * handle until the host adopts it with
 * mln_adapter_deferred_call_record_adopt().
 */
typedef struct mln_adapter_deferred_call_record {
  void* owner MLN_BINDING("kind=context;lifetime=owner");
  /** The mln_adapter_deferred_callback value naming the callback typedef. */
  uint32_t callback;
  /** Copied arguments, one generated mln_adapter_*_arguments record. */
  const void* arguments MLN_BINDING("kind=native_pointer;lifetime=owner");
} mln_adapter_deferred_call_record;

/**
 * Receives deferred calls on the thread that made them.
 *
 * Calls can arrive concurrently from several MapLibre threads. The listener
 * owns each non-null record and releases it with
 * mln_adapter_deferred_call_record_destroy(). It runs once more with a null
 * record when the context is released, after the final call, and frees
 * user_data then. A listener hands each record to the host's own execution
 * context, such as a queue it wakes, returns promptly, and never calls back
 * into MapLibre.
 */
typedef void (*mln_adapter_deferred_call_listener)(
  void* user_data MLN_BINDING("kind=context;lifetime=owner"),
  mln_adapter_deferred_call_record* record
    MLN_BINDING("ownership=owned;lifetime=owner;length=1;nullable=true")
) MLN_BINDING("thread=native;failure=contain");

/**
 * Creates a context that defers one callback typedef to a listener.
 *
 * Register mln_adapter_deferred_callback_function(callback) with the returned
 * context as its user_data and mln_adapter_deferred_callback_release() as its
 * release callback. The context calls listener for every call it defers. A
 * call the adapter cannot copy returns the callback's failure result and never
 * reaches the listener.
 *
 * Returns:
 * - MLN_STATUS_OK when out_context receives the context.
 * - MLN_STATUS_INVALID_ARGUMENT when callback is not an
 *   mln_adapter_deferred_callback value, listener is null, or out_context is
 *   null or does not point to null.
 * - MLN_STATUS_NATIVE_ERROR when adapter state could not be allocated.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_deferred_callback_create(
  uint32_t callback, mln_adapter_deferred_call_listener listener,
  void* listener_user_data MLN_BINDING("kind=context;lifetime=owner"),
  void** out_context MLN_BINDING("direction=out;kind=context;lifetime=owner"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Creates a deferred callback context that posts records to a Dart port.
 *
 * post_cobject is NativeApi.postCObject and port is a Dart SendPort.nativePort.
 * Each call posts an array of the callback value and the record as a native
 * pointer whose finalizer destroys an undelivered record. Release posts 0.
 * Other behavior matches mln_adapter_deferred_callback_create().
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_dart_deferred_callback_create(
  uint32_t callback,
  void* post_cobject MLN_BINDING("kind=native_pointer;lifetime=process"),
  int64_t port,
  void** out_context MLN_BINDING("direction=out;kind=context;lifetime=owner"),
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Returns the generated function that defers callback, or null.
 *
 * The function has the C signature of the callback typedef that callback
 * names. It returns the typedef's deferred result after delivering a copy.
 */
MLN_BINDING("execution=immediate;kind=native_pointer;ownership=borrowed")
MLN_API void* mln_adapter_deferred_callback_function(
  uint32_t callback
) MLN_NOEXCEPT;

/**
 * Releases a deferred callback context exactly once.
 *
 * This function has the signature of mln_runtime_callback_release and
 * mln_log_callback_release, so a registration passes it as its release
 * callback. Call it directly when the registering call rejects the context.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_deferred_callback_release(
  void* context MLN_BINDING("kind=context;lifetime=owner")
) MLN_NOEXCEPT;

/**
 * Transfers the record's decision handle to the host.
 *
 * Call this after constructing the host owner and before destroying the
 * record. A record without a decision handle is unchanged.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_deferred_call_record_adopt(
  mln_adapter_deferred_call_record* record MLN_BINDING("length=1")
) MLN_NOEXCEPT;

/**
 * Releases a deferred call record.
 *
 * A decision handle the host did not adopt fails its call: the adapter passes
 * a zeroed response to the decision's completion function, which the C API
 * converts to a provider error, and releases the handle. Null is a no-op.
 */
MLN_BINDING("execution=immediate")
MLN_API void mln_adapter_deferred_call_record_destroy(
  mln_adapter_deferred_call_record* record MLN_BINDING("length=1")
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
  uint32_t flags MLN_BINDING("enum=mln_adapter_url_match_flags");
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
  uint32_t flags MLN_BINDING("enum=mln_adapter_url_match_flags");
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
  uint32_t flags MLN_BINDING("enum=mln_adapter_url_match_flags");
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
 * How a resource route compares its url against a request.
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
 * One route a routed resource provider claims.
 *
 * The kind field matches mln_resource_kind values, or
 * MLN_ADAPTER_RESOURCE_KIND_ANY for every kind. The flags field is a bitwise OR
 * of mln_adapter_resource_route_flags values choosing which URL the route
 * compares and how; with no flags the route matches
 * mln_resource_request.resolved_url exactly.
 *
 * The url field is a comparison value, read literally or as a glob pattern
 * according to flags. A null url or an unknown flag bit makes the route match
 * nothing. The url pointer has the lifetime of its routed provider.
 */
typedef struct mln_adapter_resource_route {
  uint32_t kind;
  uint32_t flags MLN_BINDING("enum=mln_adapter_resource_route_flags");
  const char* url MLN_BINDING(
    "length=nul;encoding=utf8;ownership=borrowed;"
    "lifetime=owner;nullable=true"
  );
} mln_adapter_resource_route;

/**
 * A provider that forwards the requests its routes claim to another provider.
 *
 * A request that matches a route reaches callback with user_data, and the
 * routed provider returns that callback's decision. Every other request passes
 * through to native loading. The routes, their URLs, and user_data stay valid
 * through the terminal event of the command that replaces or clears this
 * provider. The routed provider never releases user_data; the owner of this
 * record releases both together.
 */
typedef struct mln_adapter_routed_resource_provider {
  const mln_adapter_resource_route* routes
    MLN_BINDING("length=route_count;ownership=borrowed;lifetime=owner");
  size_t route_count;
  mln_resource_provider_callback callback;
  void* user_data MLN_BINDING("kind=context;ownership=borrowed");
} mln_adapter_routed_resource_provider;

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
  int64_t port, mln_wake* out_wake MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
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
  mln_completion* out_completion MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
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
  void* arena MLN_BINDING("kind=context;lifetime=owner"), uint64_t handle,
  mln_diagnostic* out_diagnostic
) MLN_NOEXCEPT;

/**
 * Transfers a context release on entry; failure runs the release immediately.
 *
 * The arena calls release with context once when it is destroyed, before it
 * frees its allocations.
 */
MLN_BINDING("execution=immediate")
MLN_API mln_status mln_adapter_arena_adopt_release(
  void* arena MLN_BINDING("kind=context;lifetime=owner"),
  mln_runtime_callback_release release,
  void* context MLN_BINDING("kind=context;lifetime=owner"),
  mln_diagnostic* out_diagnostic
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
  uint64_t* out_registration MLN_BINDING("direction=out"),
  mln_diagnostic* out_diagnostic
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
    MLN_BINDING("encoding=utf8;lifetime=call;length=nul;ownership=borrowed"),
  mln_diagnostic* out_diagnostic
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
 * The mln_resource_provider_callback implementation for routed providers.
 *
 * user_data points to an mln_adapter_routed_resource_provider. A request that
 * matches one route reaches the provider's callback, and this function returns
 * its decision. Other requests, and every request of a provider without a
 * callback, report MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH.
 */
MLN_BINDING(
  "execution=immediate;callback_adapter=mln_resource_provider_callback;"
  "context_type=mln_adapter_routed_resource_provider"
)
MLN_API uint32_t mln_adapter_routed_resource_provider_callback(
  void* user_data MLN_BINDING("kind=context;lifetime=owner"),
  const mln_resource_request* request MLN_BINDING("length=1"),
  mln_resource_request_handle handle
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
  void** out_scope MLN_BINDING("direction=out;kind=context;lifetime=owner"),
  mln_diagnostic* out_diagnostic
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
