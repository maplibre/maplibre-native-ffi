// Raw C ABI coverage for queued provider route matching: route descriptors a
// binding's own route type cannot express, a request that carries no requested
// URL, and the glob language every binding shares.
//
// The adapter decides a route before it reads the handle, so these drive the
// callback directly with a synthesized request rather than through a loader.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "abi_tests.h"
#include "maplibre_native_c/callback_adapter.h"
#include "test_support.h"
#include "unity.h"

static const char alias_url[] = "maplibre://maps/style";
static const char normalized_url[] =
  "https://demotiles.maplibre.org/style.json";

// A handle the loader never issued. Releasing it after inspecting the copied
// record is a no-op.
static const mln_resource_request_handle unissued_handle = 1;

static size_t queued_requests;
static bool last_requested_url_null;
static bool last_requested_url_empty;

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

// Reports the decision one route produces for one request, and drains the
// copied record when the route claimed it.
static uint32_t route_decision(
  mln_adapter_queued_resource_provider_route route,
  const mln_resource_request* request
) {
  mln_adapter_resource_request_queue queue = MLN_HANDLE_NULL;
  const mln_wake wake = {.size = sizeof(mln_wake)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_resource_request_queue_create(&wake, &queue)
  );
  mln_adapter_queued_resource_provider provider = {
    .routes = &route,
    .route_count = 1,
    .queue = queue,
  };
  const uint32_t decision = mln_adapter_queued_resource_provider_callback(
    &provider, request, unissued_handle
  );
  mln_adapter_queued_resource_request* record = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_resource_request_queue_acquire(queue, &record)
  );
  if (record != NULL) {
    queued_requests += 1;
    last_requested_url_null = record->requested_url == NULL;
    last_requested_url_empty =
      !last_requested_url_null && record->requested_url[0] == '\0';
    mln_adapter_resource_provider_request_destroy(record);
  }
  mln_adapter_resource_request_queue_close(queue);
  return decision;
}

static void assert_claims(
  mln_adapter_queued_resource_provider_route route,
  const mln_resource_request* request
) {
  const size_t before = queued_requests;
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_HANDLE, route_decision(route, request)
  );
  TEST_ASSERT_EQUAL_size_t(before + 1, queued_requests);
}

static void assert_passes_through(
  mln_adapter_queued_resource_provider_route route,
  const mln_resource_request* request
) {
  const size_t before = queued_requests;
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH, route_decision(route, request)
  );
  TEST_ASSERT_EQUAL_size_t(before, queued_requests);
}

// A route with no comparable URL matches nothing rather than claiming every
// request the way `**` does.
static void queued_provider_routes_reject_raw_invalid_route_descriptors(void) {
  const mln_resource_request request = style_request();

  assert_passes_through(
    (mln_adapter_queued_resource_provider_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE,
      .url = NULL,
    },
    &request
  );
  assert_passes_through(
    (mln_adapter_queued_resource_provider_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
      .url = NULL,
    },
    &request
  );
  assert_passes_through(
    (mln_adapter_queued_resource_provider_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB | (1U << 31U),
      .url = "**",
    },
    &request
  );
}

// A claimed record reports an absent requested URL as the empty string, so a
// listener reads it without a null check.
static void queued_provider_routes_tolerate_raw_absent_request_urls(void) {
  mln_resource_request request = style_request();
  request.requested_url = NULL;

  assert_claims(
    (mln_adapter_queued_resource_provider_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
      .url = "**",
    },
    &request
  );
  TEST_ASSERT_FALSE(last_requested_url_null);
  TEST_ASSERT_TRUE(last_requested_url_empty);
  assert_passes_through(
    (mln_adapter_queued_resource_provider_route){
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
           (mln_adapter_queued_resource_provider_route){
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
    MLN_STATUS_OK, mln_adapter_http_header_validate("X-Test", "caf\xC3\xA9")
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_http_header_validate("Bad Name", "value")
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_http_header_validate("Range", "bytes=0-1")
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_http_header_validate("Authorization", "bad\r\nvalue")
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_adapter_http_header_validate("Authorization", invalid_utf8)
  );
}

static void completion_copy_failures_use_the_adapter_failure_channel(void) {
  completion_listener_calls = 0;
  completion_listener_received_null = false;
  mln_completion completion = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_adapter_completion_create(
                     MLN_ADAPTER_COMPLETION_COPY_TEXTURE_READBACK, 0,
                     completion_listener, NULL, &completion
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

void run_callback_adapter_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(queued_provider_routes_reject_raw_invalid_route_descriptors);
  RUN_TEST(queued_provider_routes_tolerate_raw_absent_request_urls);
  RUN_TEST(glob_routes_confine_a_star_to_one_path_segment);
  RUN_TEST(glob_routes_anchor_match_wildcards_and_escapes);
  RUN_TEST(glob_routes_backtrack_across_wildcard_runs);
  RUN_TEST(http_header_routes_match_exact_glob_kind_and_order);
  RUN_TEST(http_header_validation_uses_the_native_policy);
  RUN_TEST(completion_copy_failures_use_the_adapter_failure_channel);
}
