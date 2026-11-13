// The resource transform and the HTTP header transform.

#include "support/resources.h"
#include "support/test_support.h"

#if MLN_TEST_PLATFORM_HTTP
#include "support/http_server.h"
#endif

// The browser and OpenHarmony transports cannot drop a transformed header when
// a redirect changes origin, so they refuse the header transform.
#if defined(__OHOS__)
#define MLN_TEST_HEADER_TRANSFORM_SUPPORTED 0
#else
#define MLN_TEST_HEADER_TRANSFORM_SUPPORTED 1
#endif

static const char empty_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static void keep_url(
  void* user_data, uint32_t kind, mln_buffer_view url,
  mln_resource_transform_request request
) {
  (void)user_data;
  (void)kind;
  (void)url;
  mln_resource_transform_request_release(request);
}

static mln_status add_no_headers(
  void* user_data, uint32_t kind, mln_buffer_view url,
  mln_http_header_transform_response* out_response
) {
  (void)user_data;
  (void)kind;
  (void)url;
  (void)out_response;
  return MLN_STATUS_OK;
}

static mln_status submit_resource_transform(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_resource_transform(
    *(const mln_runtime*)context, descriptor, &completion.descriptor, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    (void)mln_test_completion_finish(&completion);
  } else {
    mln_test_completion_reject(&completion);
  }
  mln_test_completion_destroy(&completion);
  return status;
}

static void resource_transform_with_zero_size(void* descriptor) {
  ((mln_resource_transform*)descriptor)->size = 0;
}

static void resource_transform_without_callback_or_rules(void* descriptor) {
  ((mln_resource_transform*)descriptor)->callback = NULL;
}

static void resource_transform_with_callback_and_rules(void* descriptor) {
  static mln_resource_transform_rule rule;
  rule =
    (mln_resource_transform_rule){.size = sizeof(mln_resource_transform_rule)};
  ((mln_resource_transform*)descriptor)->rules = &rule;
  ((mln_resource_transform*)descriptor)->rule_count = 1;
}

static const mln_test_validation_case resource_transform_cases[] = {
  {"a well-formed transform", NULL, MLN_STATUS_OK, NULL},
  {"a zero size", resource_transform_with_zero_size,
   MLN_STATUS_INVALID_ARGUMENT, "size"},
  {"neither a callback nor rules", resource_transform_without_callback_or_rules,
   MLN_STATUS_INVALID_ARGUMENT, "rules"},
  {"a callback with rules", resource_transform_with_callback_and_rules,
   MLN_STATUS_INVALID_ARGUMENT, "rules"},
};

static void resource_transform_registration_validates_its_descriptor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = keep_url,
  };
  mln_test_run_validation_table(
    resource_transform_cases,
    sizeof(resource_transform_cases) / sizeof(resource_transform_cases[0]),
    &transform, sizeof(transform), submit_resource_transform, &runtime
  );
  MLN_TEST_INVALID(mln_test_set_resource_transform(runtime, NULL));
  MLN_TEST_INVALID(mln_test_clear_resource_transform(MLN_HANDLE_NULL));
  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  mln_test_destroy_runtime(runtime);
}

// The rule and parameter that a rules validation case edits. Each submission
// restores them for the next case.
static mln_url_query_parameter parameter_under_test;
static mln_resource_transform_rule rule_under_test;
static const char invalid_utf8[] = {'b', 'a', 'd', (char)0xFF};

static void reset_rule_under_test(void) {
  parameter_under_test = (mln_url_query_parameter){
    .name = MLN_BUFFER_LITERAL("key"),
    .value = MLN_BUFFER_LITERAL("value"),
  };
  rule_under_test = (mln_resource_transform_rule){
    .size = sizeof(mln_resource_transform_rule),
    .flags = MLN_URL_MATCH_GLOB,
    .url = MLN_BUFFER_LITERAL("http://127.0.0.1/**"),
    .parameters = &parameter_under_test,
    .parameter_count = 1,
  };
}

