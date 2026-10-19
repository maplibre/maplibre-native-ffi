// Native transform rules, which answer a transform inside the call for a host
// that cannot run code on a MapLibre thread: what registration copies and
// rejects, and, against a loopback server, the first matching rule deciding.

#include <stdlib.h>
#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

#if !defined(__EMSCRIPTEN__)
#include "support/http_server.h"
#endif

#if defined(__EMSCRIPTEN__) || defined(__OHOS__)
#define MLN_TEST_HEADER_TRANSFORM_SUPPORTED 0
#else
#define MLN_TEST_HEADER_TRANSFORM_SUPPORTED 1
#endif

static const char empty_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static char* copy_string(const char* text) {
  const size_t size = strlen(text) + 1;
  char* copy = malloc(size);
  TEST_ASSERT_NOT_NULL(copy);
  memcpy(copy, text, size);
  return copy;
}

static mln_status submit_rewrite_rules(
  mln_runtime runtime, const mln_resource_rewrite_rule* rules, size_t count,
  mln_diagnostic* diagnostic
) {
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .rules = rules,
    .rule_count = count,
  };
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status status = mln_runtime_set_resource_transform(
    runtime, &transform, &completion.descriptor, diagnostic
  );
  if (status != MLN_STATUS_OK) {
    mln_test_completion_reject(&completion);
    mln_test_completion_destroy(&completion);
    return status;
  }
  return mln_test_completion_settle(&completion);
}

static void rule_registrations_reject_an_invalid_rule(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_resource_rewrite_rule rules[] = {
    {.kind = MLN_RESOURCE_KIND_ANY, .url = "custom://first"},
    {.kind = MLN_RESOURCE_KIND_ANY,
     .flags = MLN_URL_MATCH_REQUESTED_URL,
     .url = "custom://second"},
  };
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, rules, 2, &diagnostic));
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "rewrite rule 1"));

  const mln_resource_rewrite_rule unknown_flag = {
    .kind = MLN_RESOURCE_KIND_ANY,
    .flags = 1U << 31U,
    .url = "custom://unknown",
  };
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, &unknown_flag, 1, NULL));
  const mln_resource_rewrite_rule null_url = {.kind = MLN_RESOURCE_KIND_ANY};
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, &null_url, 1, NULL));
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, NULL, 1, NULL));
  // An empty table is a transform that rewrites nothing.
  MLN_TEST_OK(submit_rewrite_rules(runtime, NULL, 0, NULL));

  // A rules transform owns no host user data.
  const mln_resource_transform with_user_data = {
    .size = sizeof(mln_resource_transform),
    .user_data = &diagnostic,
  };
  MLN_TEST_INVALID(mln_test_set_resource_transform(runtime, &with_user_data));

#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  // A diagnostic for a rejected header never carries its value.
  const mln_http_header header = {
    .name = "Authorization", .value = "secret-token\r\n"
  };
  const mln_http_header_rule header_rule = {
    .kind = MLN_RESOURCE_KIND_ANY,
    .url = "custom://headers",
    .headers = &header,
    .header_count = 1,
  };
  const mln_http_header_transform transform = {
    .size = sizeof(mln_http_header_transform),
    .rules = &header_rule,
    .rule_count = 1,
  };
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_INVALID(mln_runtime_set_http_header_transform(
    runtime, &transform, &completion.descriptor, &diagnostic
  ));
  mln_test_completion_reject(&completion);
  mln_test_completion_destroy(&completion);
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "header rule 0"));
  TEST_ASSERT_NULL(strstr(diagnostic.message, "secret-token"));

  const mln_http_header_rule null_headers = {
    .kind = MLN_RESOURCE_KIND_ANY,
    .url = "custom://headers",
    .header_count = 1,
  };
  const mln_http_header_transform null_headers_transform = {
    .size = sizeof(mln_http_header_transform),
    .rules = &null_headers,
    .rule_count = 1,
  };
  MLN_TEST_INVALID(
    mln_test_set_http_header_transform(runtime, &null_headers_transform)
  );
#endif
  mln_test_destroy_runtime(runtime);
}

#if !defined(__EMSCRIPTEN__)

static void load_style(mln_runtime runtime, const char* url) {
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), url);
  mln_test_destroy_map(map);
}

