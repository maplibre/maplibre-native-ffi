// Event delivery ownership, ordering, and subscription masks.

#include "support/test_support.h"

static const uint64_t unknown_mask_bit = UINT64_C(1) << 63U;

typedef struct foreign_thread_probe {
  mln_runtime runtime;
  mln_map map;
  mln_status drain_status;
  mln_status runtime_mask_status;
  mln_status map_mask_status;
} foreign_thread_probe;

static void call_event_api_from_a_foreign_thread(void* argument) {
  foreign_thread_probe* probe = argument;
  probe->drain_status = mln_test_dispatch_events(probe->runtime, NULL, NULL);
  probe->runtime_mask_status = mln_runtime_set_event_mask(
    probe->runtime, MLN_RUNTIME_EVENT_FLAG_ALL, NULL
  );
  probe->map_mask_status =
    mln_test_map_set_event_mask(probe->map, MLN_RUNTIME_EVENT_FLAG_ALL);
}

// Map creation rejects a mask the setter rejects, so one value is not accepted
// at creation and then refused on the way back through a read-modify-write.
static void options_reject_unknown_event_mask_bits(void) {
  const uint64_t unknown = UINT64_C(1) << 40U;

  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options map_options = mln_test_map_options();
  map_options.fields |= MLN_MAP_OPTION_EVENT_MASK;
  map_options.event_mask = MLN_RUNTIME_EVENT_FLAG_ALL | unknown;
  mln_map bad_map = MLN_HANDLE_NULL;
  MLN_TEST_INVALID(mln_test_map_create_status(runtime, &map_options, &bad_map));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, bad_map);

  // The same value the setter rejects, so the two agree.
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_INVALID(
    mln_test_map_set_event_mask(map, MLN_RUNTIME_EVENT_FLAG_ALL | unknown)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An absent event mask selects every event type, and native leaves the member
// unread. A present mask of zero selects none.
static void options_without_an_event_mask_select_every_event(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options map_options = mln_test_map_options();
  map_options.event_mask = UINT64_C(1) << 40U;
  mln_map every = mln_test_create_map_with_options(runtime, &map_options);
  uint64_t mask = 0;
  MLN_TEST_OK(mln_test_map_get_event_mask(every, &mask));
  TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_FLAG_ALL, mask);

  map_options.fields = MLN_MAP_OPTION_EVENT_MASK;
  map_options.event_mask = MLN_RUNTIME_EVENT_FLAG_NONE;
  mln_map none = mln_test_create_map_with_options(runtime, &map_options);
  MLN_TEST_OK(mln_test_map_get_event_mask(none, &mask));
  TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_FLAG_NONE, mask);

  mln_test_destroy_map(none);
  mln_test_destroy_map(every);
  mln_test_destroy_runtime(runtime);
}

// Clearing one bit leaves every other type arriving, which separates
// suppression from a broken producer.
// The fixture runtime sets its handler with a discarded command just before
// this case creates the map. The handler still receives the camera event that
// the map's construction raises.
static void a_handler_accepted_before_a_map_receives_its_first_events(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_runtime_event event;
  TEST_ASSERT_TRUE(mln_test_await_event(
    runtime, MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE, map, &event, NULL, 0
  ));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void clearing_one_type_leaves_the_others_arriving(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_FLAG_ALL &
           ~(uint64_t)MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED
  ));
  mln_test_drain_all(runtime);

  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  TEST_ASSERT_EQUAL_size_t(
    0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
  );

  MLN_TEST_OK(mln_test_map_set_event_mask(map, MLN_RUNTIME_EVENT_FLAG_ALL));
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_GREATER_THAN_size_t(
    0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct map_sourced_count {
  mln_map map;
  size_t events;
  size_t map_sourced;
  size_t wrong_source;
  size_t missing_message;
} map_sourced_count;

static void count_map_sourced(const mln_runtime_event* event, void* context) {
  map_sourced_count* count = context;
  count->events += 1;
  if (event->source_type == MLN_RUNTIME_EVENT_SOURCE_MAP) {
    if (event->source != count->map) count->wrong_source += 1;
    count->map_sourced += 1;
  }
  if (event->message.size != 0 && event->message.data == NULL) {
    count->missing_message += 1;
  }
}

// Closing a source prevents future publication without rewriting history that
// is already queued.
static void queued_events_outlive_the_map_that_produced_them(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_test_destroy_map(map);

  map_sourced_count count = {.map = map};
  MLN_TEST_OK(mln_test_dispatch_events(runtime, count_map_sourced, &count));
  TEST_ASSERT_GREATER_THAN_size_t(0, count.events);
  TEST_ASSERT_GREATER_THAN_size_t(0, count.map_sourced);
  TEST_ASSERT_EQUAL_size_t(0, count.wrong_source);
  TEST_ASSERT_EQUAL_size_t(0, count.missing_message);

  mln_test_destroy_runtime(runtime);
}

typedef struct loading_failures {
  size_t count;
  size_t without_message;
} loading_failures;

static void count_loading_failure(
  const mln_runtime_event* event, void* context
) {
  loading_failures* failures = context;
  if (event->type != MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) return;
  failures->count += 1;
  if (event->message.size == 0 || event->message.data == NULL) {
    failures->without_message += 1;
  }
}

// Each failure event carries its own message.
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

  loading_failures failures = {0};
  MLN_TEST_OK(
    mln_test_dispatch_events(runtime, count_loading_failure, &failures)
  );
  TEST_ASSERT_EQUAL_size_t(2, failures.count);
  TEST_ASSERT_EQUAL_size_t(0, failures.without_message);

  mln_test_destroy_map(second);
  mln_test_destroy_map(first);
  mln_test_destroy_runtime(runtime);
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

    // Both setters accept MLN_RUNTIME_EVENT_FLAG_ALL, and the map reports it
    // back whole.
    uint64_t map_mask = 0;
    MLN_TEST_OK(mln_test_map_get_event_mask(map, &map_mask));
    TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_FLAG_ALL, map_mask);
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
      runtime, MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED, NULL
    ));
    MLN_TEST_OK(mln_runtime_get_event_mask(runtime, &runtime_mask, NULL));
    TEST_ASSERT_EQUAL_UINT64(
      MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED, runtime_mask
    );

    MLN_TEST_INVALID(mln_runtime_get_event_mask(runtime, NULL, NULL));
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct event_completion_order {
  mln_runtime runtime;
  mln_map map;
  uint64_t event_generation;
  uint64_t completion_generation;
  mln_status completion_status;
  size_t events_after_completion;
  size_t foreign_events;
  size_t stale_snapshots;
  bool style_loaded_first;
  bool style_loaded;
  bool completed;
} event_completion_order;

