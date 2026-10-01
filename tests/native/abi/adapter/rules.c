// Native rule tables: URL rewrite rules for a resource transform, HTTP header
// rules, and provider rules that answer a request inline.
//
// Rewrite rules run inside a real transform invocation, because only there can
// a rule set a replacement URL. Provider rules answer a real style request.
// The requests use a custom scheme, so none of them reaches the network.

#include "maplibre_native_c/callback_adapter.h"
#include "support/adapter.h"
#include "support/test_support.h"

static const uint8_t inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";
static const uint8_t malformed_style_json[] = "not a style";

// A handle the loader never issued. Rules that match nothing never read it.
static const mln_resource_request_handle unissued_handle = 1;

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

// The first rule carries a header the transport manages, so a status of
// invalid-argument shows that the rule matched.
static void http_header_routes_match_exact_glob_kind_and_order(void) {
  const mln_adapter_http_header invalid[] = {{.name = "Host", .value = "bad"}};
  const mln_adapter_http_header_transform_rule rules[] = {
    {
      .kind = MLN_RESOURCE_KIND_TILE,
      .flags = MLN_ADAPTER_URL_MATCH_FLAGS_NONE,
      .url = "https://tiles.localhost/exact",
      .headers = invalid,
      .header_count = 1,
    },
    {
      .kind = MLN_ADAPTER_RESOURCE_KIND_ANY,
      .flags = MLN_ADAPTER_URL_MATCH_GLOB,
      .url = "https://tiles.localhost/**",
      .headers = NULL,
      .header_count = 0,
    },
  };

  MLN_TEST_INVALID(header_route_status(
    rules, 2, MLN_RESOURCE_KIND_TILE, "https://tiles.localhost/exact"
  ));
  MLN_TEST_OK(header_route_status(
    rules, 2, MLN_RESOURCE_KIND_STYLE, "https://tiles.localhost/exact"
  ));
  MLN_TEST_OK(header_route_status(
    rules, 2, MLN_RESOURCE_KIND_TILE, "https://elsewhere.localhost/"
  ));
  MLN_TEST_INVALID(header_route_status(
    NULL, 1, MLN_RESOURCE_KIND_TILE, "https://tiles.localhost/"
  ));
}

static void http_header_validation_uses_the_native_policy(void) {
  static const char invalid_utf8[] = {'b', 'a', 'd', (char)0xFF, '\0'};
  static const struct {
    const char* name;
    const char* value;
    mln_status expected;
  } headers[] = {
    {"X-Test", "caf\xC3\xA9", MLN_STATUS_OK},
    {"Bad Name", "value", MLN_STATUS_INVALID_ARGUMENT},
    {"Range", "bytes=0-1", MLN_STATUS_INVALID_ARGUMENT},
    {"Authorization", "bad\r\nvalue", MLN_STATUS_INVALID_ARGUMENT},
    {"Authorization", invalid_utf8, MLN_STATUS_INVALID_ARGUMENT},
  };
  for (size_t index = 0; index < sizeof(headers) / sizeof(headers[0]);
       ++index) {
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      headers[index].expected,
      mln_adapter_http_header_validate(
        headers[index].name, headers[index].value, NULL
      ),
      headers[index].name
    );
  }
}

static const mln_adapter_resource_rewrite_rule rewrite_rules[] = {
  {MLN_RESOURCE_KIND_TILE, MLN_ADAPTER_URL_MATCH_FLAGS_NONE,
   "custom://adapter-rewrite/tile.pbf", "custom://rewritten/tile.pbf"},
  {MLN_ADAPTER_RESOURCE_KIND_ANY, MLN_ADAPTER_URL_MATCH_GLOB,
   "custom://adapter-rewrite/*.json", "custom://rewritten/style.json"},
  // Matches and leaves the URL unchanged, so no later rule applies.
  {MLN_ADAPTER_RESOURCE_KIND_ANY, MLN_ADAPTER_URL_MATCH_GLOB,
   "custom://adapter-rewrite/**", NULL},
  // Match nothing.
  {MLN_ADAPTER_RESOURCE_KIND_ANY, MLN_ADAPTER_URL_MATCH_GLOB | (1U << 31U),
   "**", "custom://unknown-flag"},
  {MLN_ADAPTER_RESOURCE_KIND_ANY, MLN_ADAPTER_URL_MATCH_FLAGS_NONE, NULL,
   "custom://null-url"},
};

static const mln_adapter_resource_rewrite_rules rewrite_table = {
  .rules = rewrite_rules,
  .count = sizeof(rewrite_rules) / sizeof(rewrite_rules[0]),
};

