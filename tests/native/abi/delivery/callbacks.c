// Queue-mode callback registrations beyond completions, logs, and providers:
// a camera command's end handler, a custom source's tile fetches and cancels,
// and a resource request's cancel handler. Each delivers a copy of every call
// for its host token and then retires once, and a queue registration that
// also carries a callback is rejected.

#include "support/delivery.h"
#include "support/style.h"
#include "support/test_support.h"

// The deliveries of one host token, in queue order, gathered across batches.
enum { seen_capacity = 16 };

typedef struct seen_deliveries {
  void* token;
  size_t count;
  uint32_t types[seen_capacity];
  mln_delivery_payload payloads[seen_capacity];
  mln_camera_transition_end ends[seen_capacity];
} seen_deliveries;

static void collect(mln_delivery_queue queue, seen_deliveries* seen) {
  mln_test_delivery_drain drain;
  while (mln_test_delivery_drain_into(queue, &drain) == MLN_STATUS_OK) {
    for (size_t index = 0; index < drain.view.delivery_count; index += 1) {
      const mln_delivery* delivery = mln_test_delivery_at(&drain.view, index);
      if (delivery->user_data != seen->token) continue;
      TEST_ASSERT_TRUE(seen->count < seen_capacity);
      const size_t slot = seen->count++;
      seen->types[slot] = delivery->type;
      seen->payloads[slot] = delivery->payload;
      // The end is owned by the batch, so it is copied before release.
      if (delivery->type == MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END) {
        seen->ends[slot] = *delivery->payload.camera_transition_end.end;
      }
    }
    mln_delivery_batch_release(drain.batch);
  }
}

typedef struct seen_target {
  mln_delivery_queue queue;
  seen_deliveries* seen;
  uint32_t type;
} seen_target;

static bool saw_type(void* context) {
  seen_target* target = context;
  collect(target->queue, target->seen);
  for (size_t index = 0; index < target->seen->count; index += 1) {
    if (target->seen->types[index] == target->type) return true;
  }
  return false;
}

// Waits until a delivery of type has reached seen.
static void await_type(
  mln_delivery_queue queue, seen_deliveries* seen, uint32_t type
) {
  seen_target target = {.queue = queue, .seen = seen, .type = type};
  TEST_ASSERT_TRUE(mln_test_await(
    saw_type, &target, mln_test_deadline_default(), "a queued delivery"
  ));
}

// Callbacks that a rejected queue registration carries, which never run.
static void unused_end(void* user_data, const mln_camera_transition_end* end) {
  (void)user_data;
  (void)end;
}

static void unused_tile(void* user_data, mln_canonical_tile_id tile_id) {
  (void)user_data;
  (void)tile_id;
}

static void unused_cancel(void* user_data, mln_resource_request handle) {
  (void)user_data;
  (void)handle;
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
    .user_data = token,
    .queue = queue,
  };
  return update;
}

// A cancelled ease queues its end and then its retirement. A command that
// names a queue and a callback, or a released queue, queues nothing.
static void a_camera_end_handler_delivers_its_end_then_retires(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  int token = 0;

  mln_camera_update update = queued_ease(queue.handle, &token);
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_camera_transition(map, 7, &completion.descriptor, NULL)
  );
  seen_deliveries seen = {.token = &token};
  await_type(queue.handle, &seen, MLN_DELIVERY_TYPE_RETIRED);
  TEST_ASSERT_EQUAL_size_t(2, seen.count);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_DELIVERY_TYPE_CAMERA_TRANSITION_END, seen.types[0]
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, seen.ends[0].outcome
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(0, seen.ends[0].generation);

  update.animation.end_handler.callback = unused_end;
  MLN_TEST_INVALID(mln_map_update_camera(map, &update, NULL, NULL));
  update.animation.end_handler.callback = NULL;

  mln_test_delivery_queue_release(&queue);
  update.animation.end_handler.queue = queue.handle;
  MLN_TEST_INVALID_STATE(mln_map_update_camera(map, &update, NULL, NULL));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct queued_source {
  mln_delivery_queue queue;
  seen_deliveries* seen;
  size_t fetches;
  size_t cancels;
} queued_source;

static bool source_reached(void* context) {
  queued_source* source = context;
  collect(source->queue, source->seen);
  size_t fetches = 0;
  size_t cancels = 0;
  for (size_t index = 0; index < source->seen->count; index += 1) {
    const uint32_t type = source->seen->types[index];
    fetches += type == MLN_DELIVERY_TYPE_CUSTOM_SOURCE_FETCH_TILE;
    cancels += type == MLN_DELIVERY_TYPE_CUSTOM_SOURCE_CANCEL_TILE;
  }
  return fetches >= source->fetches && cancels >= source->cancels;
}

static const mln_canonical_tile_id root_tile = {.z = 0, .x = 0, .y = 0};

