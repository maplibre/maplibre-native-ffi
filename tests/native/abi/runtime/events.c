// Event delivery ownership, ordering, and subscription masks.

#include "support/test_support.h"

static const uint64_t unknown_mask_bit = UINT64_C(1) << 63U;

static const mln_runtime_event* batch_event(
  const mln_event_batch_view* batch, size_t index
) {
  return (const mln_runtime_event*)((const char*)batch->events +
                                    (index * batch->event_size));
}

typedef struct foreign_thread_probe {
  mln_runtime runtime;
  mln_map map;
  mln_status drain_status;
  mln_status runtime_mask_status;
  mln_status map_mask_status;
} foreign_thread_probe;

static void call_event_api_from_a_foreign_thread(void* argument) {
  foreign_thread_probe* probe = argument;
  mln_delivery_batch batch = MLN_HANDLE_NULL;
  mln_event_batch_view view = {.size = sizeof(view)};
  probe->drain_status =
    mln_test_drain_event_delivery(probe->runtime, &batch, &view, NULL);
  mln_delivery_batch_release(batch);
  probe->runtime_mask_status = mln_runtime_set_event_mask(
    probe->runtime, MLN_RUNTIME_EVENT_MASK_ALL, NULL
  );
  probe->map_mask_status =
    mln_test_map_set_event_mask(probe->map, MLN_RUNTIME_EVENT_MASK_ALL);
}

