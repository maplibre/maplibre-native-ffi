// The snapshot generation that every map event carries: a command's events are
// queued before its completion with the generation it reports, a state event
// follows a snapshot that already includes its change, and one map's events
// arrive in non-decreasing generation order.

#include "support/style.h"
#include "support/test_support.h"

typedef struct recorded_event {
  uint32_t type;
  uint64_t source;
  uint64_t generation;
  int32_t code;
  uint64_t transition_id;
  // The published snapshot read when the event was drained, set only when the
  // recorder reads snapshots.
  bool snapshot_read;
  double zoom;
  bool fully_loaded;
} recorded_event;

// Drains from any thread: the test thread, a completion, or the runtime's
// event wake. A spin lock serializes the drains and the test's reads.
typedef struct event_recorder {
  atomic_flag lock;
  mln_runtime runtime;
  _Atomic uint64_t map;
  bool read_snapshots;
  recorded_event events[2048];
  size_t count;
  bool overflowed;
  bool drain_failed;
} event_recorder;

static event_recorder recorder = {.lock = ATOMIC_FLAG_INIT};

static void recorder_lock(void) {
  while (
    atomic_flag_test_and_set_explicit(&recorder.lock, memory_order_acquire)) {
  }
}

static void recorder_unlock(void) {
  atomic_flag_clear_explicit(&recorder.lock, memory_order_release);
}

static void recorder_reset(mln_runtime runtime, bool read_snapshots) {
  recorder_lock();
  recorder.runtime = runtime;
  atomic_store(&recorder.map, MLN_HANDLE_NULL);
  recorder.read_snapshots = read_snapshots;
  recorder.count = 0;
  recorder.overflowed = false;
  recorder.drain_failed = false;
  recorder_unlock();
}

static void record_event(const mln_runtime_event* event) {
  if (recorder.count == sizeof(recorder.events) / sizeof(*recorder.events)) {
    recorder.overflowed = true;
    return;
  }
  recorded_event* record = &recorder.events[recorder.count];
  recorder.count += 1;
  *record = (recorded_event){
    .type = event->type,
    .source = event->source,
    .generation = event->generation,
    .code = event->code,
    .transition_id = event->payload_type ==
                         MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_TRANSITION_FINISHED
                       ? event->payload.camera_transition_finished.transition_id
                       : 0,
  };
  const uint64_t map = atomic_load(&recorder.map);
  if (
    !recorder.read_snapshots || map == MLN_HANDLE_NULL || event->source != map
  ) {
    return;
  }
  mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
  if (mln_map_get_snapshot(map, &snapshot, NULL) != MLN_STATUS_OK) {
    recorder.drain_failed = true;
    return;
  }
  record->snapshot_read = true;
  record->zoom = snapshot.camera.zoom;
  record->fully_loaded = snapshot.fully_loaded;
}

// Drains the queue into the recorder without asserting, so a wake or a
// completion may call it; the recorder keeps a failure for the test to report.
static void recorder_drain(void) {
  recorder_lock();
  while (true) {
    mln_event_batch batch = MLN_HANDLE_NULL;
    const mln_status drained =
      mln_runtime_drain_events(recorder.runtime, &batch, NULL);
    // An empty queue publishes no batch.
    if (drained == MLN_STATUS_NOT_READY) break;
    if (drained != MLN_STATUS_OK) {
      recorder.drain_failed = true;
      break;
    }
    mln_event_batch_view view = {.size = sizeof(view)};
    if (mln_event_batch_get(batch, &view, NULL) != MLN_STATUS_OK) {
      recorder.drain_failed = true;
      mln_event_batch_release(batch);
      break;
    }
    for (size_t index = 0; index < view.event_count; index += 1) {
      record_event((const mln_runtime_event*)((const char*)view.events +
                                              (index * view.event_size)));
    }
    mln_event_batch_release(batch);
  }
  recorder_unlock();
}

static size_t recorder_count(void) {
  recorder_lock();
  const size_t count = recorder.count;
  recorder_unlock();
  return count;
}

static void assert_recorder_complete(void) {
  recorder_lock();
  const bool failed = recorder.drain_failed;
  const bool overflowed = recorder.overflowed;
  recorder_unlock();
  TEST_ASSERT_FALSE_MESSAGE(failed, "a drain or snapshot read failed");
  TEST_ASSERT_FALSE_MESSAGE(overflowed, "the recorder overflowed");
}

