// Queued resource providers: copied requests that the host completes after
// dispatch, cancelled requests, release order, and requests that a released
// queue fails.

#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

#if MLN_TEST_PLATFORM_HTTP
#include "support/http_server.h"
#endif

static const uint8_t inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

// What the last dispatched provider call received, copied during the call.
// Dispatch runs on the case's thread.
typedef struct kept_request {
  size_t calls;
  void* user_data;
  mln_resource_request handle;
  uint32_t size;
  mln_resource_kind kind;
  bool urls_terminated;
  size_t prior_etag_size;
  char requested_url[128];
} kept_request;

static kept_request kept;

// Keeps the request's handle for the case to answer after dispatch returns.
static void keep_request(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  kept.calls += 1;
  kept.user_data = user_data;
  kept.handle = handle;
  kept.size = request->size;
  kept.kind = request->kind;
  kept.urls_terminated =
    mln_test_lent_text_is_terminated(request->requested_url) &&
    mln_test_lent_text_is_terminated(request->resolved_url) &&
    mln_test_lent_text_is_terminated(request->prior_etag);
  kept.prior_etag_size = request->prior_etag.size;
  const size_t size = request->requested_url.size < sizeof(kept.requested_url)
                        ? request->requested_url.size
                        : sizeof(kept.requested_url) - 1;
  memcpy(kept.requested_url, request->requested_url.data, size);
  kept.requested_url[size] = '\0';
  mln_test_record_call('p', user_data, MLN_STATUS_OK, 0);
}

static mln_resource_provider queue_provider(
  mln_delivery_queue queue, void* tag
) {
  memset(&kept, 0, sizeof(kept));
  mln_test_calls_reset();
  return (mln_resource_provider){
    .size = sizeof(mln_resource_provider),
    .callback = keep_request,
    .user_data = tag,
    .queue = queue,
  };
}

static mln_resource_response style_response(void) {
  return (mln_resource_response){
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
    .bytes = inline_style_json,
    .byte_count = sizeof(inline_style_json) - 1,
  };
}

// Dispatches queue until one provider call has kept its request, and returns
// the request's handle.
static mln_resource_request await_request(mln_delivery_queue queue, void* tag) {
  mln_test_dispatch_until_calls(queue, 1);
  TEST_ASSERT_EQUAL_size_t(1, kept.calls);
  TEST_ASSERT_EQUAL_PTR(tag, kept.user_data);
  return kept.handle;
}

// The host dispatches a copied request on its own thread, keeps its handle,
// and completes it after the call. The style loads with the host's response.
static void the_host_completes_a_dispatched_request_later(void) {
  static const char style_url[] = "custom://queued-provider-style.json";
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;
  const mln_resource_provider provider = queue_provider(queue.handle, &tag);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, style_url));

  const mln_resource_request handle = await_request(queue.handle, &tag);
  TEST_ASSERT_EQUAL_UINT32(sizeof(mln_resource_request_info), kept.size);
  // The copy keeps each URL NUL-terminated and leaves the absent entity tag
  // empty.
  TEST_ASSERT_TRUE(kept.urls_terminated);
  TEST_ASSERT_EQUAL_size_t(0, kept.prior_etag_size);
  TEST_ASSERT_EQUAL_STRING(style_url, kept.requested_url);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_STYLE, kept.kind);

  const mln_resource_response response = style_response();
  MLN_TEST_OK(mln_resource_request_complete(handle, &response, NULL));
  // Completion consumed the handle, so no later call reaches the request and
  // releasing it does nothing.
  MLN_TEST_INVALID_STATE(mln_resource_request_pass_through(handle, NULL));
  mln_resource_request_release(handle);
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

// A wake that dispatches its queue on the thread that pushed the request,
// before the provider has answered HANDLE, and a provider call that completes
// each request there.
typedef struct completing_wake {
  mln_delivery_queue queue;
  atomic_int completed;
} completing_wake;

static void complete_request(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)request;
  completing_wake* wake = user_data;
  const mln_resource_response response = style_response();
  if (mln_resource_request_complete(handle, &response, NULL) == MLN_STATUS_OK) {
    atomic_fetch_add(&wake->completed, 1);
  } else {
    mln_resource_request_release(handle);
  }
}

