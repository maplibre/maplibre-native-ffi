// Queue-mode resource providers: copied requests that the host completes after
// draining, untaken and cancelled requests, retirement, and requests that a
// released queue passes through.

#include <string.h>

#include "support/delivery.h"
#include "support/test_support.h"

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
    sizeof(mln_resource_request), request->request->size
  );
  TEST_ASSERT_EQUAL_STRING(style_url, request->request->requested_url);
  TEST_ASSERT_EQUAL_UINT32(MLN_RESOURCE_KIND_STYLE, request->request->kind);
  const mln_resource_request_handle handle = request->handle;
  MLN_TEST_OK(mln_delivery_batch_take(batch, 0, NULL));
  mln_delivery_batch_release(batch);

  const mln_resource_response response = style_response();
  MLN_TEST_OK(mln_resource_request_complete(handle, &response, NULL));
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
  while (mln_delivery_queue_drain(wake->queue, &batch, NULL) == MLN_STATUS_OK) {
    mln_delivery_batch_view view = {.size = sizeof(view)};
    if (mln_delivery_batch_get(batch, &view, NULL) == MLN_STATUS_OK) {
      for (size_t index = 0; index < view.delivery_count; index += 1) {
        const mln_delivery* delivery =
          (const mln_delivery*)((const unsigned char*)view.deliveries +
                                index * view.delivery_size);
        if (delivery->type != MLN_DELIVERY_TYPE_RESOURCE_REQUEST) continue;
        const mln_resource_request_handle handle =
          delivery->payload.resource_request.handle;
        if (mln_delivery_batch_take(batch, index, NULL) != MLN_STATUS_OK) {
          continue;
        }
        const mln_resource_response response = style_response();
        if (
          mln_resource_request_complete(handle, &response, NULL) ==
          MLN_STATUS_OK
        ) {
          atomic_fetch_add(&wake->completed, 1);
        }
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
           *(const mln_resource_request_handle*)context, &cancelled, NULL
         ) == MLN_STATUS_OK &&
         cancelled;
}

static void ignore_cancel(void* user_data) { (void)user_data; }

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
  mln_resource_request_handle handle = request->handle;
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

  mln_delivery_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_delivery_queue_drain(queue.handle, &batch, NULL));
  const mln_delivery_batch_view view = mln_test_delivery_view(batch);
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
  mln_delivery_batch_release(batch);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

// A queue the host released refuses each request, which passes through to
// native loading.
static void a_released_queue_passes_requests_through(void) {
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
  TEST_ASSERT_NOT_NULL(strstr(message, "declined this request"));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&queue.wakes));

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

MLN_TEST_GROUP {
  RUN_TEST(the_host_completes_a_drained_request);
  RUN_TEST(an_untaken_request_fails);
  RUN_TEST(a_request_completed_before_handle_returns_loads);
  RUN_TEST(a_request_cancelled_before_the_drain_reports_it);
  RUN_TEST(retirement_follows_the_last_request);
  RUN_TEST(a_released_queue_passes_requests_through);
  RUN_TEST(a_queue_provider_rejects_an_invalid_shape);
}
