// Queue-mode resource providers: copied requests that the host completes after
// draining, untaken and cancelled requests, retirement, and requests that a
// released queue fails.

#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

#if !defined(__EMSCRIPTEN__)
#include "support/http_server.h"
#endif

static const uint8_t inline_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static mln_resource_provider queue_provider(
  mln_delivery_queue queue, void* tag
) {
  return (mln_resource_provider){
    .size = sizeof(mln_resource_provider),
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

// Waits for one request delivery and returns its batch.
static mln_delivery_batch await_request(
  mln_delivery_queue queue, void* tag,
  const mln_resource_request_delivery** out_request
) {
  const mln_delivery* delivery = NULL;
  const mln_delivery_batch batch =
    mln_test_delivery_await_single(queue, &delivery);
  TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_RESOURCE_REQUEST, delivery->type);
  TEST_ASSERT_EQUAL_PTR(tag, delivery->user_data);
  *out_request = &delivery->payload.resource_request;
  return batch;
}

// The host drains a copied request on its own thread, takes its handle, and
// completes it there. The style loads with the host's response.
static void the_host_completes_a_drained_request(void) {
  static const char style_url[] = "custom://queued-provider-style.json";
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int tag = 0;
  const mln_resource_provider provider = queue_provider(queue.handle, &tag);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, style_url));

  const mln_resource_request_delivery* request = NULL;
  const mln_delivery_batch batch = await_request(queue.handle, &tag, &request);
  TEST_ASSERT_EQUAL_UINT32(
    sizeof(mln_resource_request_info), request->request->size
  );
  TEST_ASSERT_EQUAL_STRING(style_url, request->request->requested_url);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_STYLE, request->request->kind);
  const mln_resource_request handle = request->handle;
  MLN_TEST_OK(mln_delivery_batch_take(batch, 0, NULL));
  // A delivery's owned handle transfers once.
  MLN_TEST_INVALID_STATE(mln_delivery_batch_take(batch, 0, NULL));
  mln_delivery_batch_release(batch);

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

// Releasing a batch without taking the handle fails the request, so the
// style load reports an error instead of waiting.
static void an_untaken_request_fails(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_provider provider = queue_provider(queue.handle, NULL);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://untaken.json"));

  const mln_resource_request_delivery* request = NULL;
  mln_delivery_batch_release(await_request(queue.handle, NULL, &request));
  char message[256] = {0};
  TEST_ASSERT_TRUE(
    mln_test_await_loading_failure(runtime, map, message, sizeof(message))
  );
  TEST_ASSERT_NOT_NULL(strstr(message, "released without a response"));

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

// A wake that completes every request it drains, on the thread that pushed
// it, before the provider has answered HANDLE.
typedef struct completing_wake {
  mln_delivery_queue queue;
  atomic_int completed;
} completing_wake;

static void complete_from_wake(void* user_data) {
  completing_wake* wake = user_data;
  mln_delivery_batch batch = MLN_HANDLE_NULL;
  mln_delivery_batch_view view = {.size = sizeof(view)};
  while (mln_delivery_queue_drain(wake->queue, &batch, &view, NULL) ==
         MLN_STATUS_OK) {
    for (size_t index = 0; index < view.delivery_count; index += 1) {
      const mln_delivery* delivery = MLN_STRIDED_AT(
        mln_delivery, view.deliveries, view.delivery_size, index
      );
      if (delivery->type != MLN_DELIVERY_TYPE_RESOURCE_REQUEST) continue;
      const mln_resource_request handle =
        delivery->payload.resource_request.handle;
      if (mln_delivery_batch_take(batch, index, NULL) != MLN_STATUS_OK) {
        continue;
      }
      const mln_resource_response response = style_response();
      if (
        mln_resource_request_complete(handle, &response, NULL) == MLN_STATUS_OK
      ) {
        atomic_fetch_add(&wake->completed, 1);
      } else {
        mln_resource_request_release(handle);
      }
    }
    mln_delivery_batch_release(batch);
    batch = MLN_HANDLE_NULL;
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
  const mln_resource_provider provider = queue_provider(wake.queue, NULL);
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

// MapLibre may cancel a queued request before the host drains it. The host
// still takes the handle, sees the cancellation, and registers nothing.
static void a_request_cancelled_before_the_drain_reports_it(void) {
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

  const mln_resource_request_delivery* request = NULL;
  const mln_delivery_batch batch = await_request(queue.handle, NULL, &request);
  mln_resource_request handle = request->handle;
  MLN_TEST_OK(mln_delivery_batch_take(batch, 0, NULL));
  mln_delivery_batch_release(batch);
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

// Replacing a queue provider retires it after its last request delivery.
static void retirement_follows_the_last_request(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  int first = 0;
  int second = 0;
  mln_resource_provider provider = queue_provider(queue.handle, &first);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://retired.json"));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&queue.wakes, 1));

  provider = queue_provider(queue.handle, &second);
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));

  mln_test_delivery_drain drain;
  MLN_TEST_OK(mln_test_delivery_drain_into(queue.handle, &drain));
  const mln_delivery_batch_view view = drain.view;
  TEST_ASSERT_EQUAL_size_t(3, view.delivery_count);
  const mln_delivery* request = mln_test_delivery_at(&view, 0);
  TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_RESOURCE_REQUEST, request->type);
  TEST_ASSERT_EQUAL_PTR(&first, request->user_data);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_DELIVERY_TYPE_RETIRED, mln_test_delivery_at(&view, 1)->type
  );
  TEST_ASSERT_EQUAL_PTR(&first, mln_test_delivery_at(&view, 1)->user_data);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_DELIVERY_TYPE_RETIRED, mln_test_delivery_at(&view, 2)->type
  );
  TEST_ASSERT_EQUAL_PTR(&second, mln_test_delivery_at(&view, 2)->user_data);
  mln_delivery_batch_release(drain.batch);

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