static mln_status submit_rules_transform(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_status status =
    submit_resource_transform(context, descriptor, diagnostic);
  reset_rule_under_test();
  return status;
}

static void rules_with_user_data(void* descriptor) {
  static int context;
  ((mln_resource_transform*)descriptor)->user_data = &context;
}

static void rules_with_a_queue(void* descriptor) {
  ((mln_resource_transform*)descriptor)->queue = (mln_delivery_queue)1;
}

static void rules_that_are_null(void* descriptor) {
  ((mln_resource_transform*)descriptor)->rules = NULL;
}

static void rule_with_zero_size(void* descriptor) {
  (void)descriptor;
  rule_under_test.size = 0;
}

static void rule_with_unknown_kinds(void* descriptor) {
  (void)descriptor;
  rule_under_test.kinds = 1U << 31U;
}

static void rule_with_null_parameters(void* descriptor) {
  (void)descriptor;
  rule_under_test.parameters = NULL;
}

static void parameter_with_an_empty_name(void* descriptor) {
  (void)descriptor;
  parameter_under_test.name = (mln_buffer_view){0};
}

static void parameter_name_with_nul(void* descriptor) {
  (void)descriptor;
  parameter_under_test.name = (mln_buffer_view){.data = "a\0b", .size = 3};
}

static void parameter_value_that_is_not_utf8(void* descriptor) {
  (void)descriptor;
  parameter_under_test.value =
    (mln_buffer_view){.data = invalid_utf8, .size = sizeof(invalid_utf8)};
}

static const mln_test_validation_case rules_transform_cases[] = {
  {"well-formed rules", NULL, MLN_STATUS_OK, NULL},
  {"rules with user_data", rules_with_user_data, MLN_STATUS_INVALID_ARGUMENT,
   "user_data"},
  {"rules with a queue", rules_with_a_queue, MLN_STATUS_INVALID_ARGUMENT,
   "queue"},
  {"null rules with a count", rules_that_are_null, MLN_STATUS_INVALID_ARGUMENT,
   "rules"},
  {"a rule with a zero size", rule_with_zero_size, MLN_STATUS_INVALID_ARGUMENT,
   "size"},
  {"a rule with unknown kinds", rule_with_unknown_kinds,
   MLN_STATUS_INVALID_ARGUMENT, "kind"},
  {"null parameters with a count", rule_with_null_parameters,
   MLN_STATUS_INVALID_ARGUMENT, "parameters"},
  {"a parameter with an empty name", parameter_with_an_empty_name,
   MLN_STATUS_INVALID_ARGUMENT, "name"},
  {"a parameter name with NUL", parameter_name_with_nul,
   MLN_STATUS_INVALID_ARGUMENT, "name"},
  {"a parameter value that is not UTF-8", parameter_value_that_is_not_utf8,
   MLN_STATUS_INVALID_ARGUMENT, "value"},
};

static void a_rules_transform_validates_its_rules(void) {
  mln_runtime runtime = mln_test_create_runtime();
  reset_rule_under_test();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .rules = &rule_under_test,
    .rule_count = 1,
  };
  mln_test_run_validation_table(
    rules_transform_cases,
    sizeof(rules_transform_cases) / sizeof(rules_transform_cases[0]),
    &transform, sizeof(transform), submit_rules_transform, &runtime
  );
  mln_test_destroy_runtime(runtime);
}

static mln_status submit_header_transform(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_http_header_transform(
    *(const mln_runtime*)context, descriptor, &completion.descriptor, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    (void)mln_test_completion_finish(&completion);
  } else {
    mln_test_completion_reject(&completion);
  }
  mln_test_completion_destroy(&completion);
  return status;
}

static void header_transform_with_zero_size(void* descriptor) {
  ((mln_http_header_transform*)descriptor)->size = 0;
}

#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
static void header_transform_with_rules(void* descriptor) {
  static mln_http_header_rule rule;
  rule = (mln_http_header_rule){.size = sizeof(mln_http_header_rule)};
  rule.url = (mln_buffer_view)MLN_BUFFER_LITERAL("http://127.0.0.1/unused");
  ((mln_http_header_transform*)descriptor)->rules = &rule;
  ((mln_http_header_transform*)descriptor)->rule_count = 1;
}
#endif