static void assert_map_generations_never_decrease(mln_map map) {
  uint64_t previous = 0;
  size_t seen = 0;
  for (size_t index = 0; index < recorder.count; index += 1) {
    const recorded_event* event = &recorder.events[index];
    if (event->source != map) continue;
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64(previous, event->generation);
    previous = event->generation;
    seen += 1;
  }
  TEST_ASSERT_GREATER_THAN_size_t(0, seen);
}

// Forwards a command's completion after draining the queue into the recorder,
// so the test sees what was queued when the completion ran.
typedef struct completion_probe {
  mln_completion inner;
  size_t count_at_completion;
} completion_probe;

static void drain_then_complete(
  void* user_data, const mln_completion_result* result
) {
  completion_probe* probe = user_data;
  recorder_drain();
  probe->count_at_completion = recorder_count();
  probe->inner.callback(probe->inner.user_data, result);
}

static void release_probe(void* user_data) {
  const completion_probe* probe = user_data;
  if (probe->inner.release_user_data != NULL) {
    probe->inner.release_user_data(probe->inner.user_data);
  }
}

typedef mln_status (*map_command)(
  mln_map map, const mln_completion* completion
);

typedef struct probed_result {
  mln_status status;
  uint32_t disposition;
  uint64_t generation;
  // Recorder entries [start, end) are the events queued before the completion.
  size_t start;
  size_t end;
} probed_result;

static probed_result run_probed(mln_map map, map_command command) {
  mln_test_completion completion = mln_test_completion_default(0);
  completion_probe probe = {.inner = completion.descriptor};
  const mln_completion probed = {
    .size = sizeof(mln_completion),
    .callback = drain_then_complete,
    .user_data = &probe,
    .release_user_data = release_probe,
  };
  const size_t start = recorder_count();
  MLN_TEST_OK(command(map, &probed));
  probed_result result = {
    .status = mln_test_completion_finish(&completion),
    .disposition = mln_test_completion_disposition(&completion),
    .generation = mln_test_completion_generation(&completion),
    .start = start,
    .end = probe.count_at_completion,
  };
  mln_test_completion_destroy(&completion);
  return result;
}

static mln_status set_background_style(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_style_json(
    map, mln_test_background_style_json, completion, NULL
  );
}

static mln_status set_malformed_style(
  mln_map map, const mln_completion* completion
) {
  return mln_map_set_style_json(map, MLN_BUFFER_LITERAL("{"), completion, NULL);
}

static mln_status jump_to_zoom_three(
  mln_map map, const mln_completion* completion
) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = 3.0;
  return mln_map_update_camera(map, &update, completion, NULL);
}

static mln_status request_repaint(
  mln_map map, const mln_completion* completion
) {
  return mln_map_request_repaint(map, completion, NULL);
}

static size_t count_in_range(
  const probed_result* result, mln_map map, uint32_t type
) {
  size_t found = 0;
  for (size_t index = result->start; index < result->end; index += 1) {
    const recorded_event* event = &recorder.events[index];
    if (event->source == map && event->type == type) found += 1;
  }
  return found;
}

static void assert_range_carries_generation(
  const probed_result* result, mln_map map
) {
  for (size_t index = result->start; index < result->end; index += 1) {
    const recorded_event* event = &recorder.events[index];
    if (event->source != map) continue;
    TEST_ASSERT_EQUAL_UINT64(result->generation, event->generation);
  }
}