// One request that the rewrite rules see, and the URL they leave, or null for
// an unchanged URL.
typedef struct rewrite_case {
  uint32_t kind;
  const char* url;
  const char* replacement;
} rewrite_case;

static const rewrite_case rewrite_cases[] = {
  {MLN_RESOURCE_KIND_TILE, "custom://adapter-rewrite/tile.pbf",
   "custom://rewritten/tile.pbf"},
  {MLN_RESOURCE_KIND_SOURCE, "custom://adapter-rewrite/tile.pbf", NULL},
  {MLN_RESOURCE_KIND_STYLE, "custom://adapter-rewrite/style.json",
   "custom://rewritten/style.json"},
  {MLN_RESOURCE_KIND_STYLE, "custom://adapter-rewrite/nested/style.json", NULL},
  {MLN_RESOURCE_KIND_STYLE, "custom://elsewhere/style.json", NULL},
};

enum { rewrite_case_count = sizeof(rewrite_cases) / sizeof(rewrite_cases[0]) };

// What the transform saw and what the rules left for each case. The transform
// runs on a MapLibre thread and writes everything before it sets finished.
typedef struct rewrite_probe {
  atomic_bool claimed;
  atomic_bool finished;
  uint32_t kind;
  char url[64];
  mln_status statuses[rewrite_case_count];
  bool replaced[rewrite_case_count];
  char replacements[rewrite_case_count][64];
} rewrite_probe;

static void copy_text(char* destination, size_t capacity, const char* text) {
  const size_t length = strlen(text);
  const size_t copied = length < capacity ? length : capacity - 1;
  memcpy(destination, text, copied);
  destination[copied] = '\0';
}

// Runs every rewrite case through the rule callback inside the first transform
// invocation, then leaves the request's URL unchanged.
static mln_status rewrite_cases_transform(
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_response* response
) {
  rewrite_probe* probe = user_data;
  if (atomic_exchange(&probe->claimed, true)) {
    return MLN_STATUS_OK;
  }
  probe->kind = kind;
  copy_text(probe->url, sizeof(probe->url), url);
  for (size_t index = 0; index < rewrite_case_count; ++index) {
    (void)mln_resource_transform_response_set_url(response, "", 0, NULL);
    probe->statuses[index] = mln_adapter_resource_transform_rewrite_callback(
      (void*)&rewrite_table, rewrite_cases[index].kind,
      rewrite_cases[index].url, response
    );
    probe->replaced[index] = response->url != NULL;
    if (response->url != NULL) {
      copy_text(
        probe->replacements[index], sizeof(probe->replacements[index]),
        response->url
      );
    }
  }
  (void)mln_resource_transform_response_set_url(response, "", 0, NULL);
  mln_test_flag_set(&probe->finished);
  return MLN_STATUS_OK;
}

static mln_status set_transform_committed(
  mln_runtime runtime, const mln_resource_transform* transform
) {
  mln_test_completion completion = mln_test_completion_default(0);
  return mln_test_adapter_commit(
    mln_runtime_set_resource_transform(
      runtime, transform, &completion.descriptor, NULL
    ),
    &completion
  );
}