static void complete_from_wake(void* user_data) {
  completing_wake* wake = user_data;
  while (mln_delivery_queue_dispatch(wake->queue, NULL) == MLN_STATUS_OK) {
  }
  mln_test_pulse();
}

static void a_request_completed_before_handle_returns_loads(void) {
  static completing_wake wake;
  atomic_store(&wake.completed, 0);
  wake.queue = MLN_HANDLE_NULL;
  const mln_delivery_queue_options options = {
    .size = sizeof(mln_delivery_queue_options),
    .wake = {.callback = complete_from_wake, .user_data = &wake},
  };
  MLN_TEST_OK(mln_delivery_queue_create(&options, &wake.queue, NULL));
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = complete_request,
    .user_data = &wake,
    .queue = wake.queue,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://inline-complete.json"));

  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&wake.completed));

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_delivery_queue_release(wake.queue);
}

static bool request_cancelled(void* context) {
  bool cancelled = false;
  return mln_resource_request_is_cancelled(
           *(const mln_resource_request*)context, &cancelled, NULL
         ) == MLN_STATUS_OK &&
         cancelled;
}

static void ignore_cancel(void* user_data, mln_resource_request handle) {
  (void)user_data;
  (void)handle;
}

// MapLibre may cancel a queued request before the host dispatches it. The
// call still receives the handle, the host sees the cancellation, and
// registers nothing.
static void a_request_cancelled_before_dispatch_reports_it(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_provider provider = queue_provider(queue.handle, NULL);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://cancelled.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wakes, 1));

  // A new style discards the pending style request.
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_empty_style_json));

  mln_resource_request handle = await_request(queue.handle, NULL);
  TEST_ASSERT_TRUE(mln_test_await(
    request_cancelled, &handle, mln_test_deadline_default(),
    "the request to be cancelled"
  ));
  const mln_resource_request_cancel_handler handler = {
    .size = sizeof(mln_resource_request_cancel_handler),
    .callback = ignore_cancel,
  };
  bool cancelled = false;
  MLN_TEST_OK(
    mln_resource_request_set_cancel_callback(handle, &handler, &cancelled, NULL)
  );
  TEST_ASSERT_TRUE(cancelled);
  MLN_TEST_INVALID_STATE(mln_resource_request_pass_through(handle, NULL));
  mln_resource_request_release(handle);

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

// Replacing a queue provider queues its release after its last request.
static void the_release_follows_the_last_request(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int first = 0;
  int second = 0;
  mln_resource_provider provider = queue_provider(queue.handle, &first);
  provider.release_user_data = mln_test_record_release;
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://retired.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wakes, 1));

  provider.user_data = &second;
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));

  MLN_TEST_OK(mln_delivery_queue_dispatch(queue.handle, NULL));
  TEST_ASSERT_EQUAL_size_t(3, mln_test_calls.count);
  TEST_ASSERT_EQUAL_CHAR('p', mln_test_call_at(0)->kind);
  TEST_ASSERT_EQUAL_PTR(&first, mln_test_call_at(0)->user_data);
  TEST_ASSERT_EQUAL_CHAR('r', mln_test_call_at(1)->kind);
  TEST_ASSERT_EQUAL_PTR(&first, mln_test_call_at(1)->user_data);
  TEST_ASSERT_EQUAL_CHAR('r', mln_test_call_at(2)->kind);
  TEST_ASSERT_EQUAL_PTR(&second, mln_test_call_at(2)->user_data);
  mln_resource_request_release(kept.handle);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