// Records rather than asserts: a failing assertion must not unwind through
// the dispatch that calls this.
static void order_event(const mln_runtime_event* event, void* context) {
  event_completion_order* order = context;
  if (order->completed) order->events_after_completion += 1;
  if (event->source != order->map) order->foreign_events += 1;
  if (event->type != MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) return;
  order->style_loaded = true;
  order->event_generation = event->generation;
  mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
  if (
    mln_map_get_snapshot(order->map, &snapshot, NULL) != MLN_STATUS_OK ||
    snapshot.generation < event->generation
  ) {
    order->stale_snapshots += 1;
  }
}

static void order_completion(
  void* user_data, const mln_completion_result* result
) {
  event_completion_order* order = user_data;
  order->style_loaded_first = order->style_loaded;
  order->completion_status = result->status;
  order->completion_generation = result->generation;
  order->completed = true;
}

static bool dispatch_until_completion(void* context) {
  event_completion_order* order = context;
  const mln_status status =
    mln_test_dispatch_events(order->runtime, order_event, order);
  if (status == MLN_STATUS_NOT_READY) return false;
  MLN_TEST_OK(status);
  return order->completed;
}

static void events_and_their_completion_share_fifo_order(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);
  event_completion_order order = {.runtime = runtime, .map = map};
  const mln_completion completion = {
    .size = sizeof(completion),
    .callback = order_completion,
    .user_data = &order,
    .queue = mln_test_event_queue(runtime),
  };
  MLN_TEST_OK(mln_map_set_style_json(
    map, mln_test_background_style_json, &completion, NULL
  ));
  TEST_ASSERT_TRUE(mln_test_await(
    dispatch_until_completion, &order, mln_test_deadline_default(),
    "events followed by their completion"
  ));
  TEST_ASSERT_TRUE(order.style_loaded_first);
  MLN_TEST_OK(order.completion_status);
  TEST_ASSERT_EQUAL_size_t(0, order.events_after_completion);
  TEST_ASSERT_EQUAL_size_t(0, order.foreign_events);
  TEST_ASSERT_EQUAL_size_t(0, order.stale_snapshots);
  TEST_ASSERT_EQUAL_UINT64(order.event_generation, order.completion_generation);
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
    mln_test_map_set_event_mask(map, MLN_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED)
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
  RUN_TEST(a_handler_accepted_before_a_map_receives_its_first_events);
  RUN_TEST(callback_event_state_retires_after_its_last_event);
  RUN_TEST(events_and_their_completion_share_fifo_order);
  RUN_TEST(event_masks_change_from_any_thread_and_keep_foreign_bits);
  RUN_TEST(options_reject_unknown_event_mask_bits);
  RUN_TEST(options_without_an_event_mask_select_every_event);
  RUN_TEST(clearing_one_type_leaves_the_others_arriving);
  RUN_TEST(queued_events_outlive_the_map_that_produced_them);
  RUN_TEST(each_event_carries_its_message);
}