// A command's events are queued before its completion runs, all with the
// generation the completion reports, and they end with the one render update
// the command raises. Events raised before a command carry a lower generation.
static void a_command_queues_its_events_before_it_completes(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  recorder_reset(runtime, false);
  recorder_drain();
  const size_t creation_end = recorder_count();

  const probed_result style = run_probed(map, set_background_style);
  MLN_TEST_OK(style.status);
  TEST_ASSERT_GREATER_THAN_UINT64(0, style.generation);
  TEST_ASSERT_EQUAL_size_t(
    1, count_in_range(&style, map, MLN_RUNTIME_EVENT_MAP_STYLE_LOADED)
  );
  TEST_ASSERT_EQUAL_size_t(
    1,
    count_in_range(&style, map, MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE,
    recorder.events[style.end - 1].type
  );
  assert_range_carries_generation(&style, map);
  for (size_t index = 0; index < creation_end; index += 1) {
    TEST_ASSERT_LESS_THAN_UINT64(
      style.generation, recorder.events[index].generation
    );
  }

  const probed_result jump = run_probed(map, jump_to_zoom_three);
  MLN_TEST_OK(jump.status);
  TEST_ASSERT_GREATER_THAN_UINT64(style.generation, jump.generation);
  TEST_ASSERT_EQUAL_size_t(
    1, count_in_range(&jump, map, MLN_RUNTIME_EVENT_MAP_CAMERA_WILL_CHANGE)
  );
  TEST_ASSERT_EQUAL_size_t(
    1, count_in_range(&jump, map, MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE)
  );
  assert_range_carries_generation(&jump, map);

  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  recorder_drain();
  assert_recorder_complete();
  assert_map_generations_never_decrease(map);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A command that runs and fails still publishes, and the failure event it
// raised carries the generation its completion reports.
static void a_failed_command_reports_the_generation_of_its_events(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  recorder_reset(runtime, false);
  recorder_drain();

  const probed_result failed = run_probed(map, set_malformed_style);
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, failed.status);
  TEST_ASSERT_EQUAL_UINT32(MLN_COMMAND_DISPOSITION_FAILED, failed.disposition);
  TEST_ASSERT_GREATER_THAN_UINT64(0, failed.generation);
  TEST_ASSERT_EQUAL_size_t(
    1, count_in_range(&failed, map, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED)
  );
  assert_range_carries_generation(&failed, map);
  assert_recorder_complete();

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A malformed style fails a pending still image from inside the style
// command. The image's failure event carries the command's generation, and it
// is queued before the image's completion runs.
static void a_command_that_fails_a_still_image_queues_its_event_first(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  recorder_reset(runtime, false);
  recorder_drain();

  // Without a render session the request stays pending until the style
  // command fails it.
  mln_test_completion still = mln_test_completion_default(0);
  completion_probe still_probe = {.inner = still.descriptor};
  const mln_completion probed_still = {
    .size = sizeof(mln_completion),
    .callback = drain_then_complete,
    .user_data = &still_probe,
    .release_user_data = release_probe,
  };
  MLN_TEST_OK(mln_map_request_still_image(map, &probed_still, NULL));
  const probed_result failed = run_probed(map, set_malformed_style);
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, failed.status);
  TEST_ASSERT_EQUAL_UINT32(MLN_COMMAND_DISPOSITION_FAILED, failed.disposition);
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, mln_test_completion_finish(&still));
  mln_test_completion_destroy(&still);
  assert_recorder_complete();

  const recorded_event* still_failed = NULL;
  for (size_t index = 0; index < still_probe.count_at_completion; index += 1) {
    const recorded_event* event = &recorder.events[index];
    if (
      event->source == map &&
      event->type == MLN_RUNTIME_EVENT_MAP_STILL_IMAGE_FAILED
    ) {
      still_failed = event;
    }
  }
  TEST_ASSERT_NOT_NULL_MESSAGE(
    still_failed, "the still image completed before its event was queued"
  );
  TEST_ASSERT_EQUAL_UINT64(failed.generation, still_failed->generation);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Render updates that coalesce against the queue tail keep the newest
// generation, so the one unread update names the later command.
static void a_coalesced_render_update_carries_the_newest_generation(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.event_mask = MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_drain_all(runtime);

  mln_test_completion first = mln_test_completion_default(0);
  MLN_TEST_OK(request_repaint(map, &first.descriptor));
  MLN_TEST_OK(mln_test_completion_finish(&first));
  mln_test_completion second = mln_test_completion_default(0);
  MLN_TEST_OK(request_repaint(map, &second.descriptor));
  MLN_TEST_OK(mln_test_completion_finish(&second));
  const uint64_t newest = mln_test_completion_generation(&second);
  TEST_ASSERT_GREATER_THAN_UINT64(
    mln_test_completion_generation(&first), newest
  );
  mln_test_completion_destroy(&first);
  mln_test_completion_destroy(&second);

  recorder_reset(runtime, false);
  recorder_drain();
  assert_recorder_complete();
  TEST_ASSERT_EQUAL_size_t(1, recorder.count);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE, recorder.events[0].type
  );
  TEST_ASSERT_EQUAL_UINT64(newest, recorder.events[0].generation);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void submit_ease(mln_map map, double zoom, uint64_t transition_id) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  update.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  update.animation.duration_ms = 60000.0;
  if (transition_id != 0) {
    update.animation.fields |= MLN_ANIMATION_OPTION_TRANSITION_ID;
    update.animation.transition_id = transition_id;
  }
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

// A host that selects only finished transitions still receives them, from the
// command that ended the transition and with its generation.
static void finished_transitions_arrive_without_camera_changes(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.event_mask = MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_drain_all(runtime);
  recorder_reset(runtime, false);

  submit_ease(map, 6.0, 7);
  const probed_result jump = run_probed(map, jump_to_zoom_three);
  MLN_TEST_OK(jump.status);
  assert_recorder_complete();
  TEST_ASSERT_EQUAL_size_t(1, jump.end - jump.start);
  const recorded_event* finished = &recorder.events[jump.start];
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED, finished->type
  );
  TEST_ASSERT_EQUAL_UINT64(7, finished->transition_id);
  TEST_ASSERT_EQUAL_UINT64(jump.generation, finished->generation);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Drains on every wake, so each event is recorded with the snapshot published
// when the map queued it.
static void drain_on_wake(void* user_data) {
  (void)user_data;
  recorder_drain();
}

static bool recorded(uint32_t type, int32_t code) {
  const uint64_t map = atomic_load(&recorder.map);
  recorder_lock();
  bool found = false;
  for (size_t index = 0; index < recorder.count && !found; index += 1) {
    const recorded_event* event = &recorder.events[index];
    found = event->source == map && event->type == type && event->code == code;
  }
  recorder_unlock();
  return found;
}

static bool idle_recorded(void* context) {
  (void)context;
  return recorded(MLN_RUNTIME_EVENT_MAP_IDLE, 0);
}

static bool animated_change_recorded(void* context) {
  (void)context;
  return recorded(
    MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE, MLN_CAMERA_CHANGE_MODE_ANIMATED
  );
}

// A state event follows the snapshot that includes its change. When the eased
// camera reports its final change the snapshot is at the target zoom, and when
// the map reports that it finished loading or went idle the snapshot is fully
// loaded.
static void state_events_follow_a_snapshot_that_includes_them(void) {
  static const double target_zoom = 5.0;
  mln_runtime_options runtime_options = mln_runtime_options_default();
  runtime_options.event_wake = (mln_wake){.callback = drain_on_wake};
  // The fixture's runtime helper installs its own wake, so this creates the
  // runtime directly.
  mln_runtime runtime = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_runtime_create(&runtime_options, &runtime, MLN_TEST_DIAGNOSTIC)
  );
  recorder_reset(runtime, true);
  // The extent the render fixture attaches with, so the first frame renders.
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 64;
  options.initial_extent.height = 64;
  options.event_mask = MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_DID_CHANGE |
                       MLN_RUNTIME_EVENT_MASK_MAP_LOADING_FINISHED |
                       MLN_RUNTIME_EVENT_MASK_MAP_IDLE;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  atomic_store(&recorder.map, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  MLN_TEST_AWAIT_OK(mln_map_set_style_json(
    map, mln_test_background_style_json, &completion.descriptor, NULL
  ));
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, idle_recorded, NULL, "the map never went idle"
  ));

  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = target_zoom;
  update.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  update.animation.duration_ms = 100.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, animated_change_recorded, NULL,
    "the ease never reported its final camera change"
  ));
  // Destroying the map ends its events, so the recorder is read unshared.
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  assert_recorder_complete();

  size_t loading_finished = 0;
  size_t idle = 0;
  const recorded_event* final_change = NULL;
  for (size_t index = 0; index < recorder.count; index += 1) {
    const recorded_event* event = &recorder.events[index];
    if (event->source != map) continue;
    switch (event->type) {
      case MLN_RUNTIME_EVENT_MAP_LOADING_FINISHED:
        loading_finished += 1;
        TEST_ASSERT_TRUE(event->snapshot_read);
        TEST_ASSERT_TRUE(event->fully_loaded);
        break;
      case MLN_RUNTIME_EVENT_MAP_IDLE:
        idle += 1;
        TEST_ASSERT_TRUE(event->snapshot_read);
        TEST_ASSERT_TRUE(event->fully_loaded);
        break;
      case MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE:
        final_change = event;
        break;
      default:
        break;
    }
  }
  TEST_ASSERT_GREATER_THAN_size_t(0, loading_finished);
  TEST_ASSERT_GREATER_THAN_size_t(0, idle);
  TEST_ASSERT_NOT_NULL(final_change);
  TEST_ASSERT_EQUAL_INT32(MLN_CAMERA_CHANGE_MODE_ANIMATED, final_change->code);
  TEST_ASSERT_TRUE(final_change->snapshot_read);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, target_zoom, final_change->zoom);

  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_command_queues_its_events_before_it_completes);
  RUN_TEST(a_failed_command_reports_the_generation_of_its_events);
  RUN_TEST(a_command_that_fails_a_still_image_queues_its_event_first);
  RUN_TEST(a_coalesced_render_update_carries_the_newest_generation);
  RUN_TEST(finished_transitions_arrive_without_camera_changes);
  RUN_TEST(state_events_follow_a_snapshot_that_includes_them);
}