// A refused queue delivery fails the request before native loading.
static void a_released_queue_fails_requests(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_provider provider = queue_provider(queue.handle, NULL);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_test_delivery_queue_release(&queue);
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://passed-through.json"));

  char message[512] = {0};
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "released without a response"));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&queue.wakes));

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Releasing a queue that holds a request it has not dispatched releases its
// handle, which fails the request instead of leaving its load waiting.
static void releasing_a_queue_fails_its_pending_requests(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_provider provider = queue_provider(queue.handle, NULL);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://pending.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wakes, 1));

  mln_test_delivery_queue_release(&queue);
  char message[256] = {0};
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "released without a response"));

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_queue_provider_rejects_an_invalid_shape(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  // A queue does not stand in for the callback.
  mln_resource_provider provider = queue_provider(queue.handle, NULL);
  provider.callback = NULL;
  MLN_TEST_INVALID(mln_test_set_resource_provider(runtime, &provider));
  provider.queue = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_test_set_resource_provider(runtime, &provider));
  mln_test_delivery_queue_release(&queue);
  provider = queue_provider(queue.handle, NULL);
  MLN_TEST_INVALID_STATE(mln_test_set_resource_provider(runtime, &provider));
  mln_test_destroy_runtime(runtime);
}

static mln_status submit_routes(
  mln_runtime runtime, const mln_resource_route* routes, size_t count,
  mln_diagnostic* diagnostic
) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_resource_provider queued = queue_provider(queue.handle, NULL);
  queued.routes = routes;
  queued.route_count = count;
  mln_test_completion completion = mln_test_completion_default(0);
  mln_status status = mln_runtime_set_resource_provider(
    runtime, &queued, &completion.descriptor, diagnostic
  );
  if (status == MLN_STATUS_OK) {
    status = mln_test_completion_settle(&completion);
    MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  } else {
    mln_test_completion_reject(&completion);
    mln_test_completion_destroy(&completion);
  }
  mln_test_delivery_queue_release(&queue);
  return status;
}

static void invalid_routes_are_rejected(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_resource_route routes[] = {
    {.size = sizeof(mln_resource_route),
     .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
     .url = MLN_BUFFER_LITERAL("custom://first")},
    {.size = sizeof(mln_resource_route),
     .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
     .flags = 1U << 31U,
     .url = MLN_BUFFER_LITERAL("**")},
  };
  MLN_TEST_INVALID(submit_routes(runtime, routes, 2, &diagnostic));
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "route 1"));
  const mln_resource_route unknown_kind = {
    .size = sizeof(mln_resource_route),
    .kinds = 1U << 31U,
    .url = MLN_BUFFER_LITERAL("**")
  };
  MLN_TEST_INVALID(submit_routes(runtime, &unknown_kind, 1, NULL));
  // Zero kinds select every kind rather than none.
  mln_resource_route zero_kinds = {.size = sizeof(mln_resource_route)};
  zero_kinds.url = (mln_buffer_view)MLN_BUFFER_LITERAL("**");
  MLN_TEST_OK(submit_routes(runtime, &zero_kinds, 1, NULL));
  const mln_resource_route null_url = {
    .size = sizeof(mln_resource_route),
    .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
    .url = (mln_buffer_view){.data = NULL, .size = 1}
  };
  MLN_TEST_INVALID(submit_routes(runtime, &null_url, 1, NULL));
  MLN_TEST_INVALID(submit_routes(runtime, NULL, 1, NULL));
  MLN_TEST_OK(submit_routes(runtime, routes, 1, NULL));

  // A glob may hold 4096 tokens, and each ? is one. An exact URL has no limit.
  static char long_url[4097];
  memset(long_url, '?', sizeof(long_url));
  mln_resource_route long_route = {
    .size = sizeof(mln_resource_route),
    .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
    .flags = MLN_URL_MATCH_GLOB,
    .url = {.data = long_url, .size = 4096},
  };
  MLN_TEST_OK(submit_routes(runtime, &long_route, 1, NULL));
  long_route.url.size = 4097;
  MLN_TEST_INVALID(submit_routes(runtime, &long_route, 1, NULL));
  long_route.flags = MLN_URL_MATCH_EXACT;
  MLN_TEST_OK(submit_routes(runtime, &long_route, 1, NULL));

  // An escaped byte and a run of * each count as one token, so 4095 escapes
  // and one long run make exactly 4096.
  static char escapes_and_run[4095 * 2 + 8192];
  for (size_t index = 0; index < 4095; index += 1) {
    escapes_and_run[index * 2] = '\\';
    escapes_and_run[index * 2 + 1] = '?';
  }
  memset(escapes_and_run + 4095 * 2, '*', 8192);
  const mln_resource_route boundary_route = {
    .size = sizeof(mln_resource_route),
    .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
    .flags = MLN_URL_MATCH_GLOB,
    .url = {.data = escapes_and_run, .size = sizeof(escapes_and_run)},
  };
  MLN_TEST_OK(submit_routes(runtime, &boundary_route, 1, NULL));
  mln_test_destroy_runtime(runtime);
}

