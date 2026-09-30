// Raw C ABI coverage for the callback adapters: routed provider matching for
// route descriptors a binding's own route type cannot express, a request that
// carries no requested URL, and the glob language every binding shares; and
// deferred callbacks, which answer MapLibre at once and hand a copy of each
// call to a listener.
//
// Route matching happens before the handle is read, so those tests drive the
// callback directly with a synthesized request rather than through a loader.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "maplibre_native_c/callback_adapter.h"

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static const char alias_url[] = "maplibre://maps/style";
static const char normalized_url[] =
  "https://demotiles.maplibre.org/style.json";
static const uint8_t inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

// A handle the loader never issued. Route matching never reads it.
static const mln_resource_request_handle unissued_handle = 1;

static size_t routed_requests;
static const mln_resource_request* last_routed_request;

static size_t completion_listener_calls;
static bool completion_listener_received_null;

static void completion_listener(
  void* user_data, mln_adapter_completion_record* record
) {
  (void)user_data;
  completion_listener_calls += 1;
  completion_listener_received_null = record == NULL;
  mln_adapter_completion_record_destroy(record);
}

static mln_resource_request style_request(void) {
  return (mln_resource_request){
    .size = sizeof(mln_resource_request),
    .requested_url = alias_url,
    .resolved_url = normalized_url,
    .kind = MLN_RESOURCE_KIND_STYLE,
    .loading_method = MLN_RESOURCE_LOADING_METHOD_ALL,
    .priority = MLN_RESOURCE_PRIORITY_REGULAR,
    .usage = MLN_RESOURCE_USAGE_ONLINE,
    .storage_policy = MLN_RESOURCE_STORAGE_POLICY_PERMANENT,
  };
}

// The inner provider of a routed provider: it claims what reaches it.
static uint32_t routed_provider_stub(
  void* user_data, const mln_resource_request* request,
  mln_resource_request_handle handle
) {
  (void)user_data;
  (void)handle;
  routed_requests += 1;
  last_routed_request = request;
  return MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

// Reports the decision one route produces for one request.
static uint32_t route_decision(
  mln_adapter_resource_route route, const mln_resource_request* request
) {
  const mln_adapter_routed_resource_provider provider = {
    .routes = &route,
    .route_count = 1,
    .callback = routed_provider_stub,
  };
  return mln_adapter_routed_resource_provider_callback(
    (void*)&provider, request, unissued_handle
  );
}

static void assert_claims(
  mln_adapter_resource_route route, const mln_resource_request* request
) {
  const size_t before = routed_requests;
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_HANDLE, route_decision(route, request)
  );
  TEST_ASSERT_EQUAL_size_t(before + 1, routed_requests);
  TEST_ASSERT_EQUAL_PTR(request, last_routed_request);
}

static void assert_passes_through(
  mln_adapter_resource_route route, const mln_resource_request* request
) {
  const size_t before = routed_requests;
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH, route_decision(route, request)
  );
  TEST_ASSERT_EQUAL_size_t(before, routed_requests);
}

// A route with no comparable URL matches nothing rather than claiming every
// request the way `**` does.
static void routed_provider_rejects_raw_invalid_route_descriptors(void) {
  const mln_resource_request request = style_request();

  assert_passes_through(
    (mln_adapter_resource_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE,
      .url = NULL,
    },
    &request
  );
  assert_passes_through(
    (mln_adapter_resource_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
      .url = NULL,
    },
    &request
  );
  assert_passes_through(
    (mln_adapter_resource_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB | (1U << 31U),
      .url = "**",
    },
    &request
  );
  assert_passes_through(
    (mln_adapter_resource_route){
      .kind = MLN_RESOURCE_KIND_TILE,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
      .url = "**",
    },
    &request
  );

  // A provider without an inner callback passes everything through.
  const mln_adapter_resource_route route = {
    .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
    .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
    .url = "**",
  };
  const mln_adapter_routed_resource_provider provider = {
    .routes = &route,
    .route_count = 1,
  };
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    mln_adapter_routed_resource_provider_callback(
      (void*)&provider, &request, unissued_handle
    )
  );
}