// Releasing a queue that holds an undrained request releases its handle,
// which fails the request instead of leaving its load waiting.
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
  mln_resource_provider provider = queue_provider(MLN_HANDLE_NULL, NULL);
  MLN_TEST_INVALID(mln_test_set_resource_provider(runtime, &provider));
  provider = queue_provider(queue.handle, NULL);
  provider.release_user_data = mln_test_delivery_count_wake_release;
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
    {.kinds = MLN_RESOURCE_KIND_MASK_ALL,
     .url = MLN_BUFFER_LITERAL("custom://first")},
    {.kinds = MLN_RESOURCE_KIND_MASK_ALL,
     .flags = 1U << 31U,
     .url = MLN_BUFFER_LITERAL("**")},
  };
  MLN_TEST_INVALID(submit_routes(runtime, routes, 2, &diagnostic));
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "route 1"));
  const mln_resource_route unknown_kind = {
    .kinds = 1U << 31U, .url = MLN_BUFFER_LITERAL("**")
  };
  MLN_TEST_INVALID(submit_routes(runtime, &unknown_kind, 1, NULL));
  mln_resource_route zero_kinds = mln_resource_route_default();
  zero_kinds.kinds = 0;
  zero_kinds.url = (mln_buffer_view)MLN_BUFFER_LITERAL("**");
  MLN_TEST_INVALID(submit_routes(runtime, &zero_kinds, 1, NULL));
  const mln_resource_route null_url = {
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .url = (mln_buffer_view){.data = NULL, .size = 1}
  };
  MLN_TEST_INVALID(submit_routes(runtime, &null_url, 1, NULL));
  MLN_TEST_INVALID(submit_routes(runtime, NULL, 1, NULL));
  MLN_TEST_OK(submit_routes(runtime, routes, 1, NULL));

  // A glob may hold 4096 tokens, and each ? is one. An exact URL has no limit.
  static char long_url[4097];
  memset(long_url, '?', sizeof(long_url));
  mln_resource_route long_route = {
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
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
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
    .flags = MLN_URL_MATCH_GLOB,
    .url = {.data = escapes_and_run, .size = sizeof(escapes_and_run)},
  };
  MLN_TEST_OK(submit_routes(runtime, &boundary_route, 1, NULL));
  mln_test_destroy_runtime(runtime);
}

#if !defined(__EMSCRIPTEN__)

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
    {.kinds = MLN_RESOURCE_KIND_MASK_TILE,
     .flags = MLN_URL_MATCH_GLOB,
     .url = MLN_BUFFER_LITERAL("**")},
    {.kinds = MLN_RESOURCE_KIND_MASK_STYLE,
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
    .kinds = MLN_RESOURCE_KIND_MASK_ALL,
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
  const mln_resource_request_delivery* request = NULL;
  const mln_delivery_batch batch = await_request(queue.handle, NULL, &request);
  TEST_ASSERT_EQUAL_STRING(
    "custom://routed/style.json", request->request->requested_url
  );
  const mln_resource_request handle = request->handle;
  MLN_TEST_OK(mln_delivery_batch_take(batch, 0, NULL));
  mln_delivery_batch_release(batch);
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
  void* user_data, uint32_t kind, const char* url,
  mln_resource_transform_request request
) {
  (void)kind;
  (void)url;
  const char* target = user_data;
  MLN_TEST_OK(mln_resource_transform_request_complete(
    request, mln_buffer_view_from_cstr(target), NULL
  ));
}

// The host can decline after taking a queued request. The resource transform
// still applies, and the request never visits the provider a second time.
static void a_drained_request_can_pass_through_once(void) {
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
  const mln_resource_request_delivery* request = NULL;
  const mln_delivery_batch batch = await_request(queue.handle, NULL, &request);
  const mln_resource_request handle = request->handle;
  MLN_TEST_OK(mln_delivery_batch_take(batch, 0, NULL));
  mln_delivery_batch_release(batch);
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
  RUN_TEST(the_host_completes_a_drained_request);
  RUN_TEST(an_untaken_request_fails);
  RUN_TEST(a_request_completed_before_handle_returns_loads);
  RUN_TEST(a_request_cancelled_before_the_drain_reports_it);
  RUN_TEST(retirement_follows_the_last_request);
  RUN_TEST(a_released_queue_fails_requests);
  RUN_TEST(releasing_a_queue_fails_its_pending_requests);
  RUN_TEST(a_queue_provider_rejects_an_invalid_shape);
  RUN_TEST(invalid_routes_are_rejected);
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(routes_filter_a_callback_provider);
  RUN_TEST(routes_filter_a_queue_provider);
  RUN_TEST(a_drained_request_can_pass_through_once);
#endif
}
