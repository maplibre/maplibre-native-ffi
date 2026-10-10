// Raw C ABI coverage for the batch event drain and the two subscription masks:
// batch layout and lifetime, mask validation, and the suppression a cleared
// mask bit causes at push time.

#include "support/test_support.h"

static const uint64_t unknown_mask_bit = UINT64_C(1) << 63U;

static const mln_runtime_event* batch_event(
  const mln_test_event_batch* batch, size_t index
) {
  return (const mln_runtime_event*)((const char*)batch->events +
                                    (index * batch->event_size));
}

static mln_map_options map_options_with_event_mask(uint64_t mask) {
  mln_map_options options = mln_map_options_default();
  options.event_mask = mask;
  return options;
}

// An owned output must start null, and a retired runtime accepts no new drain.
static void a_drain_rejects_a_nonnull_output_or_a_stale_runtime(void) {
  mln_runtime runtime = mln_test_create_runtime();
  MLN_TEST_INVALID(mln_runtime_drain_events(runtime, NULL, NULL));
  mln_event_batch batch = UINT64_C(1);
  MLN_TEST_INVALID(mln_runtime_drain_events(runtime, &batch, NULL));

  mln_test_destroy_runtime(runtime);
  batch = MLN_HANDLE_NULL;
  MLN_TEST_INVALID_STATE(mln_runtime_drain_events(runtime, &batch, NULL));
  MLN_TEST_INVALID_STATE(
    mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_ALL, NULL)
  );
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
  mln_event_batch batch = MLN_HANDLE_NULL;
  probe->drain_status = mln_runtime_drain_events(probe->runtime, &batch, NULL);
  mln_event_batch_release(batch);
  probe->runtime_mask_status = mln_runtime_set_event_mask(
    probe->runtime, MLN_RUNTIME_EVENT_MASK_ALL, NULL
  );
  probe->map_mask_status =
    mln_test_map_set_event_mask(probe->map, MLN_RUNTIME_EVENT_MASK_ALL);
}

// Map creation rejects a mask the setter rejects, so one value is not accepted
// at creation and then refused on the way back through a read-modify-write.
// The runtime options have the same check in lifecycle.c.
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

// A creation mask applies before MapLibre reports constructor-time events.
static void a_creation_mask_applies_during_construction(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_map_options options =
    map_options_with_event_mask(MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED);
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  uint64_t map_mask = MLN_RUNTIME_EVENT_MASK_ALL;
  MLN_TEST_OK(mln_test_map_get_event_mask(map, &map_mask));
  TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED, map_mask);

  mln_test_event_batch batch = mln_test_event_batch_default();
  MLN_TEST_OK(mln_test_drain_events(runtime, &batch));
  TEST_ASSERT_EQUAL_size_t(0, batch.event_count);

  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  TEST_ASSERT_EQUAL_size_t(
    0, mln_test_drain_counting(
         runtime, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
       )
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
           ~(uint64_t)MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE
  ));
  mln_test_drain_all(runtime);

  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  TEST_ASSERT_EQUAL_size_t(
    0, mln_test_drain_counting(
         runtime, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
       )
  );

  MLN_TEST_OK(mln_test_map_set_event_mask(map, MLN_RUNTIME_EVENT_MASK_ALL));
  MLN_TEST_OK(mln_test_map_request_repaint(map));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_GREATER_THAN_size_t(
    0, mln_test_drain_counting(
         runtime, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
       )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Suppression happens at push time, so a cleared type produces no queue record
// even though the runtime worker continues to process the command.
static void a_suppressed_producer_leaves_the_queue_empty(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const mln_map_options options =
    map_options_with_event_mask(MLN_RUNTIME_EVENT_MASK_NONE);
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  MLN_TEST_OK(
    mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_NONE, NULL)
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_drain_all(runtime));

  MLN_TEST_OK(mln_test_map_request_repaint(map));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_drain_all(runtime));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Closing a source prevents future publication without rewriting history that
