// Routed resource providers: route descriptors that a binding's own route type
// cannot express, a request that carries no requested URL, and the glob
// language that every rule table shares.
//
// Route matching happens before the handle is read, so these cases drive the
// callback directly with a synthesized request rather than through a loader.

#include "maplibre_native_c/callback_adapter.h"
#include "support/test_support.h"

static const char alias_url[] = "maplibre://maps/style";
static const char resolved_url[] = "https://maps.localhost/style.json";

// A handle the loader never issued. Route matching never reads it.
static const mln_resource_request_handle unissued_handle = 1;

static size_t routed_requests;
static const mln_resource_request* last_routed_request;

static mln_resource_request style_request(void) {
  return (mln_resource_request){
    .size = sizeof(mln_resource_request),
    .requested_url = alias_url,
    .resolved_url = resolved_url,
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

// Reports whether one route claims one request, checking that a claimed
// request reaches the inner provider and a passed one does not.
static bool route_claims(
  mln_adapter_resource_route route, const mln_resource_request* request
) {
  const mln_adapter_routed_resource_provider provider = {
    .routes = &route,
    .route_count = 1,
    .callback = routed_provider_stub,
  };
  const size_t before = routed_requests;
  const uint32_t decision = mln_adapter_routed_resource_provider_callback(
    (void*)&provider, request, unissued_handle
  );
  if (decision == MLN_RESOURCE_PROVIDER_DECISION_HANDLE) {
    TEST_ASSERT_EQUAL_size_t(before + 1, routed_requests);
    TEST_ASSERT_EQUAL_PTR(request, last_routed_request);
    return true;
  }
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH, decision
  );
  TEST_ASSERT_EQUAL_size_t(before, routed_requests);
  return false;
}

// A route with no comparable URL matches nothing rather than claiming every
// request the way `**` does.
static void routed_provider_rejects_raw_invalid_route_descriptors(void) {
  static const struct {
    const char* label;
    mln_adapter_resource_route route;
  } routes[] = {
    {"null exact url",
     {MLN_ADAPTER_RESOURCE_KIND_ANY, MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE,
      NULL}},
    {"null glob url",
     {MLN_ADAPTER_RESOURCE_KIND_ANY, MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
      NULL}},
    {"unknown flag bit",
     {MLN_ADAPTER_RESOURCE_KIND_ANY,
      MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB | (1U << 31U), "**"}},
    {"other kind",
     {MLN_RESOURCE_KIND_TILE, MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB, "**"}},
  };
  const mln_resource_request request = style_request();
  for (size_t index = 0; index < sizeof(routes) / sizeof(routes[0]); ++index) {
    TEST_ASSERT_FALSE_MESSAGE(
      route_claims(routes[index].route, &request), routes[index].label
    );
  }

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

  TEST_ASSERT_TRUE(route_claims(
    (mln_adapter_resource_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
      .url = "**",
    },
    &request
  ));
  TEST_ASSERT_FALSE(route_claims(
    (mln_adapter_resource_route){
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB |
               MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL,
      .url = "**",
    },
    &request
  ));
}

// One glob pattern, one candidate URL, and whether the pattern claims it. A
// null url stands for resolved_url.
typedef struct glob_case {
  const char* pattern;
  const char* url;
  bool claims;
} glob_case;

static const glob_case glob_cases[] = {
  // `*` spans one path segment, so a path segment that carries the host name
  // leaves a host pattern unmatched.
  {"https://*.localhost/**", NULL, true},
  {"https://*.localhost/**", "https://tiles.localhost/1/2/3.pbf", true},
  {"https://*.localhost/**", "https://127.0.0.1/x.localhost/style.json", false},
  {"https://*/style.json", NULL, true},
  {"https://*.localhost/*", NULL, true},
  {"https://*.localhost/*/*", NULL, false},
  {"**", NULL, true},
  {"*", NULL, false},
  // A pattern matches the complete URL, so a bare suffix or infix claims
  // nothing without a leading wildcard.
  {"**.json", NULL, true},
  {".json", NULL, false},
  {"**.pbf", NULL, false},
  {"https://maps**", NULL, true},
  // A pattern with no metacharacters compares byte for byte.
  {"https://maps.localhost/style.json", NULL, true},
  {"https://maps.localhost/Style.json", NULL, false},
  {"", NULL, false},
  {"", "", true},
  // `?` spans one character other than `/`.
  {"?ttps://maps.localhost/style.json", NULL, true},
  {"https://maps.localhost/style?json", NULL, true},
  {"https:?**", NULL, false},
  // `\` makes the next character literal, and a trailing `\` matches itself.
  {"https://maps.localhost/style\\?json", NULL, false},
  {"https://star\\*.localhost/x", "https://star*.localhost/x", true},
  {"https://star\\*.localhost/x", "https://starry.localhost/x", false},
  {"custom://style\\", "custom://style\\", true},
  {"custom://style\\", "custom://style", false},
  // A failed match after a wildcard run retries from a later position.
  {"https://**/tiles/*", "https://host/tiles/a/b/tiles/x", true},
};

static void glob_routes_match_the_documented_pattern_language(void) {
  for (size_t index = 0; index < sizeof(glob_cases) / sizeof(glob_cases[0]);
       ++index) {
    const glob_case* row = &glob_cases[index];
    mln_resource_request request = style_request();
    if (row->url != NULL) {
      request.resolved_url = row->url;
    }
    const bool claims = route_claims(
      (mln_adapter_resource_route){
        .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
        .flags = MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB,
        .url = row->pattern,
      },
      &request
    );
    TEST_ASSERT_EQUAL_MESSAGE(row->claims, claims, row->pattern);
  }
}

MLN_TEST_GROUP {
  RUN_TEST(routed_provider_rejects_raw_invalid_route_descriptors);
  RUN_TEST(routed_provider_tolerates_raw_absent_request_urls);
  RUN_TEST(glob_routes_match_the_documented_pattern_language);
}