// A transport that cannot drop a transformed header when a redirect changes
// origin reports MLN_STATUS_UNSUPPORTED rather than leak the credential to the
// redirect's destination.
static const mln_test_validation_case header_transform_cases[] = {
#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  {"a well-formed transform", NULL, MLN_STATUS_OK, NULL},
#else
  {"a well-formed transform", NULL, MLN_STATUS_UNSUPPORTED, NULL},
#endif
  {"a zero size", header_transform_with_zero_size, MLN_STATUS_INVALID_ARGUMENT,
   "size"},
#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  {"a callback with rules", header_transform_with_rules,
   MLN_STATUS_INVALID_ARGUMENT, "rules"},
#endif
};

static void header_transform_registration_validates_its_descriptor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_http_header_transform transform = {
    .size = sizeof(mln_http_header_transform),
    .callback = add_no_headers,
  };
  mln_test_run_validation_table(
    header_transform_cases,
    sizeof(header_transform_cases) / sizeof(header_transform_cases[0]),
    &transform, sizeof(transform), submit_header_transform, &runtime
  );
  MLN_TEST_INVALID(mln_test_set_http_header_transform(runtime, NULL));
  MLN_TEST_INVALID(mln_test_clear_http_header_transform(MLN_HANDLE_NULL));
  mln_test_destroy_runtime(runtime);
}

typedef struct header_case {
  const char* label;
  const char* name;
  size_t name_size;
  const char* value;
  size_t value_size;
} header_case;

// Headers the helper refuses: malformed names, values that could split the
// request, values that are not UTF-8, and headers MapLibre or the transport
// manage themselves.
static const header_case refused_headers[] = {
  {"a name with a space", "Bad Name", 8, "value", 5},
  {"a value with a line break", "Authorization", 13, "bad\r\nvalue", 10},
  {"a managed header", "Range", 5, "bytes=0-1", 9},
  {"a value that is not UTF-8", "Authorization", 13, invalid_utf8,
   sizeof(invalid_utf8)},
  {"a null name", NULL, 1, "value", 5},
  {"a value with NUL", "X-Name", 6, "before\0after", 12},
};

static void a_header_needs_a_transform_invocation_and_a_valid_field(void) {
  mln_http_header_transform_response response = {
    .size = sizeof(mln_http_header_transform_response),
  };
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_http_header_transform_response_set_header(
      &response, (mln_buffer_view){.data = "X-Test", .size = 6},
      (mln_buffer_view){.data = "value", .size = 5}, NULL
    )
  );
  for (size_t index = 0;
       index < sizeof(refused_headers) / sizeof(refused_headers[0]);
       index += 1) {
    const header_case* row = &refused_headers[index];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_http_header_transform_response_set_header(
        &response, (mln_buffer_view){.data = row->name, .size = row->name_size},
        (mln_buffer_view){.data = row->value, .size = row->value_size}, NULL
      ),
      row->label
    );
  }
}

#if MLN_TEST_PLATFORM_HTTP

// Rewrites URLs whose path starts /a/ to /b/, and counts its calls.
typedef struct rewriting_transform {
  atomic_int calls;
} rewriting_transform;

static void rewrite_a_to_b(
  void* user_data, uint32_t kind, mln_buffer_view url_view,
  mln_resource_transform_request request
) {
  const char* url = url_view.data;
  (void)kind;
  rewriting_transform* transform = user_data;
  atomic_fetch_add(&transform->calls, 1);
  const char* path = strstr(url, "/a/");
  char rewritten[512];
  const int written = path == NULL ? 0
                                   : snprintf(
                                       rewritten, sizeof(rewritten), "%.*sb/%s",
                                       (int)(path - url + 1), url, path + 3
                                     );
  if (written > 0 && (size_t)written < sizeof(rewritten)) {
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_resource_transform_request_complete(
        request, (mln_buffer_view){.data = rewritten, .size = (size_t)written},
        NULL
      )
    );
    return;
  }
  mln_resource_transform_request_release(request);
}