// Registration copies every rule and string, so the caller frees them at
// once. The first rule whose kind and URL match decides, and one without a
// replacement leaves the URL unchanged.
static void the_first_matching_rewrite_rule_decides(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/rewritten.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/kept.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 2);
  char original[256];
  char rewritten[256];
  char kept[256];
  mln_test_http_server_url(
    server, "/original.json", original, sizeof(original)
  );
  mln_test_http_server_url(
    server, "/rewritten.json", rewritten, sizeof(rewritten)
  );
  mln_test_http_server_url(server, "/kept.json", kept, sizeof(kept));

  mln_resource_rewrite_rule rules[] = {
    // Another kind: no match.
    {.kind = MLN_RESOURCE_KIND_TILE,
     .url = copy_string(original),
     .replacement_url = copy_string("http://127.0.0.1:1/tile.json")},
    {.kind = MLN_RESOURCE_KIND_STYLE,
     .url = copy_string(original),
     .replacement_url = copy_string(rewritten)},
    // Kept URLs match here first and stay unchanged.
    {.kind = MLN_RESOURCE_KIND_ANY,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_string("http://127.0.0.1:*/kept*")},
    {.kind = MLN_RESOURCE_KIND_ANY,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_string("**"),
     .replacement_url = copy_string("http://127.0.0.1:1/never.json")},
  };
  const size_t count = sizeof(rules) / sizeof(rules[0]);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(submit_rewrite_rules(runtime, rules, count, NULL));
  for (size_t index = 0; index < count; index += 1) {
    free((char*)rules[index].url);
    free((char*)rules[index].replacement_url);
    rules[index] = (mln_resource_rewrite_rule){0};
  }

  load_style(runtime, original);
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/rewritten.json")
  );
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_http_server_requests(server, "/original.json")
  );
  load_style(runtime, kept);
  TEST_ASSERT_EQUAL_INT(1, mln_test_http_server_requests(server, "/kept.json"));

  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

#endif

#if !defined(__EMSCRIPTEN__) && MLN_TEST_HEADER_TRANSFORM_SUPPORTED

// The first matching header rule supplies all its headers; a later rule that
// also matches adds none.
static void the_first_matching_header_rule_supplies_its_headers(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/style.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char url[256];
  mln_test_http_server_url(server, "/style.json", url, sizeof(url));

  mln_http_header first[] = {
    {.name = copy_string("X-Token"), .value = copy_string("first")},
    {.name = copy_string("X-Extra"), .value = copy_string("extra")},
  };
  mln_http_header second[] = {
    {.name = copy_string("X-Token"), .value = copy_string("second")},
    {.name = copy_string("X-Second"), .value = copy_string("second")},
  };
  mln_http_header_rule rules[] = {
    {.kind = MLN_RESOURCE_KIND_TILE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_string("**"),
     .headers = second,
     .header_count = 2},
    {.kind = MLN_RESOURCE_KIND_STYLE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_string("http://127.0.0.1:*/**"),
     .headers = first,
     .header_count = 2},
    {.kind = MLN_RESOURCE_KIND_ANY,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_string("**"),
     .headers = second,
     .header_count = 2},
  };
  const mln_http_header_transform transform = {
    .size = sizeof(mln_http_header_transform),
    .rules = rules,
    .rule_count = sizeof(rules) / sizeof(rules[0]),
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_http_header_transform(runtime, &transform));
  for (size_t index = 0; index < 2; index += 1) {
    free((char*)first[index].name);
    free((char*)first[index].value);
    free((char*)second[index].name);
    free((char*)second[index].value);
  }
  for (size_t index = 0; index < sizeof(rules) / sizeof(rules[0]); index += 1) {
    free((char*)rules[index].url);
  }
  memset(first, 0, sizeof(first));
  memset(second, 0, sizeof(second));

  load_style(runtime, url);
  char value[64];
  TEST_ASSERT_TRUE(mln_test_http_server_request_header(
    server, "/style.json", 0, "X-Token", value, sizeof(value)
  ));
  TEST_ASSERT_EQUAL_STRING("first", value);
  TEST_ASSERT_TRUE(mln_test_http_server_request_header(
    server, "/style.json", 0, "X-Extra", value, sizeof(value)
  ));
  TEST_ASSERT_FALSE(mln_test_http_server_request_header(
    server, "/style.json", 0, "X-Second", value, sizeof(value)
  ));

  MLN_TEST_OK(mln_test_clear_http_header_transform(runtime));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(rule_registrations_reject_an_invalid_rule);
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(the_first_matching_rewrite_rule_decides);
#endif
#if !defined(__EMSCRIPTEN__) && MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  RUN_TEST(the_first_matching_header_rule_supplies_its_headers);
#endif
}
