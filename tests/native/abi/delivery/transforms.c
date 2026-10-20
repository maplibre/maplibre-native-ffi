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

#if !defined(__EMSCRIPTEN__)
static mln_buffer_view copy_view(const char* text) {
  const size_t size = strlen(text);
  char* copy = malloc(size);
  TEST_ASSERT_NOT_NULL(copy);
  memcpy(copy, text, size);
  return (mln_buffer_view){.data = copy, .size = size};
}
#endif

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
    {.kinds = MLN_RESOURCE_KIND_MASK_ALL,
     .url = MLN_BUFFER_LITERAL("custom://first")},
    {.kinds = MLN_RESOURCE_KIND_MASK_ALL,
     .flags = 1U << 1U,
     .url = MLN_BUFFER_LITERAL("custom://second")},
  };
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, rules, 2, &diagnostic));
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "rewrite rule 1"));

  const mln_resource_rewrite_rule unknown_flag = {
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .flags = 1U << 31U,
    .url = MLN_BUFFER_LITERAL("custom://unknown"),
  };
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, &unknown_flag, 1, NULL));
  const mln_resource_rewrite_rule null_url = {
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .url = (mln_buffer_view){.data = NULL, .size = 1}
  };
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, &null_url, 1, NULL));
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, NULL, 1, NULL));
  // An empty registration must use the explicit clear command.
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, NULL, 0, NULL));

  // A glob longer than 4096 tokens is rejected here as it is for routes.
  static char long_glob[4097];
  memset(long_glob, '?', sizeof(long_glob));
  const mln_resource_rewrite_rule long_rewrite = {
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .flags = MLN_URL_MATCH_GLOB,
    .url = {.data = long_glob, .size = sizeof(long_glob)},
  };
  MLN_TEST_INVALID(submit_rewrite_rules(runtime, &long_rewrite, 1, NULL));

  // A rules transform owns no host user data.
  const mln_resource_transform with_user_data = {
    .size = sizeof(mln_resource_transform),
    .user_data = &diagnostic,
  };
  MLN_TEST_INVALID(mln_test_set_resource_transform(runtime, &with_user_data));