// A route compares the URL its flags select, so a request without a requested
// URL still reaches a resolved-URL route.
static void routed_provider_tolerates_raw_absent_request_urls(void) {
  mln_resource_request request = style_request();
  request.requested_url = NULL;

  assert_claims(
    (mln_adapter_resource_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
      .url = "**",
    },
    &request
  );
  assert_passes_through(
    (mln_adapter_resource_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB |
               MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL,
      .url = "**",
    },
    &request
  );
}

// Reports whether one glob pattern claims one resolved URL.
static bool glob_claims(const char* pattern, const char* resolved_url) {
  mln_resource_request request = style_request();
  request.resolved_url = resolved_url;
  return route_decision(
           (mln_adapter_resource_route){
             .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
             .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
             .url = pattern,
           },
           &request
         ) == MLN_RESOURCE_PROVIDER_DECISION_HANDLE;
}

static void glob_routes_confine_a_star_to_one_path_segment(void) {
  static const char host_pattern[] = "https://*.maplibre.org/**";

  TEST_ASSERT_TRUE(glob_claims(host_pattern, normalized_url));
  TEST_ASSERT_TRUE(
    glob_claims(host_pattern, "https://tiles.maplibre.org/1/2/3.pbf")
  );
  // A path segment carrying the host name would match if `*` crossed
  // separators.
  TEST_ASSERT_FALSE(
    glob_claims(host_pattern, "https://attacker.test/x.maplibre.org/style.json")
  );
  // A single `*` spans one segment, and `**` spans the rest of the URL.
  TEST_ASSERT_TRUE(glob_claims("https://*/style.json", normalized_url));
  TEST_ASSERT_TRUE(glob_claims("https://*.maplibre.org/*", normalized_url));
  TEST_ASSERT_FALSE(glob_claims("https://*.maplibre.org/*/*", normalized_url));
  TEST_ASSERT_TRUE(glob_claims("**", normalized_url));
  TEST_ASSERT_FALSE(glob_claims("*", normalized_url));
}

static void glob_routes_anchor_match_wildcards_and_escapes(void) {
  // A pattern matches the complete URL, so a bare suffix or infix claims
  // nothing without a leading wildcard.
  TEST_ASSERT_TRUE(glob_claims("**.json", normalized_url));
  TEST_ASSERT_FALSE(glob_claims(".json", normalized_url));
  TEST_ASSERT_FALSE(glob_claims("**.pbf", normalized_url));
  TEST_ASSERT_TRUE(glob_claims("https://demotiles**", normalized_url));

  // A pattern with no metacharacters compares byte for byte.
  TEST_ASSERT_TRUE(glob_claims(normalized_url, normalized_url));
  TEST_ASSERT_FALSE(glob_claims("", normalized_url));
  TEST_ASSERT_TRUE(glob_claims("", ""));

  // `?` spans one character other than a separator, and `\` makes the next
  // character literal.
  TEST_ASSERT_TRUE(
    glob_claims("?ttps://demotiles.maplibre.org/style.json", normalized_url)
  );
  TEST_ASSERT_TRUE(
    glob_claims("https://demotiles.maplibre.org/style?json", normalized_url)
  );
  TEST_ASSERT_FALSE(
    glob_claims("https://demotiles.maplibre.org/style\\?json", normalized_url)
  );
  TEST_ASSERT_FALSE(glob_claims("https:?**", normalized_url));
  TEST_ASSERT_TRUE(
    glob_claims("https://star\\*.test/x", "https://star*.test/x")
  );
  TEST_ASSERT_FALSE(
    glob_claims("https://star\\*.test/x", "https://starry.test/x")
  );
}

static void glob_routes_backtrack_across_wildcard_runs(void) {
  TEST_ASSERT_TRUE(
    glob_claims("https://**/tiles/*", "https://host/tiles/a/b/tiles/x")
  );
}

static mln_status header_route_status(
  const mln_adapter_http_header_transform_rule* rules, size_t count,
  uint32_t kind, const char* url
) {
  mln_adapter_http_header_transform_rules table = {
    .rules = rules,
    .count = count,
  };
  mln_http_header_transform_response response = {
    .size = sizeof(mln_http_header_transform_response),
  };
  return mln_adapter_http_header_transform_callback(
    &table, kind, url, &response
  );
}

