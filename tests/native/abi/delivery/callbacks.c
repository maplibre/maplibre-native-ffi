// Queued callback registrations beyond completions, logs, and providers: a
// camera command's end handler, a custom source's tile fetches and cancels,
// and a resource request's cancel handler. Each runs every call in dispatch
// and then its release, and a queue never stands in for a callback.

#include "support/delivery.h"
#include "support/style.h"
#include "support/test_support.h"

// The tile and request handle of the calls this file's queues ran, by call
// log index.
static mln_canonical_tile_id seen_tiles[mln_test_max_calls];
static mln_resource_request seen_requests[mln_test_max_calls];

static void record_fetch(void* user_data, mln_canonical_tile_id tile_id) {
  if (mln_test_calls.count < mln_test_max_calls) {
    seen_tiles[mln_test_calls.count] = tile_id;
  }
  mln_test_record_call('f', user_data, MLN_STATUS_OK, 0);
}

static void record_cancel_tile(void* user_data, mln_canonical_tile_id tile_id) {
  if (mln_test_calls.count < mln_test_max_calls) {
    seen_tiles[mln_test_calls.count] = tile_id;
  }
  mln_test_record_call('x', user_data, MLN_STATUS_OK, 0);
}

static void record_end(void* user_data, const mln_camera_transition_end* end) {
  (void)end;
  mln_test_record_call('e', user_data, MLN_STATUS_OK, 0);
}

static size_t count_kind(char kind) {
  size_t total = 0;
  for (size_t index = 0;
       index < mln_test_calls.count && index < mln_test_max_calls; ++index) {
    total += mln_test_calls.calls[index].kind == kind;
  }
  return total;
}

static mln_camera_update queued_ease(mln_delivery_queue queue, void* token) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = 6;
  update.animation.fields =
    MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_TRANSITION_ID;
  update.animation.duration_ms = 60000;
  update.animation.transition_id = 7;
  update.animation.end_handler = (mln_camera_transition_handler){
    .callback = record_end,
    .user_data = token,
    .queue = queue,
  };
  return update;
}

// A handler that names a queue still needs its callback, and a released
// queue is refused. Neither command queues anything.
static void a_camera_end_handler_needs_a_callback_and_a_live_queue(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  int token = 0;

  mln_camera_update update = queued_ease(queue.handle, &token);
  update.animation.end_handler.callback = NULL;
  MLN_TEST_INVALID(mln_map_update_camera(map, &update, NULL, NULL));
  update.animation.end_handler.callback = record_end;

  mln_test_delivery_queue_release(&queue);
  MLN_TEST_INVALID_STATE(mln_map_update_camera(map, &update, NULL, NULL));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&queue.wakes));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct queued_source {
  mln_delivery_queue queue;
  size_t fetches;
  size_t cancels;
} queued_source;

static bool source_reached(void* context) {
  queued_source* source = context;
  (void)mln_delivery_queue_dispatch(source->queue, NULL);
  return count_kind('f') >= source->fetches &&
         count_kind('x') >= source->cancels;
}

static const mln_canonical_tile_id root_tile = {.z = 0, .x = 0, .y = 0};