// The first rule whose kind and URL match decides, and a rule without a
// replacement leaves the URL unchanged.
static void rewrite_rules_replace_the_url_of_the_first_matching_rule(void) {
  static const char style_url[] = "custom://adapter-rewrite/style.json";
  static rewrite_probe probe;
  memset(&probe, 0, sizeof(probe));
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = rewrite_cases_transform,
    .user_data = &probe,
  };
  MLN_TEST_OK(set_transform_committed(runtime, &transform));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, style_url));
  TEST_ASSERT_TRUE(mln_test_wait_until(runtime, &probe.finished));

  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_STYLE, probe.kind);
  TEST_ASSERT_EQUAL_STRING(style_url, probe.url);
  for (size_t index = 0; index < rewrite_case_count; ++index) {
    const rewrite_case* row = &rewrite_cases[index];
    MLN_TEST_OK_MESSAGE(probe.statuses[index], row->url);
    TEST_ASSERT_EQUAL_MESSAGE(
      row->replacement != NULL, probe.replaced[index], row->url
    );
    if (row->replacement != NULL) {
      TEST_ASSERT_EQUAL_STRING_MESSAGE(
        row->replacement, probe.replacements[index], row->url
      );
    }
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Missing arguments leave the URL unchanged rather than failing the request.
static void rewrite_rules_accept_raw_null_arguments(void) {
  mln_resource_transform_response response = {
    .size = sizeof(mln_resource_transform_response),
  };
  MLN_TEST_OK(mln_adapter_resource_transform_rewrite_callback(
    NULL, MLN_RESOURCE_KIND_STYLE, "custom://adapter-rewrite/style.json",
    &response
  ));
  MLN_TEST_OK(mln_adapter_resource_transform_rewrite_callback(
    (void*)&rewrite_table, MLN_RESOURCE_KIND_STYLE, NULL, &response
  ));
  MLN_TEST_OK(mln_adapter_resource_transform_rewrite_callback(
    (void*)&rewrite_table, MLN_RESOURCE_KIND_STYLE,
    "custom://adapter-rewrite/style.json", NULL
  ));
  TEST_ASSERT_NULL(response.url);
}

static mln_resource_response style_response(const uint8_t* bytes, size_t size) {
  return (mln_resource_response){
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .bytes = bytes,
    .byte_count = size,
  };
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

// A matching rule answers the request inline with its response, and the first
// rule whose kind and URL match decides. A request no rule matches passes
// through to native loading, which declines the custom scheme.
static void provider_rules_answer_the_first_matching_request_inline(void) {
  const mln_adapter_resource_provider_rule rules[] = {
    {MLN_RESOURCE_KIND_TILE, MLN_ADAPTER_URL_MATCH_GLOB,
     "custom://adapter-rules/**",
     style_response(malformed_style_json, sizeof(malformed_style_json) - 1)},
    {MLN_RESOURCE_KIND_STYLE, MLN_ADAPTER_URL_MATCH_FLAGS_NONE, NULL,
     style_response(malformed_style_json, sizeof(malformed_style_json) - 1)},
    {MLN_RESOURCE_KIND_STYLE, MLN_ADAPTER_URL_MATCH_GLOB | (1U << 31U), "**",
     style_response(malformed_style_json, sizeof(malformed_style_json) - 1)},
    {MLN_RESOURCE_KIND_STYLE, MLN_ADAPTER_URL_MATCH_GLOB,
     "custom://adapter-rules/*.json",
     style_response(inline_style_json, sizeof(inline_style_json) - 1)},
  };
  const mln_adapter_resource_provider_rules table = {
    .rules = rules,
    .count = sizeof(rules) / sizeof(rules[0]),
  };
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = mln_adapter_resource_provider_rules_callback,
    .user_data = (void*)&table,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_adapter_set_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);

  MLN_TEST_OK(
    mln_test_map_set_style_url(map, "custom://adapter-rules/style.json")
  );
  TEST_ASSERT_TRUE(wait_for_map_event(
    runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED, map, NULL, 0
  ));

  MLN_TEST_OK(
    mln_test_map_set_style_url(map, "custom://adapter-rules/nested/style.json")
  );
  char message[512] = {0};
  TEST_ASSERT_TRUE(wait_for_map_event(
    runtime, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, map, message, sizeof(message)
  ));
  TEST_ASSERT_NOT_NULL(strstr(message, "declined"));

  MLN_TEST_OK(mln_test_adapter_clear_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A request the rules cannot compare passes through without touching its
// handle.
static void provider_rules_pass_raw_incomplete_requests_through(void) {
  const mln_adapter_resource_provider_rule rule = {
    MLN_ADAPTER_RESOURCE_KIND_ANY, MLN_ADAPTER_URL_MATCH_GLOB, "**",
    style_response(inline_style_json, sizeof(inline_style_json) - 1)
  };
  const mln_adapter_resource_provider_rules table = {
    .rules = &rule, .count = 1
  };
  mln_resource_request request = {
    .size = sizeof(mln_resource_request),
    .requested_url = NULL,
    .resolved_url = "custom://adapter-rules/style.json",
    .kind = MLN_RESOURCE_KIND_STYLE,
  };
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    mln_adapter_resource_provider_rules_callback(
      (void*)&table, &request, unissued_handle
    )
  );
  request.requested_url = "custom://adapter-rules/style.json";
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    mln_adapter_resource_provider_rules_callback(
      (void*)&table, &request, MLN_HANDLE_NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    mln_adapter_resource_provider_rules_callback(
      NULL, &request, unissued_handle
    )
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH,
    mln_adapter_resource_provider_rules_callback(
      (void*)&table, NULL, unissued_handle
    )
  );
}

MLN_TEST_GROUP {
  RUN_TEST(http_header_routes_match_exact_glob_kind_and_order);
  RUN_TEST(http_header_validation_uses_the_native_policy);
  RUN_TEST(rewrite_rules_replace_the_url_of_the_first_matching_rule);
  RUN_TEST(rewrite_rules_accept_raw_null_arguments);
  RUN_TEST(provider_rules_answer_the_first_matching_request_inline);
  RUN_TEST(provider_rules_pass_raw_incomplete_requests_through);
}