// A drawn source queues the fetch of its root tile, an invalidation queues
// its cancel and a second fetch, and removing the source retires it.
static void a_custom_source_delivers_its_fetches_and_cancels_then_retires(
  void
) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  int token = 0;
  mln_test_load_style_and_wait(map, mln_test_background_style_json);

  mln_custom_geometry_source_options options = {
    .size = sizeof(mln_custom_geometry_source_options)
  };
  options.user_data = &token;
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

  seen_deliveries seen = {.token = &token};
  queued_source fetched = {.queue = queue.handle, .seen = &seen, .fetches = 1};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, source_reached, &fetched, "the queued fetch"
  ));
  const mln_canonical_tile_id first =
    seen.payloads[0].custom_source_fetch_tile.tile_id;
  TEST_ASSERT_EQUAL_UINT32(root_tile.z, first.z);

  MLN_TEST_AWAIT_OK(mln_map_invalidate_custom_geometry_source_tile(
    map, MLN_BUFFER_LITERAL("queued"), root_tile, &completion.descriptor, NULL
  ));
  queued_source refetched = {
    .queue = queue.handle, .seen = &seen, .fetches = 2, .cancels = 1
  };
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
  await_type(queue.handle, &seen, MLN_DELIVERY_TYPE_RETIRED);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_DELIVERY_TYPE_RETIRED, seen.types[seen.count - 1]
  );

  // A queue source carries no callbacks.
  options.fetch_tile = unused_tile;
  MLN_TEST_INVALID(mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL("refused"), &options, NULL, NULL
  ));
  options.fetch_tile = NULL;
  options.cancel_tile = unused_tile;
  MLN_TEST_INVALID(mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL("refused"), &options, NULL, NULL
  ));

  mln_custom_vector_source_options mvt = {
    .size = sizeof(mln_custom_vector_source_options)
  };
  mvt.queue = queue.handle;
  mvt.cancel_tile = unused_tile;
  MLN_TEST_INVALID(mln_map_add_custom_vector_source(
    map, MLN_BUFFER_LITERAL("refused"), &mvt, NULL, NULL
  ));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

static mln_resource_provider queue_provider(mln_delivery_queue queue) {
  return (mln_resource_provider){
    .size = sizeof(mln_resource_provider),
    .queue = queue,
  };
}

// A request that a style change discards queues its cancel, which names the
// request, and then the cancel registration retires.
static void a_request_cancel_handler_delivers_its_request_then_retires(void) {
  mln_test_delivery_queue queue;
  mln_test_delivery_queue_create(&queue);
  const mln_resource_provider provider = queue_provider(queue.handle);
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_OK(mln_test_set_resource_provider(runtime, &provider));
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_style_url(map, "custom://cancel-queued.json"));

  const mln_delivery* delivery = NULL;
  const mln_delivery_batch batch =
    mln_test_delivery_await_single(queue.handle, &delivery);
  TEST_ASSERT_EQUAL_UINT32(MLN_DELIVERY_TYPE_RESOURCE_REQUEST, delivery->type);
  const mln_resource_request handle = delivery->payload.resource_request.handle;
  MLN_TEST_OK(mln_delivery_batch_take(batch, 0, NULL));
  mln_delivery_batch_release(batch);

  int token = 0;
  mln_resource_request_cancel_handler handler = {
    .size = sizeof(mln_resource_request_cancel_handler),
    .user_data = &token,
    .queue = queue.handle,
  };
  handler.callback = unused_cancel;
  bool cancelled = false;
  MLN_TEST_INVALID(
    mln_resource_request_set_cancel_callback(handle, &handler, &cancelled, NULL)
  );
  handler.callback = NULL;
  MLN_TEST_OK(
    mln_resource_request_set_cancel_callback(handle, &handler, &cancelled, NULL)
  );
  TEST_ASSERT_FALSE(cancelled);

  // A new style discards the pending style request.
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_empty_style_json));
  seen_deliveries seen = {.token = &token};
  await_type(queue.handle, &seen, MLN_DELIVERY_TYPE_RETIRED);
  TEST_ASSERT_EQUAL_size_t(2, seen.count);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_DELIVERY_TYPE_RESOURCE_REQUEST_CANCEL, seen.types[0]
  );
  TEST_ASSERT_EQUAL_UINT64(
    handle, seen.payloads[0].resource_request_cancel.handle
  );
  mln_resource_request_release(handle);

  MLN_TEST_OK(mln_test_clear_resource_provider(runtime));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_delivery_queue_release(&queue);
}

MLN_TEST_GROUP {
  RUN_TEST(a_camera_end_handler_delivers_its_end_then_retires);
  RUN_TEST(a_custom_source_delivers_its_fetches_and_cancels_then_retires);
  RUN_TEST(a_request_cancel_handler_delivers_its_request_then_retires);
}