static void load_from_server(
  mln_runtime runtime, mln_test_http_server* server, const char* path,
  bool expect_loaded
) {
  char url[256];
  mln_test_http_server_url(server, path, url, sizeof(url));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  if (expect_loaded) {
    TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), path);
  } else {
    char message[512];
    TEST_ASSERT_TRUE_MESSAGE(
      mln_test_await_loading_failure(runtime, map, message, sizeof(message)),
      path
    );
  }
  mln_test_destroy_map(map);
}

static void a_resource_transform_rewrites_until_cleared(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/b/first.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/a/second.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 2);
  rewriting_transform probe;
  atomic_init(&probe.calls, 0);
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = rewrite_a_to_b,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));

  load_from_server(runtime, server, "/a/first.json", true);
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/b/first.json")
  );
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_http_server_requests(server, "/a/first.json")
  );
  const int calls = atomic_load(&probe.calls);
  TEST_ASSERT_GREATER_OR_EQUAL_INT(1, calls);

  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  load_from_server(runtime, server, "/a/second.json", true);
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/a/second.json")
  );
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_http_server_requests(server, "/b/second.json")
  );
  TEST_ASSERT_EQUAL_INT(calls, atomic_load(&probe.calls));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

static void expect_request_target(
  mln_test_http_server* server, const char* path, int index,
  const char* expected
) {
  char target[512];
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_http_server_request_target(
      server, path, index, target, sizeof(target)
    ),
    path
  );
  TEST_ASSERT_EQUAL_STRING(expected, target);
}