// is already queued.
static void queued_events_outlive_the_map_that_produced_them(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_destroy_map(map);

  mln_event_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_drain_events(runtime, &batch, NULL));
  mln_event_batch_view view = {.size = sizeof(mln_event_batch_view)};
  MLN_TEST_OK(mln_event_batch_get(batch, &view, NULL));
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
    TEST_ASSERT_TRUE(
      (size_t)event->message_offset + event->message_size <= view.messages_size
    );
  }
  TEST_ASSERT_GREATER_THAN_size_t(0, map_sourced);

  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_GREATER_THAN_size_t(0, view.event_count);
  mln_event_batch_release(batch);
}

// Counts the transition-finished events in one drain and reports the last
// transition ID it saw.
static size_t drain_counting_transitions(
  mln_runtime runtime, uint64_t* out_last_transition_id
) {
  size_t found = 0;
  mln_test_event_batch batch = mln_test_event_batch_default();
  MLN_TEST_OK(mln_test_drain_events(runtime, &batch));
  for (size_t index = 0; index < batch.event_count; index += 1) {
    const mln_runtime_event* event = batch_event(&batch, index);
    if (event->type != MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED) {
      continue;
    }
    found += 1;
    if (out_last_transition_id != NULL) {
      *out_last_transition_id =
        event->payload.camera_transition_finished.transition_id;
    }
  }
  return found;
}

static void submit_camera_update(
  mln_map map, uint32_t mode, double zoom, uint64_t transition_id
) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = zoom;
  mln_animation_options animation = mln_animation_options_default();
  // Long enough that the transition is still running when the next command
  // ends it, so every outcome below is the one the test named.
  animation.fields = MLN_ANIMATION_OPTION_DURATION;
  animation.duration_ms = 60000;
  if (transition_id != 0) {
    animation.fields |= MLN_ANIMATION_OPTION_TRANSITION_ID;
    animation.transition_id = transition_id;
  }
  mln_camera_update update = mln_camera_update_default();
  update.mode = mode;
  update.camera = camera;
  update.animation = animation;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

// A transition ends exactly once, whichever way it ends: replaced by a later
// transition, cancelled by a jump, or never reported because the caller
// omitted an ID.
static void a_transition_reports_one_terminal_outcome(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  submit_camera_update(map, MLN_CAMERA_UPDATE_MODE_EASE, 4.0, 11);
  TEST_ASSERT_EQUAL_size_t(0, drain_counting_transitions(runtime, NULL));

  // The second ease replaces the first, which ends the first and reports its
  // own ID rather than the superseding one.
  submit_camera_update(map, MLN_CAMERA_UPDATE_MODE_EASE, 6.0, 12);
  uint64_t transition_id = 0;
  TEST_ASSERT_EQUAL_size_t(
    1, drain_counting_transitions(runtime, &transition_id)
  );
  TEST_ASSERT_EQUAL_UINT64(11, transition_id);

  // A jump cancels the running transition, which reports the cancelled ID.
  submit_camera_update(map, MLN_CAMERA_UPDATE_MODE_JUMP, 8.0, 0);
  transition_id = 0;
  TEST_ASSERT_EQUAL_size_t(
    1, drain_counting_transitions(runtime, &transition_id)
  );
  TEST_ASSERT_EQUAL_UINT64(12, transition_id);

  // An ease with no transition ID is silent, and so is the jump that ends it.
  submit_camera_update(map, MLN_CAMERA_UPDATE_MODE_EASE, 10.0, 0);
  submit_camera_update(map, MLN_CAMERA_UPDATE_MODE_JUMP, 12.0, 0);
  TEST_ASSERT_EQUAL_size_t(0, drain_counting_transitions(runtime, NULL));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Two text-bearing events in one batch point at distinct arena ranges, and each
// range holds that event's own bytes.
static void the_message_arena_carries_one_range_per_event(void) {
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
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));

  mln_test_event_batch batch = mln_test_event_batch_default();
  MLN_TEST_OK(mln_test_drain_events(runtime, &batch));

  size_t failures = 0;
  uint32_t first_offset = 0;
  uint32_t second_offset = 0;
  for (size_t index = 0; index < batch.event_count; index += 1) {
    const mln_runtime_event* event = batch_event(&batch, index);
    if (event->type != MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
      continue;
    }
    TEST_ASSERT_GREATER_THAN_UINT32(0, event->message_size);
    TEST_ASSERT_TRUE(
      (size_t)event->message_offset + event->message_size <= batch.messages_size
    );
    // The arena null-terminates each message, so a host reads a range as a C
    // string without copying it first.
    TEST_ASSERT_EQUAL_CHAR(
      '\0', batch.messages[event->message_offset + event->message_size]
    );
    TEST_ASSERT_EQUAL_size_t(
      event->message_size, strlen(batch.messages + event->message_offset)
    );
    if (failures == 0) {
      first_offset = event->message_offset;
    } else {
      second_offset = event->message_offset;
    }
    failures += 1;
  }
  TEST_ASSERT_EQUAL_size_t(2, failures);
  TEST_ASSERT_NOT_EQUAL_UINT32(first_offset, second_offset);

  mln_test_destroy_map(second);
  mln_test_destroy_map(first);
  mln_test_destroy_runtime(runtime);
}