// Each route's size steps to the next, so a caller built against a later
// header passes larger routes, and native skips the members it does not know.
// A route smaller than this header's struct is rejected.
static void routes_step_by_their_declared_size(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  typedef struct later_route {
    mln_resource_route route;
    uint64_t later_member;
  } later_route;
  later_route routes[] = {
    {.route =
       {.size = sizeof(later_route),
        .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
        .url = MLN_BUFFER_LITERAL("custom://first")},
     .later_member = UINT64_MAX},
    {.route = {
       .size = sizeof(later_route),
       .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
       .flags = 1U << 31U,
       .url = MLN_BUFFER_LITERAL("**")
     }},
  };
  MLN_TEST_INVALID(submit_routes(runtime, &routes[0].route, 2, &diagnostic));
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "route 1"));
  routes[1].route.flags = MLN_URL_MATCH_GLOB;
  MLN_TEST_OK(submit_routes(runtime, &routes[0].route, 2, NULL));

  mln_resource_route undersized = {.size = sizeof(mln_resource_route)};
  undersized.size = sizeof(undersized) - sizeof(uint64_t);
  undersized.url = (mln_buffer_view)MLN_BUFFER_LITERAL("**");
  MLN_TEST_INVALID(submit_routes(runtime, &undersized, 1, &diagnostic));
  TEST_ASSERT_NOT_NULL(
    strstr(diagnostic.message, "route 0 has an invalid size")
  );
  mln_test_destroy_runtime(runtime);
}

#if MLN_TEST_PLATFORM_HTTP

// A callback provider that answers every request it sees with an empty style.
static void answer_with_style(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)request;
  atomic_fetch_add((atomic_int*)user_data, 1);
  const mln_resource_response response = style_response();
  mln_test_request_answer(handle, &response);
}

static void load_style(mln_runtime runtime, const char* url) {
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, url));
  TEST_ASSERT_TRUE_MESSAGE(mln_test_await_style_loaded(runtime, map), url);
  mln_test_destroy_map(map);
}

// Routes filter a callback provider too: a request inside them reaches the
// callback, and one outside them loads natively from the server.
static void routes_filter_a_callback_provider(void) {
  static const mln_test_http_route served[] = {
    {.path = "/native/style.json",
     .body = inline_style_json,
     .body_size = sizeof(inline_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(served, 1);
  char routed_url[256];
  char native_url[256];
  mln_test_http_server_url(
    server, "/routed/style.json", routed_url, sizeof(routed_url)
  );
  mln_test_http_server_url(
    server, "/native/style.json", native_url, sizeof(native_url)
  );
  static atomic_int calls;
  atomic_store(&calls, 0);
  const mln_resource_route routes[] = {
    // Another kind: claims nothing here.
    {.size = sizeof(mln_resource_route),
     .kinds = MLN_RESOURCE_KIND_FLAG_TILE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = MLN_BUFFER_LITERAL("**")},
    {.size = sizeof(mln_resource_route),
     .kinds = MLN_RESOURCE_KIND_FLAG_STYLE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = MLN_BUFFER_LITERAL("http://127.0.0.1:*/routed/**")},
  };
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = answer_with_style,
    .user_data = &calls,
    .routes = routes,
    .route_count = 2,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));

  load_style(runtime, routed_url);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&calls));
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_http_server_requests(server, "/routed/style.json")
  );
  load_style(runtime, native_url);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&calls));
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/native/style.json")
  );

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_runtime(runtime);
  mln_test_http_server_stop(server);
}