static void a_rules_transform_adds_query_parameters(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/exact.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/glob/style.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/other.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 3);
  char exact[256];
  char glob[256];
  char glob_tree[256];
  mln_test_http_server_url(server, "/exact.json", exact, sizeof(exact));
  mln_test_http_server_url(server, "/glob/*", glob, sizeof(glob));
  mln_test_http_server_url(server, "/glob/**", glob_tree, sizeof(glob_tree));
  const mln_url_query_parameter tile_parameter = {
    .name = MLN_BUFFER_LITERAL("tile"),
    .value = MLN_BUFFER_LITERAL("yes"),
  };
  const mln_url_query_parameter exact_parameter = {
    .name = MLN_BUFFER_LITERAL("exact"),
    .value = MLN_BUFFER_LITERAL("yes"),
  };
  const mln_url_query_parameter glob_parameters[] = {
    {.name = MLN_BUFFER_LITERAL("api key"),
     .value = MLN_BUFFER_LITERAL("a/b \xC3\xA9&c=~-._+%41")},
    {.name = MLN_BUFFER_LITERAL("empty"), .value = {0}},
  };
  const mln_url_query_parameter shadowed_parameter = {
    .name = MLN_BUFFER_LITERAL("shadowed"),
    .value = MLN_BUFFER_LITERAL("yes"),
  };
  const mln_resource_transform_rule rules[] = {
    {.size = sizeof(mln_resource_transform_rule),
     .kinds = MLN_RESOURCE_KIND_FLAG_TILE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = MLN_BUFFER_LITERAL("**"),
     .parameters = &tile_parameter,
     .parameter_count = 1},
    {.size = sizeof(mln_resource_transform_rule),
     .url = mln_buffer_view_from_cstr(exact),
     .parameters = &exact_parameter,
     .parameter_count = 1},
    {.size = sizeof(mln_resource_transform_rule),
     .flags = MLN_URL_MATCH_GLOB,
     .url = mln_buffer_view_from_cstr(glob),
     .parameters = glob_parameters,
     .parameter_count = 2},
    {.size = sizeof(mln_resource_transform_rule),
     .flags = MLN_URL_MATCH_GLOB,
     .url = mln_buffer_view_from_cstr(glob_tree),
     .parameters = &shadowed_parameter,
     .parameter_count = 1},
  };
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .rules = rules,
    .rule_count = sizeof(rules) / sizeof(rules[0]),
  };
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));

  // An exact pattern, then the first of two globs, which adds to the query
  // that the URL has. A style request skips the tile rule, and a URL that no
  // rule matches loads unchanged.
  load_from_server(runtime, server, "/exact.json", true);
  expect_request_target(server, "/exact.json", 0, "/exact.json?exact=yes");
  load_from_server(runtime, server, "/glob/style.json?v=1", true);
  expect_request_target(
    server, "/glob/style.json", 0,
    "/glob/style.json?v=1&api%20key=a%2Fb%20%C3%A9%26c%3D~-._%2B%2541&empty="
  );
  load_from_server(runtime, server, "/other.json", true);
  expect_request_target(server, "/other.json", 0, "/other.json");

  // A rejected transform leaves the rules in place. Each load uses a new URL,
  // which the cache can't answer.
  const mln_resource_transform_rule invalid_rule = {
    .size = sizeof(mln_resource_transform_rule),
    .parameters = &(const mln_url_query_parameter){.name = {0}},
    .parameter_count = 1,
  };
  const mln_resource_transform invalid = {
    .size = sizeof(mln_resource_transform),
    .rules = &invalid_rule,
    .rule_count = 1,
  };
  MLN_TEST_INVALID(mln_test_set_resource_transform(runtime, &invalid));
  load_from_server(runtime, server, "/glob/style.json?v=2", true);
  expect_request_target(
    server, "/glob/style.json", 1,
    "/glob/style.json?v=2&api%20key=a%2Fb%20%C3%A9%26c%3D~-._%2B%2541&empty="
  );

  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  load_from_server(runtime, server, "/glob/style.json?v=3", true);
  expect_request_target(server, "/glob/style.json", 2, "/glob/style.json?v=3");
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// A rate-limited request retries with the URL that the rules rewrote, so the
// retry carries each parameter once.
static void a_retry_keeps_the_parameters_of_its_first_request(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/limited.json", .status = 429, .headers = "Retry-After: 0\r\n"},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  const mln_url_query_parameter parameter = {
    .name = MLN_BUFFER_LITERAL("key"),
    .value = MLN_BUFFER_LITERAL("abc"),
  };
  const mln_resource_transform_rule rule = {
    .size = sizeof(mln_resource_transform_rule),
    .flags = MLN_URL_MATCH_GLOB,
    .url = MLN_BUFFER_LITERAL("**"),
    .parameters = &parameter,
    .parameter_count = 1,
  };
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .rules = &rule,
    .rule_count = 1,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));
  char url[256];
  mln_test_http_server_url(server, "/limited.json", url, sizeof(url));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE(
    mln_test_http_server_wait_for_requests(server, "/limited.json", 2)
  );
  expect_request_target(server, "/limited.json", 0, "/limited.json?key=abc");
  expect_request_target(server, "/limited.json", 1, "/limited.json?key=abc");
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// What the provider saw of the style request before it passed it through.
static char provided_url[256];
static atomic_bool provided;

static void record_and_pass_through(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)user_data;
  if (!atomic_load(&provided)) {
    const size_t size = request->requested_url.size < sizeof(provided_url)
                          ? request->requested_url.size
                          : sizeof(provided_url) - 1;
    memcpy(provided_url, request->requested_url.data, size);
    provided_url[size] = '\0';
    mln_test_flag_set(&provided);
  }
  mln_test_request_pass_through(handle);
}