// Map creation rejects a mask the setter rejects, so one value is not accepted
// at creation and then refused on the way back through a read-modify-write.
static void options_reject_unknown_event_mask_bits(void) {
  const uint64_t unknown = UINT64_C(1) << 40U;

  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options map_options = mln_map_options_default();
  map_options.event_mask = MLN_RUNTIME_EVENT_MASK_ALL | unknown;
  mln_map bad_map = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_test_map_create_status(runtime, &map_options, &bad_map));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, bad_map);

  // The same value the setter rejects, so the two agree.
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_INVALID(
    mln_test_map_set_event_mask(map, MLN_RUNTIME_EVENT_MASK_ALL | unknown)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Clearing one bit leaves every other type arriving, which separates
// suppression from a broken producer.
static void clearing_one_type_leaves_the_others_arriving(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_MASK_ALL &
           ~(uint64_t)MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED
  ));
  mln_test_drain_all(runtime);

  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  TEST_ASSERT_EQUAL_size_t(
    0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
  );

  MLN_TEST_OK(mln_test_map_set_event_mask(map, MLN_RUNTIME_EVENT_MASK_ALL));
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_GREATER_THAN_size_t(
    0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Closing a source prevents future publication without rewriting history that
// is already queued.
static void queued_events_outlive_the_map_that_produced_them(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_test_destroy_map(map);

  mln_delivery_batch batch = MLN_HANDLE_NULL;
  mln_event_batch_view view = {.size = sizeof(view)};
  MLN_TEST_OK(mln_test_drain_event_delivery(runtime, &batch, &view, NULL));
  TEST_ASSERT_GREATER_THAN_size_t(0, view.event_count);

  size_t map_sourced = 0;
  for (size_t index = 0; index < view.event_count; index += 1) {
    const mln_runtime_event* event =
      (const mln_runtime_event*)((const char*)view.events +
                                 (index * view.event_size));
    if (event->source_type == MLN_RUNTIME_EVENT_SOURCE_MAP) {
      TEST_ASSERT_EQUAL_UINT64(map, event->source);
      map_sourced += 1;
    }
    if (event->message.size != 0) TEST_ASSERT_NOT_NULL(event->message.data);
  }
  TEST_ASSERT_GREATER_THAN_size_t(0, map_sourced);

  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_GREATER_THAN_size_t(0, view.event_count);
  mln_delivery_batch_release(batch);
}

// Message views retain each event's bytes until the delivery batch is released.
static void each_event_carries_its_message(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map first = mln_test_create_map(runtime);
  mln_map second = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  MLN_TEST_OK(
    mln_test_map_set_style_json(first, MLN_BUFFER_LITERAL("{\"version\":"))
  );
  MLN_TEST_OK(
    mln_test_map_set_style_json(second, MLN_BUFFER_LITERAL("not json at all"))
  );
  MLN_TEST_OK(mln_test_map_fence(second));

  mln_delivery_batch delivery = MLN_HANDLE_NULL;
  mln_event_batch_view batch = {.size = sizeof(batch)};
  MLN_TEST_OK(mln_test_drain_event_delivery(runtime, &delivery, &batch, NULL));

  size_t failures = 0;
  mln_buffer_view retained[2] = {0};
  char copied[2][512] = {0};
  for (size_t index = 0; index < batch.event_count; index += 1) {
    const mln_runtime_event* event = batch_event(&batch, index);
    if (event->type != MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
      continue;
    }
    TEST_ASSERT_GREATER_THAN_UINT32(0, event->message.size);
    TEST_ASSERT_NOT_NULL(event->message.data);
    TEST_ASSERT_LESS_THAN_size_t(2, failures);
    TEST_ASSERT_LESS_OR_EQUAL_size_t(sizeof(copied[0]), event->message.size);
    retained[failures] = event->message;
    memcpy(copied[failures], event->message.data, event->message.size);
    failures += 1;
  }
  TEST_ASSERT_EQUAL_size_t(2, failures);

  mln_test_destroy_map(second);
  mln_test_destroy_map(first);
  mln_test_destroy_runtime(runtime);
  for (size_t index = 0; index < failures; index += 1) {
    TEST_ASSERT_EQUAL_MEMORY(
      copied[index], retained[index].data, retained[index].size
    );
  }
  mln_delivery_batch_release(delivery);
}

// The masks change from any thread, and the runtime setter rejects unknown
// bits but keeps the other group's.
static void event_masks_change_from_any_thread_and_keep_foreign_bits(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_test_drain_all(runtime);
  {
    foreign_thread_probe probe = {.runtime = runtime, .map = map};
    mln_test_thread* thread =
      mln_test_thread_start(call_event_api_from_a_foreign_thread, &probe);
    mln_test_thread_join(thread);

    // The queue was drained above, so the accepted foreign drain finds it
    // empty.
    MLN_TEST_STATUS(MLN_STATUS_NOT_READY, probe.drain_status);
    MLN_TEST_OK(probe.runtime_mask_status);
    MLN_TEST_OK(probe.map_mask_status);

    // Both setters accept MLN_RUNTIME_EVENT_MASK_ALL, and the map reports it
    // back whole.
    uint64_t map_mask = 0;
    MLN_TEST_OK(mln_test_map_get_event_mask(map, &map_mask));
    TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_ALL, map_mask);
  }
  // The setter reads only its own group's bits but stores the whole value, so
  // a host that reads a mask, sets one bit, and writes it back keeps every
  // other.
  {
    MLN_TEST_INVALID(
      mln_runtime_set_event_mask(runtime, unknown_mask_bit, NULL)
    );

    // One in-group bit for the other source kind, which the setter accepts and
    // reports back unchanged.
    uint64_t runtime_mask = 0;
    MLN_TEST_OK(mln_runtime_set_event_mask(
      runtime, MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED, NULL
    ));
    MLN_TEST_OK(mln_runtime_get_event_mask(runtime, &runtime_mask, NULL));
    TEST_ASSERT_EQUAL_UINT64(
      MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED, runtime_mask
    );

    MLN_TEST_INVALID(mln_runtime_get_event_mask(runtime, NULL, NULL));
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct event_completion_order {
  mln_delivery_queue queue;
  mln_map map;
  uint64_t event_generation;
  uint64_t completion_generation;
  bool style_loaded;
  bool completed;
} event_completion_order;

static bool drain_until_completion(void* context) {
  event_completion_order* order = context;
  mln_delivery_batch batch = MLN_HANDLE_NULL;
  mln_delivery_batch_view view = {.size = sizeof(view)};
  const mln_status status =
    mln_delivery_queue_drain(order->queue, &batch, &view, NULL);
  if (status == MLN_STATUS_NOT_READY) return false;
  MLN_TEST_OK(status);
  for (size_t item = 0; item < view.delivery_count; ++item) {
    const mln_delivery* delivery =
      (const mln_delivery*)((const char*)view.deliveries +
                            item * view.delivery_size);
    if (delivery->type == MLN_DELIVERY_TYPE_RUNTIME_EVENTS) {
      const mln_event_batch_view* events =
        delivery->payload.runtime_events.events;
      for (size_t index = 0; index < events->event_count; ++index) {
        const mln_runtime_event* event =
          (const mln_runtime_event*)((const char*)events->events +
                                     index * events->event_size);
        TEST_ASSERT_FALSE(order->completed);
        TEST_ASSERT_EQUAL_UINT64(order->map, event->source);
        if (event->type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) {
          order->style_loaded = true;
          order->event_generation = event->generation;
          mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
          MLN_TEST_OK(mln_map_get_snapshot(order->map, &snapshot, NULL));
          TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
            event->generation, snapshot.generation
          );
        }
      }
    } else if (delivery->type == MLN_DELIVERY_TYPE_COMPLETION) {
      TEST_ASSERT_TRUE(order->style_loaded);
      MLN_TEST_OK(delivery->payload.completion.status);
      order->completed = true;
      order->completion_generation = delivery->payload.completion.generation;
    }
  }
  mln_delivery_batch_release(batch);
  return order->completed;
}

static void events_and_their_completion_share_fifo_order(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);
  event_completion_order order = {
    .queue = mln_test_event_queue(runtime), .map = map
  };
  const mln_completion completion = {
    .size = sizeof(completion), .queue = order.queue
  };
  MLN_TEST_OK(mln_map_set_style_json(
    map, mln_test_background_style_json, &completion, NULL
  ));
  TEST_ASSERT_TRUE(mln_test_await(
    drain_until_completion, &order, mln_test_deadline_default(),
    "events followed by their completion"
  ));
  TEST_ASSERT_EQUAL_UINT64(order.event_generation, order.completion_generation);
  mln_test_completion clear = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_runtime_clear_event_handler(runtime, &clear.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_completion_settle(&clear));
  mln_delivery_batch retired = MLN_HANDLE_NULL;
  mln_delivery_batch_view retired_view = {.size = sizeof(retired_view)};
  MLN_TEST_OK(
    mln_delivery_queue_drain(order.queue, &retired, &retired_view, NULL)
  );
  TEST_ASSERT_EQUAL_size_t(1, retired_view.delivery_count);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_DELIVERY_TYPE_RETIRED, retired_view.deliveries[0].type
  );
  mln_delivery_batch_release(retired);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct callback_events {
  size_t calls;
  size_t releases;
  mln_runtime_event event;
  mln_status snapshot_status;
  uint64_t snapshot_generation;
} callback_events;