// A drawn source queues the fetch of its root tile, an invalidation queues
// its cancel and a second fetch, and removing the source queues its release.
static void a_custom_source_runs_its_fetches_and_cancels_then_its_release(
  void
) {
  mln_test_calls_reset();
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  int token = 0;
  mln_test_load_style_and_wait(map, mln_test_background_style_json);

  mln_custom_geometry_source_options options = {
    .size = sizeof(mln_custom_geometry_source_options)
  };
  options.fetch_tile = record_fetch;
  options.cancel_tile = record_cancel_tile;
  options.user_data = &token;
  options.release_user_data = mln_test_record_release;
  options.queue = queue.handle;
  MLN_TEST_AWAIT_OK(mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL("queued"), &options, &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map,
    MLN_BUFFER_LITERAL(
      "{\"id\":\"queued-dots\",\"type\":\"circle\",\"source\":\"queued\"}"
    ),
    MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
  ));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  queued_source fetched = {.queue = queue.handle, .fetches = 1};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, source_reached, &fetched, "the queued fetch"
  ));
  TEST_ASSERT_EQUAL_CHAR('f', mln_test_call_at(0)->kind);
  TEST_ASSERT_EQUAL_PTR(&token, mln_test_call_at(0)->user_data);
  TEST_ASSERT_EQUAL_UINT32(root_tile.z, seen_tiles[0].z);

  MLN_TEST_AWAIT_OK(mln_map_invalidate_custom_geometry_source_tile(
    map, MLN_BUFFER_LITERAL("queued"), root_tile, &completion.descriptor, NULL
  ));
  queued_source refetched = {.queue = queue.handle, .fetches = 2, .cancels = 1};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, source_reached, &refetched, "the queued cancel and refetch"
  ));

  mln_test_render_fixture_destroy(&fixture);
  MLN_TEST_AWAIT_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("queued-dots"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_remove_style_source(
    map, MLN_BUFFER_LITERAL("queued"), &completion.descriptor, NULL
  ));
  const size_t before_release = mln_test_calls.count;
  mln_test_dispatch_until_calls(queue.handle, before_release + 1);
  TEST_ASSERT_EQUAL_CHAR('r', mln_test_call_at(mln_test_calls.count - 1)->kind);
  TEST_ASSERT_EQUAL_size_t(1, mln_test_count_calls('r', &token));

  // A queue does not stand in for fetch_tile.
  options.fetch_tile = NULL;
  options.cancel_tile = NULL;
  options.release_user_data = NULL;
  MLN_TEST_INVALID(mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL("refused"), &options, NULL, NULL
  ));
  mln_custom_vector_source_options mvt = {
    .size = sizeof(mln_custom_vector_source_options)
  };
  mvt.queue = queue.handle;
  MLN_TEST_INVALID(mln_map_add_custom_vector_source(
    map, MLN_BUFFER_LITERAL("refused"), &mvt, NULL, NULL
  ));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

// Keeps each request handle that a dispatched provider call receives.
static void keep_request(
  void* user_data, const mln_resource_request_info* request,
  mln_resource_request handle
) {
  (void)request;
  if (mln_test_calls.count < mln_test_max_calls) {
    seen_requests[mln_test_calls.count] = handle;
  }
  mln_test_record_call('p', user_data, MLN_STATUS_OK, 0);
}

static void record_request_cancel(
  void* user_data, mln_resource_request handle
) {
  if (mln_test_calls.count < mln_test_max_calls) {
    seen_requests[mln_test_calls.count] = handle;
  }
  mln_test_record_call('k', user_data, MLN_STATUS_OK, 0);
}

// A request that a style change discards queues its cancel, which names the
// request, and then the cancel registration's release.
static void a_request_cancel_handler_runs_its_cancel_then_its_release(void) {
  mln_test_calls_reset();
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_provider provider = {
    .size = sizeof(mln_resource_provider),
    .callback = keep_request,
    .queue = queue.handle,
  };
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://cancel-queued.json"));

  mln_test_dispatch_until_calls(queue.handle, 1);
  TEST_ASSERT_EQUAL_CHAR('p', mln_test_call_at(0)->kind);
  const mln_resource_request handle = seen_requests[0];

  int token = 0;
  mln_resource_request_cancel_handler handler = {
    .size = sizeof(mln_resource_request_cancel_handler),
    .user_data = &token,
    .release_user_data = mln_test_record_release,
    .queue = queue.handle,
  };
  bool cancelled = false;
  MLN_TEST_INVALID(
    mln_resource_request_set_cancel_callback(handle, &handler, &cancelled, NULL)
  );
  handler.callback = record_request_cancel;
  MLN_TEST_OK(
    mln_resource_request_set_cancel_callback(handle, &handler, &cancelled, NULL)
  );
  TEST_ASSERT_FALSE(cancelled);

  // A new style discards the pending style request.
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_empty_style_json));
  mln_test_dispatch_until_calls(queue.handle, 3);
  TEST_ASSERT_EQUAL_CHAR('k', mln_test_call_at(1)->kind);
  TEST_ASSERT_EQUAL_PTR(&token, mln_test_call_at(1)->user_data);
  TEST_ASSERT_EQUAL_UINT64(handle, seen_requests[1]);
  TEST_ASSERT_EQUAL_CHAR('r', mln_test_call_at(2)->kind);
  TEST_ASSERT_EQUAL_PTR(&token, mln_test_call_at(2)->user_data);
  mln_resource_request_release(handle);

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

MLN_TEST_GROUP {
  RUN_TEST(a_camera_end_handler_needs_a_callback_and_a_live_queue);
  RUN_TEST(a_custom_source_runs_its_fetches_and_cancels_then_its_release);
  RUN_TEST(a_request_cancel_handler_runs_its_cancel_then_its_release);
}