// A provider sees the URL before the rules, and the request it passes through
// loads with them.
static void a_passed_through_request_reaches_the_rules(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/provided.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  const mln_url_query_parameter parameter = {
    .name = MLN_BUFFER_LITERAL("key"),
    .value = MLN_BUFFER_LITERAL("abc"),
  };
  const mln_resource_transform_rule rule = {
    .size = sizeof(mln_resource_transform_rule),
    .flags = MLN_URL_MATCH_GLOB,
    .url = MLN_BUFFER_LITERAL("**"),
    .parameters = &parameter,
    .parameter_count = 1,
  };
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .rules = &rule,
    .rule_count = 1,
  };
  atomic_store(&provided, false);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = record_and_pass_through,
  };
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));

  char url[256];
  mln_test_http_server_url(server, "/provided.json", url, sizeof(url));
  load_from_server(runtime, server, "/provided.json", true);
  TEST_ASSERT_TRUE(atomic_load(&provided));
  TEST_ASSERT_EQUAL_STRING(url, provided_url);
  expect_request_target(server, "/provided.json", 0, "/provided.json?key=abc");
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// Records the kind each transform reports for the image source's URL, and
// whether the URL arrived NUL-terminated, as native lends text.
typedef struct image_kind_probe {
  atomic_uint resource_kind;
  atomic_bool resource_seen;
  atomic_bool resource_url_terminated;
  atomic_uint header_kind;
  atomic_bool header_seen;
  atomic_bool header_url_terminated;
} image_kind_probe;

static const char overlay_path[] = "/overlay.png";

static void record_image_resource_kind(
  void* user_data, uint32_t kind, mln_buffer_view url_view,
  mln_resource_transform_request request
) {
  const char* url = url_view.data;
  image_kind_probe* probe = user_data;
  if (strstr(url, overlay_path) != NULL) {
    atomic_store(&probe->resource_kind, kind);
    atomic_store(
      &probe->resource_url_terminated,
      mln_test_lent_text_is_terminated(url_view)
    );
    mln_test_flag_set(&probe->resource_seen);
  }
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_resource_transform_request_complete(request, url_view, NULL)
  );
}

#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
static mln_status record_image_header_kind(
  void* user_data, uint32_t kind, mln_buffer_view url_view,
  mln_http_header_transform_response* out_response
) {
  const char* url = url_view.data;
  (void)out_response;
  image_kind_probe* probe = user_data;
  if (strstr(url, overlay_path) != NULL) {
    atomic_store(&probe->header_kind, kind);
    atomic_store(
      &probe->header_url_terminated, mln_test_lent_text_is_terminated(url_view)
    );
    mln_test_flag_set(&probe->header_seen);
  }
  return MLN_STATUS_OK;
}
#endif

static void an_image_source_request_reaches_the_transforms_as_an_image(void) {
  mln_test_http_server* server = mln_test_http_server_start(NULL, 0);
  image_kind_probe probe;
  atomic_init(&probe.resource_kind, MLN_RESOURCE_KIND_UNKNOWN);
  atomic_init(&probe.resource_seen, false);
  atomic_init(&probe.resource_url_terminated, false);
  atomic_init(&probe.header_kind, MLN_RESOURCE_KIND_UNKNOWN);
  atomic_init(&probe.header_seen, false);
  atomic_init(&probe.header_url_terminated, false);
  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = record_image_resource_kind,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));
#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  const mln_http_header_transform header_transform = {
    .size = sizeof(mln_http_header_transform),
    .callback = record_image_header_kind,
    .user_data = &probe,
  };
  MLN_TEST_OK(mln_test_set_http_header_transform(runtime, &header_transform));
#endif

  char overlay_url[256];
  mln_test_http_server_url(
    server, overlay_path, overlay_url, sizeof(overlay_url)
  );
  char style[512];
  const int written = snprintf(
    style, sizeof(style),
    "{\"version\":8,\"sources\":{\"overlay\":{\"type\":\"image\",\"url\":"
    "\"%s\",\"coordinates\":[[-1,1],[1,1],[1,-1],[-1,-1]]}},\"layers\":[]}",
    overlay_url
  );
  TEST_ASSERT_TRUE(written > 0 && (size_t)written < sizeof(style));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_json(
    map, (mln_buffer_view){.data = style, .size = (size_t)written}
  ));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_wait_for_flag(&probe.resource_seen),
    "the resource transform never saw the image request"
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_KIND_IMAGE, atomic_load(&probe.resource_kind)
  );
  TEST_ASSERT_TRUE(atomic_load(&probe.resource_url_terminated));