static void http_header_routes_match_exact_glob_kind_and_order(void) {
  const mln_adapter_http_header invalid[] = {{.name = "Host", .value = "bad"}};
  const mln_adapter_http_header_transform_rule rules[] = {
    {
      .kind = MLN_RESOURCE_KIND_TILE,
      .flags = MLN_ADAPTER_URL_MATCH_FLAGS_NONE,
      .url = "https://tiles.test/exact",
      .headers = invalid,
      .header_count = 1,
    },
    {
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_URL_MATCH_GLOB,
      .url = "https://tiles.test/**",
      .headers = NULL,
      .header_count = 0,
    },
  };

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    header_route_status(
      rules, 2, MLN_RESOURCE_KIND_TILE, "https://tiles.test/exact"
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    header_route_status(
      rules, 2, MLN_RESOURCE_KIND_STYLE, "https://tiles.test/exact"
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, header_route_status(
                     rules, 2, MLN_RESOURCE_KIND_TILE, "https://elsewhere.test/"
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    header_route_status(NULL, 1, MLN_RESOURCE_KIND_TILE, "https://tiles.test/")
  );
}

static void http_header_validation_uses_the_native_policy(void) {
  static const char invalid_utf8[] = {'b', 'a', 'd', (char)0xFF, '\0'};

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_adapter_http_header_validate("X-Test", "caf\xC3\xA9", NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_http_header_validate("Bad Name", "value", NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_http_header_validate("Range", "bytes=0-1", NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_http_header_validate("Authorization", "bad\r\nvalue", NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_http_header_validate("Authorization", invalid_utf8, NULL)
  );
}

static void completion_copy_failures_use_the_adapter_failure_channel(void) {
  completion_listener_calls = 0;
  completion_listener_received_null = false;
  mln_completion completion = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_completion_create(
                     MLN_ADAPTER_COMPLETION_COPY_TEXTURE_READBACK_RESULT, 0,
                     completion_listener, NULL, &completion, NULL
                   )
  );

  // A texture readback completion must carry exactly one result. The invalid
  // shape deterministically exercises the adapter's copy-failure path.
  const mln_texture_readback_result value = {0};
  const mln_completion_result result = {
    .size = sizeof(mln_completion_result),
    .status = MLN_STATUS_OK,
    .value = &value,
    .value_count = 2,
  };
  completion.callback(completion.user_data, &result);
  TEST_ASSERT_EQUAL_size_t(1, completion_listener_calls);
  TEST_ASSERT_TRUE(completion_listener_received_null);
  completion.release_user_data(completion.user_data);
}

// Collects what one deferred callback context hands its listener. The listener
// runs on the calling thread, which is a MapLibre thread for registered
// contexts, so its counters are atomic.
typedef struct deferred_probe {
  atomic_size_t records;
  atomic_size_t releases;
  atomic_bool delivered;
  // Keeps the last record for the test thread instead of destroying it.
  bool keep;
  _Atomic(mln_adapter_deferred_call_record*) kept;
} deferred_probe;

static void deferred_listener(
  void* user_data, mln_adapter_deferred_call_record* record
) {
  deferred_probe* probe = user_data;
  if (record == NULL) {
    atomic_fetch_add(&probe->releases, 1);
    return;
  }
  atomic_fetch_add(&probe->records, 1);
  if (probe->keep) {
    atomic_store(&probe->kept, record);
  } else {
    mln_adapter_deferred_call_record_destroy(record);
  }
  mln_test_flag_set(&probe->delivered);
}

static void* deferred_context(uint32_t callback, deferred_probe* probe) {
  void* context = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_deferred_callback_create(
                     callback, deferred_listener, probe, &context, NULL
                   )
  );
  TEST_ASSERT_NOT_NULL(context);
  return context;
}

static mln_log_callback deferred_log_callback(void) {
  void* address =
    mln_adapter_deferred_callback_function(MLN_ADAPTER_DEFERRED_LOG_CALLBACK);
  TEST_ASSERT_NOT_NULL(address);
  mln_log_callback callback = NULL;
  memcpy(&callback, &address, sizeof(callback));
  return callback;
}

static mln_resource_provider_callback deferred_provider_callback(void) {
  void* address = mln_adapter_deferred_callback_function(
    MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK
  );
  TEST_ASSERT_NOT_NULL(address);
  mln_resource_provider_callback callback = NULL;
  memcpy(&callback, &address, sizeof(callback));
  return callback;
}

static void deferred_callbacks_reject_raw_invalid_arguments(void) {
  deferred_probe probe = {0};
  void* context = NULL;
  TEST_ASSERT_NULL(mln_adapter_deferred_callback_function(0));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_adapter_deferred_callback_create(
                                   0, deferred_listener, &probe, &context, NULL
                                 )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_deferred_callback_create(
      MLN_ADAPTER_DEFERRED_LOG_CALLBACK, NULL, &probe, &context, NULL
    )
  );
  context = &probe;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_adapter_deferred_callback_create(
                                   MLN_ADAPTER_DEFERRED_LOG_CALLBACK,
                                   deferred_listener, &probe, &context, NULL
                                 )
  );
  TEST_ASSERT_EQUAL_PTR(&probe, context);
  TEST_ASSERT_EQUAL_size_t(0, atomic_load(&probe.releases));
}

// The log adapter consumes the record at once and delivers a copy that
// outlives the borrowed message.
static void deferred_log_callback_consumes_and_delivers_a_copy(void) {
  deferred_probe probe = {.keep = true};
  void* context = deferred_context(MLN_ADAPTER_DEFERRED_LOG_CALLBACK, &probe);
  char message[] = "borrowed message";

  TEST_ASSERT_EQUAL_UINT32(
    1, deferred_log_callback()(
         context, MLN_LOG_SEVERITY_WARNING, MLN_LOG_EVENT_STYLE, 42, message
       )
  );
  memset(message, 'X', sizeof(message) - 1);

  mln_adapter_deferred_call_record* record = atomic_load(&probe.kept);
  TEST_ASSERT_NOT_NULL(record);
  TEST_ASSERT_EQUAL_UINT32(MLN_ADAPTER_DEFERRED_LOG_CALLBACK, record->callback);
  const mln_adapter_log_callback_arguments* arguments = record->arguments;
  TEST_ASSERT_EQUAL_UINT32(MLN_LOG_SEVERITY_WARNING, arguments->severity);
  TEST_ASSERT_EQUAL_UINT32(MLN_LOG_EVENT_STYLE, arguments->event);
  TEST_ASSERT_EQUAL_INT64(42, arguments->code);
  TEST_ASSERT_EQUAL_STRING("borrowed message", arguments->message);
  mln_adapter_deferred_call_record_destroy(record);

  // A context answers only its own callback typedef, with that callback's
  // failure result.
  const mln_resource_request request = style_request();
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    deferred_provider_callback()(context, &request, unissued_handle)
  );
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.records));

  mln_adapter_deferred_callback_release(context);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
}

