// Transforms for a host that cannot run code on a MapLibre thread: a queued
// resource transform, which the host answers after dispatch, and native header
// rules, which answer inside the call. Covers what registration copies and
// rejects, and, against a loopback server, the answer that reaches the
// transport.

#include <stdlib.h>
#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

#if MLN_TEST_PLATFORM_HTTP
#include "support/http_server.h"
#endif

#if defined(__EMSCRIPTEN__) || defined(__OHOS__)
#define MLN_TEST_HEADER_TRANSFORM_SUPPORTED 0
#else
#define MLN_TEST_HEADER_TRANSFORM_SUPPORTED 1
#endif

static const char empty_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

#if MLN_TEST_PLATFORM_HTTP && MLN_TEST_HEADER_TRANSFORM_SUPPORTED
static mln_buffer_view copy_view(const char* text) {
  const size_t size = strlen(text);
  char* copy = malloc(size);
  TEST_ASSERT_NOT_NULL(copy);
  memcpy(copy, text, size);
  return (mln_buffer_view){.data = copy, .size = size};
}
#endif

// What the last dispatched transform call received, copied during the call.
// Dispatch runs on the case's thread.
typedef struct kept_transform {
  size_t calls;
  void* user_data;
  mln_resource_kind kind;
  bool url_terminated;
  char url[256];
  mln_resource_transform_request request;
} kept_transform;

static kept_transform kept;

// Keeps the request for the case to answer after dispatch returns.
static void keep_transform(
  void* user_data, mln_resource_kind kind, mln_buffer_view url,
  mln_resource_transform_request request
) {
  kept.calls += 1;
  kept.user_data = user_data;
  kept.kind = kind;
  kept.url_terminated = mln_test_lent_text_is_terminated(url);
  const size_t size =
    url.size < sizeof(kept.url) ? url.size : sizeof(kept.url) - 1;
  memcpy(kept.url, url.data, size);
  kept.url[size] = '\0';
  kept.request = request;
  mln_test_record_call('t', user_data, MLN_STATUS_OK, 0);
}

static mln_resource_transform queue_transform(
  mln_delivery_queue queue, void* tag
) {
  memset(&kept, 0, sizeof(kept));
  mln_test_calls_reset();
  return (mln_resource_transform){
    .size = sizeof(mln_resource_transform),
    .callback = keep_transform,
    .user_data = tag,
    .queue = queue,
  };
}

// A queue does not stand in for a transform's callback.
static void a_queue_transform_rejects_an_invalid_shape(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_resource_transform transform = queue_transform(queue.handle, NULL);
  transform.callback = NULL;
  MLN_TEST_INVALID(mln_test_set_resource_transform(runtime, &transform));
  transform.queue = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_test_set_resource_transform(runtime, &transform));
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

static void rule_registrations_reject_an_invalid_rule(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
#if MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  // A diagnostic for a rejected header never carries its value.
  const mln_http_header header = {
    .name = MLN_BUFFER_LITERAL("Authorization"),
    .value = MLN_BUFFER_LITERAL("secret-token\r\n")
  };
  const mln_http_header_rule header_rule = {
    .size = sizeof(mln_http_header_rule),
    .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
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
    .size = sizeof(mln_http_header_rule),
    .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
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

  // A rule smaller than this header's struct has no size to step by.
  mln_http_header_rule undersized_rule = {.size = sizeof(mln_http_header_rule)};
  undersized_rule.size = sizeof(undersized_rule) - sizeof(uint64_t);
  undersized_rule.url = (mln_buffer_view)MLN_BUFFER_LITERAL("custom://headers");
  const mln_http_header_transform undersized_transform = {
    .size = sizeof(mln_http_header_transform),
    .rules = &undersized_rule,
    .rule_count = 1,
  };
  MLN_TEST_INVALID(
    mln_test_set_http_header_transform(runtime, &undersized_transform)
  );

  const mln_http_header token_header = {
    .name = MLN_BUFFER_LITERAL("X-Token"),
    .value = MLN_BUFFER_LITERAL("token"),
  };
  // A glob longer than 4096 tokens is rejected here as it is for routes.
  static char long_glob[4097];
  memset(long_glob, '?', sizeof(long_glob));
  const mln_http_header_rule long_header_rule = {
    .size = sizeof(mln_http_header_rule),
    .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
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
#else
  (void)diagnostic;
#endif
  mln_test_destroy_runtime(runtime);
}

#if MLN_TEST_PLATFORM_HTTP

// Dispatches queue until one more transform call has kept its request, and
// returns the request.
static mln_resource_transform_request await_transform(
  mln_delivery_queue queue, void* tag
) {
  const size_t before = kept.calls;
  mln_test_dispatch_until_calls(queue, mln_test_calls.count + 1);
  TEST_ASSERT_EQUAL_size_t(before + 1, kept.calls);
  TEST_ASSERT_EQUAL_PTR(tag, kept.user_data);
  return kept.request;
}

// The request waits for the host, which answers after its dispatched call:
// the transport fetches the URL the host names. An answer is one-shot, and the
// handle still needs its release.
static void the_host_answers_a_dispatched_transform_later(void) {
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
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;
  const mln_resource_transform transform = queue_transform(queue.handle, &tag);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, original));

  const mln_resource_transform_request request =
    await_transform(queue.handle, &tag);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_STYLE, kept.kind);
  TEST_ASSERT_TRUE(kept.url_terminated);
  TEST_ASSERT_EQUAL_STRING(original, kept.url);

  MLN_TEST_INVALID(
    mln_resource_transform_request_complete(request, (mln_buffer_view){0}, NULL)
  );
  MLN_TEST_INVALID(mln_resource_transform_request_complete(
    request, MLN_BUFFER_LITERAL("custom://before\0after"), NULL
  ));
  MLN_TEST_OK(mln_resource_transform_request_complete(
    request, mln_buffer_view_from_cstr(rewritten), NULL
  ));
  // The answer consumed the handle, so a second answer finds it gone and a
  // release does nothing.
  MLN_TEST_INVALID_STATE(mln_resource_transform_request_complete(
    request, mln_buffer_view_from_cstr(rewritten), NULL
  ));
  mln_resource_transform_request_release(request);
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/rewritten.json")
  );
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_http_server_requests(server, "/original.json")
  );

  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
  mln_test_http_server_stop(server);
}