// A queue provider's routes decide which requests reach the queue.
static void routes_filter_a_queue_provider(void) {
  static const mln_test_http_route served[] = {
    {.path = "/native/style.json",
     .body = inline_style_json,
     .body_size = sizeof(inline_style_json) - 1},
  };
  mln_test_http_server* server = mln_test_http_server_start(served, 1);
  char native_url[256];
  mln_test_http_server_url(
    server, "/native/style.json", native_url, sizeof(native_url)
  );
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_route route = {
    .size = sizeof(mln_resource_route),
    .kinds = MLN_RESOURCE_KIND_FLAG_ALL,
    .flags = MLN_URL_MATCH_GLOB,
    .url = MLN_BUFFER_LITERAL("custom://routed/**"),
  };
  mln_resource_provider provider = queue_provider(queue.handle, NULL);
  provider.routes = &route;
  provider.route_count = 1;
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));

  load_style(runtime, native_url);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&queue.wakes));

  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://routed/style.json"));
  const mln_resource_request handle = await_request(queue.handle, NULL);
  TEST_ASSERT_EQUAL_STRING("custom://routed/style.json", kept.requested_url);
  const mln_resource_response response = style_response();
  MLN_TEST_OK(mln_resource_request_complete(handle, &response, NULL));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
  mln_test_http_server_stop(server);
}

// Answers every transform with the URL in user_data.
static void transform_to(
  void* user_data, uint32_t kind, mln_buffer_view url,
  mln_resource_transform_request request
) {
  (void)kind;
  (void)url;
  const char* target = user_data;
  MLN_TEST_OK(mln_resource_transform_request_complete(
    request, mln_buffer_view_from_cstr(target), NULL
  ));
}

// The host can decline a request after its dispatched call. The resource
// transform still applies, and the request never visits the provider a second
// time.
static void a_dispatched_request_can_pass_through_once(void) {
  const mln_test_http_route route = {
    .path = "/rewritten.json",
    .body = inline_style_json,
    .body_size = sizeof(inline_style_json) - 1,
  };
  mln_test_http_server* server = mln_test_http_server_start(&route, 1);
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
  const mln_resource_provider provider = queue_provider(queue.handle, NULL);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  const mln_resource_transform transform = {
    .size = sizeof(mln_resource_transform),
    .callback = transform_to,
    .user_data = rewritten,
  };
  MLN_TEST_OK(mln_test_set_resource_transform(runtime, &transform));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, original));
  const mln_resource_request handle = await_request(queue.handle, NULL);
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_http_server_requests(server, "/rewritten.json")
  );
  MLN_TEST_OK(mln_resource_request_pass_through(handle, NULL));
  MLN_TEST_INVALID_STATE(mln_resource_request_pass_through(handle, NULL));
  const mln_resource_response response = style_response();
  MLN_TEST_INVALID_STATE(
    mln_resource_request_complete(handle, &response, NULL)
  );
  mln_resource_request_release(handle);
  MLN_TEST_INVALID_STATE(mln_resource_request_pass_through(handle, NULL));
  TEST_ASSERT_TRUE(mln_test_await_style_loaded(runtime, map));
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_http_server_requests(server, "/rewritten.json")
  );
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_http_server_requests(server, "/original.json")
  );
  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
  mln_test_http_server_stop(server);
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(the_host_completes_a_dispatched_request_later);
  RUN_TEST(a_request_completed_before_handle_returns_loads);
  RUN_TEST(a_request_cancelled_before_dispatch_reports_it);
  RUN_TEST(the_release_follows_the_last_request);
  RUN_TEST(a_released_queue_fails_requests);
  RUN_TEST(releasing_a_queue_fails_its_pending_requests);
  RUN_TEST(a_queue_provider_rejects_an_invalid_shape);
  RUN_TEST(invalid_routes_are_rejected);
  RUN_TEST(routes_step_by_their_declared_size);
#if MLN_TEST_PLATFORM_HTTP
  RUN_TEST(routes_filter_a_callback_provider);
  RUN_TEST(routes_filter_a_queue_provider);
  RUN_TEST(a_dispatched_request_can_pass_through_once);
#endif
}