// A provider call the adapter cannot copy takes the callback's failure result,
// so native loading keeps the request.
static void deferred_provider_passes_through_an_uncopyable_request(void) {
  deferred_probe probe = {0};
  void* context =
    deferred_context(MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, &probe);
  mln_resource_request request = style_request();
  request.prior_data = NULL;
  request.prior_data_size = 4;

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    deferred_provider_callback()(context, &request, unissued_handle)
  );
  TEST_ASSERT_EQUAL_size_t(0, atomic_load(&probe.records));
  mln_adapter_deferred_callback_release(context);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
}

static bool wait_for_map_event(
  mln_runtime runtime, uint32_t type, mln_map map, char* message,
  size_t message_capacity
) {
  mln_runtime_event event = {0};
  return mln_test_await_event(
    runtime, type, map, &event, message, message_capacity
  );
}

static mln_status set_provider_committed(
  mln_runtime runtime, const mln_resource_provider* provider
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_resource_provider(
    runtime, provider, &completion.descriptor, NULL
  );
  if (status != MLN_STATUS_OK) {
    mln_test_completion_reject(&completion);
    mln_test_completion_destroy(&completion);
    return status;
  }
  return mln_test_completion_settle(&completion);
}

static mln_status clear_provider_committed(mln_runtime runtime) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status =
    mln_runtime_clear_resource_provider(runtime, &completion.descriptor, NULL);
  if (status != MLN_STATUS_OK) {
    mln_test_completion_reject(&completion);
    mln_test_completion_destroy(&completion);
    return status;
  }
  return mln_test_completion_settle(&completion);
}