// A request the host releases unanswered, and one that a released queue
// abandons: both keep their URLs.
static void an_unanswered_transform_keeps_its_url(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/abandoned.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
    {.path = "/released.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 2);
  char abandoned[256];
  char released[256];
  mln_test_http_server_url(
    server, "/abandoned.json", abandoned, sizeof(abandoned)
  );
  mln_test_http_server_url(
    server, "/released.json", released, sizeof(released)
  );
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_transform transform = queue_transform(queue.handle, NULL);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));

  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, released));
  mln_resource_transform_request_release(await_transform(queue.handle, NULL));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/released.json")
  );
  mln_test_destroy_map(map);

  map = mln_test_create_map(runtime);
  const int wakes = atomic_load(&queue.wakes);
  MLN_TEST_OK(mln_test_map_set_style_url(map, abandoned));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wakes, wakes + 1));
  mln_test_delivery_queue_release(&queue);
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/abandoned.json")
  );

  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// Clearing the transform queues its release after the last request it
// queued.
static void the_release_follows_the_last_transform(void) {
  static const mln_test_http_route routes[] = {
    {.path = "/style.json",
     .body = empty_style_json,
     .body_size = sizeof(empty_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(routes, 1);
  char url[256];
  mln_test_http_server_url(server, "/style.json", url, sizeof(url));
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;
  mln_resource_transform transform = queue_transform(queue.handle, &tag);
  transform.release_user_data = mln_test_record_release;
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  const mln_resource_transform_request request =
    await_transform(queue.handle, &tag);

  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  mln_test_dispatch_until_calls(queue.handle, 2);
  TEST_ASSERT_EQUAL_CHAR('r', mln_test_call_at(1)->kind);
  TEST_ASSERT_EQUAL_PTR(&tag, mln_test_call_at(1)->user_data);
  // The clear leaves a request already dispatched waiting for its answer.
  mln_resource_transform_request_release(request);
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
  mln_test_http_server_stop(server);
}

#endif

#if MLN_TEST_PLATFORM_HTTP && MLN_TEST_HEADER_TRANSFORM_SUPPORTED

static void load_style(mln_runtime runtime, const char* url) {
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), url);
  mln_test_destroy_map(map);
}

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
    {.size = sizeof(mln_http_header_rule),
     .kinds = MLN_RESOURCE_KIND_FLAG_TILE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_view("**"),
     .headers = second,
     .header_count = 2},
    {.size = sizeof(mln_http_header_rule),
     .kinds = MLN_RESOURCE_KIND_FLAG_STYLE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = copy_view("http://127.0.0.1:*/**"),
     .headers = first,
     .header_count = 2},
    {.size = sizeof(mln_http_header_rule),
     .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
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

  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_transform transform = queue_transform(queue.handle, NULL);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));
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
    {.size = sizeof(mln_http_header_rule),
     .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
     .url = mln_buffer_view_from_cstr(original),
     .headers = &original_header,
     .header_count = 1},
    {.size = sizeof(mln_http_header_rule),
     .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
     .url = mln_buffer_view_from_cstr(rewritten),
     .headers = &rewritten_header,
     .header_count = 1},
  };
  const mln_http_header_transform headers = {
    .size = sizeof(mln_http_header_transform),
    .rules = header_rules,
    .rule_count = 2,
  };
  MLN_TEST_OK(mln_test_set_http_header_transform(runtime, &headers));

  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, original));
  const mln_resource_transform_request request =
    await_transform(queue.handle, NULL);
  MLN_TEST_OK(mln_resource_transform_request_complete(
    request, mln_buffer_view_from_cstr(rewritten), NULL
  ));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  char value[64];
  TEST_ASSERT_TRUE(mln_test_http_server_request_header(
    server, "/rewritten.json", 0, "X-Rewritten", value, sizeof(value)
  ));
  TEST_ASSERT_FALSE(mln_test_http_server_request_header(
    server, "/rewritten.json", 0, "X-Original", value, sizeof(value)
  ));

  MLN_TEST_OK(mln_test_clear_http_header_transform(runtime));
  MLN_TEST_OK(mln_test_clear_resource_transform(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
  mln_test_http_server_stop(server);
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(a_queue_transform_rejects_an_invalid_shape);
  RUN_TEST(rule_registrations_reject_an_invalid_rule);
#if MLN_TEST_PLATFORM_HTTP
  RUN_TEST(the_host_answers_a_dispatched_transform_later);
  RUN_TEST(an_unanswered_transform_keeps_its_url);
  RUN_TEST(the_release_follows_the_last_transform);
#endif
#if MLN_TEST_PLATFORM_HTTP && MLN_TEST_HEADER_TRANSFORM_SUPPORTED
  RUN_TEST(the_first_matching_header_rule_supplies_its_headers);
  RUN_TEST(header_rules_match_the_rewritten_url);
#endif
}