static void receive_style_event(
  void* context, const mln_event_batch_view* events
) {
  callback_events* received = context;
  received->calls += events->event_count;
  if (events->event_count != 0) {
    received->event = events->events[0];
    mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
    received->snapshot_status =
      mln_map_get_snapshot(received->event.source, &snapshot, NULL);
    received->snapshot_generation = snapshot.generation;
  }
}

static void release_event_context(void* context) {
  callback_events* received = context;
  received->releases += 1;
}

// A callback host copies events during the call and owns rejected contexts.
// Clear retires accepted callback state before its completion.
static void callback_event_state_retires_after_its_last_event(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(
    mln_test_map_set_event_mask(map, MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED)
  );
  callback_events received = {0};
  mln_runtime_event_handler handler = {
    .size = sizeof(handler),
    .callback = receive_style_event,
    .user_data = &received,
    .release_user_data = release_event_context,
  };
  handler.size = 0;
  MLN_TEST_INVALID(
    mln_runtime_set_event_handler(runtime, &handler, NULL, NULL)
  );
  TEST_ASSERT_EQUAL_size_t(0, received.releases);
  handler.size = sizeof(handler);
  MLN_TEST_AWAIT_OK(mln_runtime_set_event_handler(
    runtime, &handler, &completion.descriptor, NULL
  ));
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  MLN_TEST_AWAIT_OK(
    mln_runtime_clear_event_handler(runtime, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_size_t(1, received.calls);
  TEST_ASSERT_EQUAL_size_t(1, received.releases);
  MLN_TEST_OK(received.snapshot_status);
  TEST_ASSERT_EQUAL_UINT64(map, received.event.source);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RUNTIME_EVENT_MAP_STYLE_LOADED, received.event.type
  );
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
    received.event.generation, received.snapshot_generation
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(callback_event_state_retires_after_its_last_event);
  RUN_TEST(events_and_their_completion_share_fifo_order);
  RUN_TEST(event_masks_change_from_any_thread_and_keep_foreign_bits);
  RUN_TEST(options_reject_unknown_event_mask_bits);
  RUN_TEST(clearing_one_type_leaves_the_others_arriving);
  RUN_TEST(queued_events_outlive_the_map_that_produced_them);
  RUN_TEST(each_event_carries_its_message);
}