// A routed deferred provider claims its route, answers HANDLE at once, and
// delivers a record whose adopted handle the host completes later. The style
// load finishes with the host's response.
static void routed_deferred_provider_delivers_a_request_the_host_completes(
  void
) {
  static const char style_url[] = "custom://deferred-provider-style.json";
  deferred_probe probe = {.keep = true};
  void* context =
    deferred_context(MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, &probe);
  const mln_adapter_resource_route route = {
    .kind = MLN_RESOURCE_KIND_STYLE,
    .flags = MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL,
    .url = style_url,
  };
  const mln_adapter_routed_resource_provider routed = {
    .routes = &route,
    .route_count = 1,
    .callback = deferred_provider_callback(),
    .user_data = context,
  };
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = mln_adapter_routed_resource_provider_callback,
    .user_data = (void*)&routed,
  };
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_provider_committed(runtime, &provider)
  );
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_url(map, style_url)
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&probe.delivered));

  mln_adapter_deferred_call_record* record = atomic_load(&probe.kept);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, record->callback
  );
  const mln_adapter_resource_provider_callback_arguments* arguments =
    record->arguments;
  TEST_ASSERT_EQUAL_STRING(style_url, arguments->request->requested_url);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_STYLE, arguments->request->kind);
  const mln_resource_request_handle handle = arguments->handle;
  mln_adapter_deferred_call_record_adopt(record);
  mln_adapter_deferred_call_record_destroy(record);

  const mln_resource_response response = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .bytes = inline_style_json,
    .byte_count = sizeof(inline_style_json) - 1,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_resource_request_complete(handle, &response, NULL)
  );
  mln_resource_request_release(handle);
  TEST_ASSERT_TRUE(wait_for_map_event(
    runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED, map, NULL, 0
  ));

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, clear_provider_committed(runtime));
  mln_adapter_deferred_callback_release(context);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.records));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A record the host destroys without adopting its handle fails the request,
// so the style load reports an error instead of waiting. The registration
// releases its context once.
static void unadopted_deferred_request_fails_and_releases_once(void) {
  static const char style_url[] = "custom://deferred-provider-dropped.json";
  deferred_probe probe = {0};
  void* context =
    deferred_context(MLN_ADAPTER_DEFERRED_RESOURCE_PROVIDER_CALLBACK, &probe);
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = deferred_provider_callback(),
    .user_data = context,
    .release_user_data = mln_adapter_deferred_callback_release,
  };
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, set_provider_committed(runtime, &provider)
  );
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_set_style_url(map, style_url)
  );
  char message[256] = {0};
  TEST_ASSERT_TRUE(wait_for_map_event(
    runtime, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, map, message, sizeof(message)
  ));
  TEST_ASSERT_NOT_NULL(strstr(message, "released without a response"));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.records));

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, clear_provider_committed(runtime));
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_size_t(1, atomic_load(&probe.releases));
}

MLN_TEST_GROUP {
  RUN_TEST(routed_provider_rejects_raw_invalid_route_descriptors);
  RUN_TEST(routed_provider_tolerates_raw_absent_request_urls);
  RUN_TEST(glob_routes_confine_a_star_to_one_path_segment);
  RUN_TEST(glob_routes_anchor_match_wildcards_and_escapes);
  RUN_TEST(glob_routes_backtrack_across_wildcard_runs);
  RUN_TEST(http_header_routes_match_exact_glob_kind_and_order);
  RUN_TEST(http_header_validation_uses_the_native_policy);
  RUN_TEST(completion_copy_failures_use_the_adapter_failure_channel);
  RUN_TEST(deferred_callbacks_reject_raw_invalid_arguments);
  RUN_TEST(deferred_log_callback_consumes_and_delivers_a_copy);
  RUN_TEST(deferred_provider_passes_through_an_uncopyable_request);
  RUN_TEST(routed_deferred_provider_delivers_a_request_the_host_completes);
  RUN_TEST(unadopted_deferred_request_fails_and_releases_once);
}