// A fresh map and runtime select every event type. The masks change from any
// thread, and each setter rejects unknown bits but keeps the other group's.
static void event_masks_select_every_type_and_keep_foreign_bits(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  {
    uint64_t runtime_mask = 0;
    MLN_TEST_OK(mln_runtime_get_event_mask(runtime, &runtime_mask, NULL));
    TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_ALL, runtime_mask);
    uint64_t map_mask = 0;
    MLN_TEST_OK(mln_test_map_get_event_mask(map, &map_mask));
    TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_ALL, map_mask);

    mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
    TEST_ASSERT_GREATER_THAN_size_t(
      0, mln_test_drain_counting(runtime, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
    );
  }
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
  }
  // Each setter reads only its own group's bits but stores the whole value, so
  // a host that reads a mask, sets one bit, and writes it back keeps every
  // other.
  {
    MLN_TEST_INVALID(
      mln_runtime_set_event_mask(runtime, unknown_mask_bit, NULL)
    );
    MLN_TEST_INVALID(mln_test_map_set_event_mask(map, unknown_mask_bit));

    MLN_TEST_OK(
      mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_ALL, NULL)
    );
    MLN_TEST_OK(mln_test_map_set_event_mask(map, MLN_RUNTIME_EVENT_MASK_ALL));

    uint64_t runtime_mask = 0;
    MLN_TEST_OK(mln_runtime_get_event_mask(runtime, &runtime_mask, NULL));
    TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_ALL, runtime_mask);
    uint64_t map_mask = 0;
    MLN_TEST_OK(mln_test_map_get_event_mask(map, &map_mask));
    TEST_ASSERT_EQUAL_UINT64(MLN_RUNTIME_EVENT_MASK_ALL, map_mask);

    // One in-group bit for the other source kind, which each setter accepts and
    // reports back unchanged.
    MLN_TEST_OK(mln_runtime_set_event_mask(
      runtime, MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED, NULL
    ));
    MLN_TEST_OK(mln_runtime_get_event_mask(runtime, &runtime_mask, NULL));
    TEST_ASSERT_EQUAL_UINT64(
      MLN_RUNTIME_EVENT_MASK_MAP_STYLE_LOADED, runtime_mask
    );

    MLN_TEST_INVALID(mln_runtime_get_event_mask(runtime, NULL, NULL));
    MLN_TEST_INVALID(mln_map_snapshot_get(map, NULL, NULL));
    mln_map_snapshot undersized = {.size = sizeof(mln_map_snapshot) - 1};
    MLN_TEST_INVALID(mln_map_snapshot_get(map, &undersized, NULL));
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A drain of an empty queue reports not ready, leaves its output null, and
// consumes nothing: the next drain is empty too, and the next event still
// arrives in a batch.
static void an_empty_drain_publishes_no_batch(void) {
  mln_runtime runtime = mln_test_create_runtime();
  for (int attempt = 0; attempt < 2; attempt += 1) {
    mln_event_batch batch = MLN_HANDLE_NULL;
    MLN_TEST_STATUS(
      MLN_STATUS_NOT_READY, mln_runtime_drain_events(runtime, &batch, NULL)
    );
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, batch);
  }

  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_event_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_drain_events(runtime, &batch, NULL));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, batch);
  mln_event_batch_view view = {.size = sizeof(mln_event_batch_view)};
  MLN_TEST_OK(mln_event_batch_get(batch, &view, NULL));
  TEST_ASSERT_GREATER_THAN_size_t(0, view.event_count);
  mln_event_batch_release(batch);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A drained batch is an owned handle that stays readable across later drains.
