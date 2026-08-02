// Raw C ABI coverage for queued provider route matching: route descriptors a
// binding's own route type cannot express, and a request that carries no
// requested URL. Which URL each flag combination compares is semantic, so it
// belongs to the binding suites that register these routes.
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

// A handle the loader never issued. A route that matches hands the copied
// record to the listener below, which releases the record without completing
// or releasing the handle it carries.
static const mln_resource_request_handle unissued_handle = 1;

static size_t queued_requests;
// The record's strings die with the record, so the listener records what it
// saw rather than pointers into it.
static bool last_requested_url_null;
static bool last_requested_url_empty;

static void count_queued_request(void* request) {
  const mln_adapter_queued_resource_request* record = request;
  queued_requests += 1;
  last_requested_url_null = record->requested_url == NULL;
  last_requested_url_empty =
    !last_requested_url_null && record->requested_url[0] == '\0';
  mln_adapter_resource_provider_request_destroy(request);
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

// Reports the decision one route produces for one request, and leaves
// queued_requests incremented when the route claimed it.
static uint32_t route_decision(
  mln_adapter_queued_resource_provider_route route,
  const mln_resource_request* request
) {
  mln_adapter_queued_resource_provider provider = {
    .routes = &route,
    .route_count = 1,
    .listener = count_queued_request,
  };
  return mln_adapter_queued_resource_provider_callback(
    &provider, request, unissued_handle
  );
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

// This verifies the two route descriptors a binding route type cannot express:
// a null comparison URL and a flag bit this C API version does not define.
// Neither names a URL family the adapter can compare, so each matches nothing
// rather than claiming every request the way an empty prefix does.
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
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_PREFIX,
      .url = NULL,
    },
    &request
  );
  assert_passes_through(
    (mln_adapter_queued_resource_provider_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_PREFIX | (1U << 31U),
      .url = "",
    },
    &request
  );
}

// This verifies a request whose requested URL is absent, which the loader does
// not produce and a binding cannot synthesize. An empty prefix over the URL
// that is present still claims the request, while the same prefix over the
// absent URL matches nothing. A claimed record reports the absent URL as the
// empty string, so a listener reads it without a null check.
static void queued_provider_routes_tolerate_raw_absent_request_urls(void) {
  mln_resource_request request = style_request();
  request.requested_url = NULL;

  assert_claims(
    (mln_adapter_queued_resource_provider_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_PREFIX,
      .url = "",
    },
    &request
  );
  TEST_ASSERT_FALSE(last_requested_url_null);
  TEST_ASSERT_TRUE(last_requested_url_empty);
  assert_passes_through(
    (mln_adapter_queued_resource_provider_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_PREFIX |
               MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL,
      .url = "",
    },
    &request
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

static void http_header_routes_match_exact_prefix_kind_and_order(void) {
  const mln_adapter_http_header invalid[] = {{.name = "Host", .value = "bad"}};
  const mln_adapter_http_header_transform_rule rules[] = {
    {
      .kind = MLN_RESOURCE_KIND_TILE,
      .flags = MLN_ADAPTER_HTTP_HEADER_ROUTE_FLAGS_NONE,
      .url = "https://tiles.test/exact",
      .headers = invalid,
      .header_count = 1,
    },
    {
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_HTTP_HEADER_ROUTE_MATCH_PREFIX,
      .url = "https://tiles.test/",
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

void run_callback_adapter_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(queued_provider_routes_reject_raw_invalid_route_descriptors);
  RUN_TEST(queued_provider_routes_tolerate_raw_absent_request_urls);
  RUN_TEST(http_header_routes_match_exact_prefix_kind_and_order);
  RUN_TEST(http_header_validation_uses_the_native_policy);
}