#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_wait_for_flag(&probe.header_seen),
    "the header transform never saw the image request"
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_KIND_IMAGE, atomic_load(&probe.header_kind)
  );
  TEST_ASSERT_TRUE(atomic_load(&probe.header_url_terminated));
#endif
  TEST_ASSERT_TRUE(
    mln_test_http_server_wait_for_requests(server, overlay_path, 1)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

#endif

#if MLN_TEST_PLATFORM_HTTP && MLN_TEST_HEADER_TRANSFORM_SUPPORTED

static const char token_header[] = "X-Test-Token";
static const char token_value[] = "secret-token";

// Adds the token header to every request, and records how many it saw and the
// last one's kind.
typedef struct token_transform {
  atomic_int calls;
  atomic_uint last_kind;
} token_transform;

static mln_status add_token(
  void* user_data, uint32_t kind, mln_buffer_view url,
  mln_http_header_transform_response* out_response
) {
  (void)url;
  token_transform* transform = user_data;
  atomic_fetch_add(&transform->calls, 1);
  atomic_store(&transform->last_kind, kind);
  return mln_http_header_transform_response_set_header(
    out_response,
    (mln_buffer_view){.data = token_header, .size = sizeof(token_header) - 1},
    (mln_buffer_view){.data = token_value, .size = sizeof(token_value) - 1},
    NULL
  );
}

static void install_token_transform(
  mln_runtime runtime, token_transform* probe
) {
  atomic_init(&probe->calls, 0);
  atomic_init(&probe->last_kind, 0);
  const mln_http_header_transform transform = {
    .size = sizeof(mln_http_header_transform),
    .callback = add_token,
    .user_data = probe,
  };
  MLN_TEST_OK(mln_test_set_http_header_transform(runtime, &transform));
}

static bool request_carried_token(
  mln_test_http_server* server, const char* path
) {
  char value[128];
  return mln_test_http_server_request_header(
           server, path, 0, token_header, value, sizeof(value)
         ) &&
         strcmp(value, token_value) == 0;
}

static void a_header_transform_adds_headers_until_cleared(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/with-token.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/after-clear.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 2);
  mln_runtime runtime = mln_test_create_runtime();
  token_transform probe;
  install_token_transform(runtime, &probe);

  load_from_server(runtime, server, "/with-token.json", true);
  TEST_ASSERT_TRUE(request_carried_token(server, "/with-token.json"));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RESOURCE_KIND_STYLE, atomic_load(&probe.last_kind)
  );
  const int calls = atomic_load(&probe.calls);

  MLN_TEST_OK(mln_test_clear_http_header_transform(runtime));
  load_from_server(runtime, server, "/after-clear.json", true);
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/after-clear.json")
  );
  TEST_ASSERT_FALSE(request_carried_token(server, "/after-clear.json"));
  TEST_ASSERT_EQUAL_INT(calls, atomic_load(&probe.calls));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// A file:// style never reaches the HTTP client, so the transform is not
// called for it.
static void a_header_transform_skips_requests_that_are_not_http(void) {
  char path[1024];
  mln_test_temp_path("header-transform-style.json", path, sizeof(path));
  FILE* file = fopen(path, "wb");
  TEST_ASSERT_NOT_NULL(file);
  TEST_ASSERT_EQUAL_size_t(
    sizeof(empty_style_json) - 1,
    fwrite(empty_style_json, 1, sizeof(empty_style_json) - 1, file)
  );
  TEST_ASSERT_EQUAL_INT(0, fclose(file));
  char url[1100];
  (void)snprintf(
    url, sizeof(url), "file://%s%s", path[0] == '/' ? "" : "/", path
  );
  for (char* cursor = url; *cursor != '\0'; cursor += 1) {
    if (*cursor == '\\') *cursor = '/';
  }

  mln_runtime runtime = mln_test_create_runtime();
  token_transform probe;
  install_token_transform(runtime, &probe);
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), url);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.calls));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  (void)remove(path);
}