static void a_drained_batch_is_an_owned_handle(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  {
    mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);

    mln_event_batch first = MLN_HANDLE_NULL;
    MLN_TEST_OK(mln_runtime_drain_events(runtime, &first, NULL));
    mln_event_batch_view first_view = {.size = sizeof(mln_event_batch_view)};
    MLN_TEST_OK(mln_event_batch_get(first, &first_view, NULL));
    TEST_ASSERT_EQUAL_UINT32(sizeof(mln_runtime_event), first_view.event_size);
    TEST_ASSERT_GREATER_THAN_size_t(1, first_view.event_count);
    const size_t first_count = first_view.event_count;
    const mln_runtime_event first_event = first_view.events[0];

    mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
    mln_event_batch second = MLN_HANDLE_NULL;
    MLN_TEST_OK(mln_runtime_drain_events(runtime, &second, NULL));
    mln_event_batch_view second_view = {.size = sizeof(mln_event_batch_view)};
    MLN_TEST_OK(mln_event_batch_get(second, &second_view, NULL));
    TEST_ASSERT_GREATER_THAN_size_t(0, second_view.event_count);
    TEST_ASSERT_EQUAL_size_t(first_count, first_view.event_count);
    TEST_ASSERT_EQUAL_UINT32(first_event.type, first_view.events[0].type);

    mln_event_batch_release(second);
    mln_event_batch_release(first);
  }
  // A drained batch is an owned handle: releasing it twice is a no-op,
  // releasing the null handle is a no-op, and a released handle is stale.
  {
    mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);

    mln_event_batch batch = MLN_HANDLE_NULL;
    MLN_TEST_OK(mln_runtime_drain_events(runtime, &batch, NULL));
    TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, batch);
    mln_event_batch_release(batch);
    mln_event_batch_release(batch);
    mln_event_batch_release(MLN_HANDLE_NULL);

    mln_event_batch_view view = {
      .size = sizeof(mln_event_batch_view), .event_count = 99
    };
    MLN_TEST_INVALID_STATE(mln_event_batch_get(batch, &view, NULL));
    TEST_ASSERT_EQUAL_size_t(99, view.event_count);
    MLN_TEST_INVALID(mln_event_batch_get(MLN_HANDLE_NULL, &view, NULL));
    TEST_ASSERT_EQUAL_size_t(99, view.event_count);
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_drain_rejects_a_nonnull_output_or_a_stale_runtime);
  RUN_TEST(event_masks_select_every_type_and_keep_foreign_bits);
  RUN_TEST(options_reject_unknown_event_mask_bits);
  RUN_TEST(a_creation_mask_applies_during_construction);
  RUN_TEST(clearing_one_type_leaves_the_others_arriving);
  RUN_TEST(a_suppressed_producer_leaves_the_queue_empty);
  RUN_TEST(an_empty_drain_publishes_no_batch);
  RUN_TEST(a_drained_batch_is_an_owned_handle);
  RUN_TEST(queued_events_outlive_the_map_that_produced_them);
  RUN_TEST(a_transition_reports_one_terminal_outcome);
  RUN_TEST(the_message_arena_carries_one_range_per_event);
}