#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  // A diagnostic for a rejected header never carries its value.
  const mln_http_header header = {
    .name = MLN_BUFFER_LITERAL("Authorization"),
    .value = MLN_BUFFER_LITERAL("secret-token\r\n")
  };
  const mln_http_header_rule header_rule = {
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .url = MLN_BUFFER_LITERAL("custom://headers"),
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
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .url = MLN_BUFFER_LITERAL("custom://headers"),
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

  const mln_http_header token_header = {
    .name = MLN_BUFFER_LITERAL("X-Token"),
    .value = MLN_BUFFER_LITERAL("token"),
  };
  const mln_http_header_rule long_header_rule = {
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .flags = MLN_URL_MATCH_GLOB,
    .url = {.data = long_glob, .size = sizeof(long_glob)},
    .headers = &token_header,
    .header_count = 1,
  };
  const mln_http_header_transform long_header_transform = {
    .size = sizeof(mln_http_header_transform),
    .rules = &long_header_rule,
    .rule_count = 1,
  };
  MLN_TEST_INVALID(
    mln_test_set_http_header_transform(runtime, &long_header_transform)
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
    {.kinds = MLN_RESOURCE_KIND_MASK_TILE,
     .url = copy_view(original),
     .replacement_url = copy_view("http://127.0.0.1:1/tile.json")},
    {.kinds = MLN_RESOURCE_KIND_MASK_STYLE,
     .url = copy_view(original),
     .replacement_url = copy_view(rewritten)},
    // Kept URLs match here first and stay unchanged.
    {.kinds = MLN_RESOURCE_KIND_MASK_ALL,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_view("http://127.0.0.1:*/kept*")},
    {.kinds = MLN_RESOURCE_KIND_MASK_ALL,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_view("**"),
     .replacement_url = copy_view("http://127.0.0.1:1/never.json")},
  };
  const size_t count = sizeof(rules) / sizeof(rules[0]);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(submit_rewrite_rules(runtime, rules, count, NULL));
  for (size_t index = 0; index < count; index += 1) {
    free((char*)rules[index].url.data);
    free((char*)rules[index].replacement_url.data);
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
    {.name = copy_view("X-Token"), .value = copy_view("first")},
    {.name = copy_view("X-Extra"), .value = copy_view("extra")},
  };
  mln_http_header second[] = {
    {.name = copy_view("X-Token"), .value = copy_view("second")},
    {.name = copy_view("X-Second"), .value = copy_view("second")},
  };
  mln_http_header_rule rules[] = {
    {.kinds = MLN_RESOURCE_KIND_MASK_TILE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_view("**"),
     .headers = second,
     .header_count = 2},
    {.kinds = MLN_RESOURCE_KIND_MASK_STYLE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_view("http://127.0.0.1:*/**"),
     .headers = first,
     .header_count = 2},
    {.kinds = MLN_RESOURCE_KIND_MASK_ALL,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_view("**"),
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
    free((char*)first[index].name.data);
    free((char*)first[index].value.data);
    free((char*)second[index].name.data);
    free((char*)second[index].value.data);
  }
  for (size_t index = 0; index < sizeof(rules) / sizeof(rules[0]); index += 1) {
    free((char*)rules[index].url.data);
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

// Header rules compare the URL the resource transform produced, so a rule for
// the original URL adds nothing to a rewritten request, and a rule for the
// rewritten URL applies even though the style names the original.
static void header_rules_match_the_rewritten_url(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/rewritten.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char original[256];
  char rewritten[256];
  mln_test_http_server_url(
    server, "/original.json", original, sizeof(original)
  );
  mln_test_http_server_url(
    server, "/rewritten.json", rewritten, sizeof(rewritten)
  );

  mln_runtime runtime = mln_test_create_runtime();
  const mln_resource_rewrite_rule rewrite = {
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .url = {.data = original, .size = strlen(original)},
    .replacement_url = {.data = rewritten, .size = strlen(rewritten)},
  };
  MLN_TEST_OK(submit_rewrite_rules(runtime, &rewrite, 1, NULL));
  const mln_http_header original_header = {
    .name = MLN_BUFFER_LITERAL("X-Original"),
    .value = MLN_BUFFER_LITERAL("yes"),
  };
  const mln_http_header rewritten_header = {
    .name = MLN_BUFFER_LITERAL("X-Rewritten"),
    .value = MLN_BUFFER_LITERAL("yes"),
  };
  // The rule for the original URL comes first, so it would decide if header
  // rules compared the URL before rewriting.
  const mln_http_header_rule header_rules[] = {
    {.kinds = MLN_RESOURCE_KIND_MASK_ALL,
     .url = {.data = original, .size = strlen(original)},
     .headers = &original_header,
     .header_count = 1},
    {.kinds = MLN_RESOURCE_KIND_MASK_ALL,
     .url = {.data = rewritten, .size = strlen(rewritten)},
     .headers = &rewritten_header,
     .header_count = 1},
  };
  const mln_http_header_transform headers = {
    .size = sizeof(mln_http_header_transform),
    .rules = header_rules,
    .rule_count = 2,
  };
  MLN_TEST_OK(mln_test_set_http_header_transform(runtime, &headers));

  load_style(runtime, original);
  char value[64];
  TEST_ASSERT_TRUE(mln_test_http_server_request_header(
    server, "/rewritten.json", 0, "X-Rewritten", value, sizeof(value)
  ));
  TEST_ASSERT_FALSE(mln_test_http_server_request_header(
    server, "/rewritten.json", 0, "X-Original", value, sizeof(value)
  ));

  MLN_TEST_OK(mln_test_clear_http_header_transform(runtime));
  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
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
  RUN_TEST(header_rules_match_the_rewritten_url);
#endif
}