// Header rules match the URL that the resource transform's rules wrote.
static void header_rules_see_the_rewritten_url(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/keyed.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char rewritten[256];
  mln_test_http_server_url(
    server, "/keyed.json?key=abc", rewritten, sizeof(rewritten)
  );
  const mln_url_query_parameter parameter = {
    .name = MLN_BUFFER_LITERAL("key"),
    .value = MLN_BUFFER_LITERAL("abc"),
  };
  const mln_resource_transform_rule rule = {
    .size = sizeof(mln_resource_transform_rule),
    .flags = MLN_URL_MATCH_GLOB,
    .url = MLN_BUFFER_LITERAL("**"),
    .parameters = &parameter,
    .parameter_count = 1,
  };
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .rules = &rule,
    .rule_count = 1,
  };
  const mln_http_header header = {
    .name = {.data = token_header, .size = sizeof(token_header) - 1},
    .value = {.data = token_value, .size = sizeof(token_value) - 1},
  };
  const mln_http_header_rule header_rule = {
    .size = sizeof(mln_http_header_rule),
    .url = mln_buffer_view_from_cstr(rewritten),
    .headers = &header,
    .header_count = 1,
  };
  const mln_http_header_transform header_transform = {
    .size = sizeof(mln_http_header_transform),
    .rules = &header_rule,
    .rule_count = 1,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));
  MLN_TEST_OK(mln_test_set_http_header_transform(runtime, &header_transform));

  load_from_server(runtime, server, "/keyed.json", true);
  TEST_ASSERT_TRUE(request_carried_token(server, "/keyed.json"));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

static void a_redirect_keeps_headers_only_within_the_origin(void) {
  static const mln_test_http_route other_routes[] = {
    {.path = "/elsewhere.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* other = mln_test_http_server_start(other_routes, 1);
  static char cross_origin_location[256];
  char elsewhere[200];
  mln_test_http_server_url(
    other, "/elsewhere.json", elsewhere, sizeof(elsewhere)
  );
  (void)snprintf(
    cross_origin_location, sizeof(cross_origin_location), "Location: %s\r\n",
    elsewhere
  );
  const mln_test_http_route routes[] = {
    {.path = "/same-origin.json",
     .status = 302,
     .headers = "Location: /redirected.json\r\n"},
    {.path = "/redirected.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/cross-origin.json",
     .status = 302,
     .headers = cross_origin_location},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 3);
  mln_runtime runtime = mln_test_create_runtime();
  token_transform probe;
  install_token_transform(runtime, &probe);

  load_from_server(runtime, server, "/same-origin.json", true);
  TEST_ASSERT_TRUE(request_carried_token(server, "/same-origin.json"));
  TEST_ASSERT_TRUE(request_carried_token(server, "/redirected.json"));

  load_from_server(runtime, server, "/cross-origin.json", true);
  TEST_ASSERT_TRUE(request_carried_token(server, "/cross-origin.json"));
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(other, "/elsewhere.json")
  );
  TEST_ASSERT_FALSE(request_carried_token(other, "/elsewhere.json"));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
  mln_test_http_server_stop(other);
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(resource_transform_registration_validates_its_descriptor);
  RUN_TEST(a_rules_transform_validates_its_rules);
  RUN_TEST(header_transform_registration_validates_its_descriptor);
  RUN_TEST(a_header_needs_a_transform_invocation_and_a_valid_field);
#if MLN_TEST_PLATFORM_HTTP
  RUN_TEST(a_resource_transform_rewrites_until_cleared);
  RUN_TEST(a_rules_transform_adds_query_parameters);
  RUN_TEST(a_retry_keeps_the_parameters_of_its_first_request);
  RUN_TEST(a_passed_through_request_reaches_the_rules);
  RUN_TEST(an_image_source_request_reaches_the_transforms_as_an_image);
#endif
#if MLN_TEST_PLATFORM_HTTP && MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  RUN_TEST(a_header_transform_adds_headers_until_cleared);
  RUN_TEST(a_header_transform_skips_requests_that_are_not_http);
  RUN_TEST(a_redirect_keeps_headers_only_within_the_origin);
  RUN_TEST(header_rules_see_the_rewritten_url);
#endif
}
