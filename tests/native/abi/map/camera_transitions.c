// Camera transitions and gestures: an ease or an animated delta advances only
// as frames render, its end handler reports how it ended exactly once, a
// cancellation stops it where it stands, and gesture phases mark the map
// around a camera write.

#include "support/camera.h"
#include "support/test_support.h"

static mln_camera_options test_camera(void) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  camera.center.latitude = 37.7749;
  camera.center.longitude = -122.4194;
  camera.zoom = 11.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  return camera;
}

static void update_camera(mln_map map, const mln_camera_update* update) {
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, update, &completion.descriptor, NULL)
  );
}

// Submits the update and returns the generation that its completion reports.
static uint64_t update_camera_generation(
  mln_map map, const mln_camera_update* update
) {
  mln_test_completion command = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_update_camera(map, update, &command.descriptor, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&command));
  const uint64_t generation = mln_test_completion_generation(&command);
  mln_test_completion_destroy(&command);
  return generation;
}

// An ease toward `zoom` that lasts `duration_ms` and reports its end to
// `probe`, when one is given.
static mln_camera_update ease_to_zoom(
  double zoom, double duration_ms, mln_test_transition_end* probe
) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  update.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  update.animation.duration_ms = duration_ms;
  if (probe != NULL) {
    update.animation.end_handler = mln_test_transition_end_handler(probe);
  }
  return update;
}

// Gives the animation an identity for mln_map_cancel_camera_transition().
static void with_transition_id(
  mln_animation_options* animation, uint64_t transition_id
) {
  animation->fields |= MLN_ANIMATION_OPTION_TRANSITION_ID;
  animation->transition_id = transition_id;
}

static mln_map_snapshot read_settled_snapshot(
  mln_runtime runtime, mln_map map
) {
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  return snapshot;
}

// An ordered camera read.
static mln_camera_options query_camera(mln_map map) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_query_result));
  MLN_TEST_OK(mln_map_get_camera(map, &query.descriptor, NULL));
  mln_camera_query_result result = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &result, sizeof(result))
  );
  return result.camera;
}

// Asserts that the handler ran once with `outcome`, and released after it.
static void assert_ended(
  mln_test_transition_end* probe, uint32_t outcome, const char* label
) {
  TEST_ASSERT_TRUE_MESSAGE(mln_test_wait_transition_end(probe), label);
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, atomic_load(&probe->calls), label);
  TEST_ASSERT_EQUAL_INT_MESSAGE(1, atomic_load(&probe->releases), label);
  TEST_ASSERT_FALSE_MESSAGE(atomic_load(&probe->released_first), label);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    sizeof(mln_camera_transition_end), atomic_load(&probe->end_size), label
  );
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    outcome, atomic_load(&probe->outcome), label
  );
}

// What the handler of a rendered transition saw from inside its callback.
typedef struct end_observation {
  mln_runtime runtime;
  mln_map map;
  bool did_change_queued;
  bool render_update_queued;
  bool snapshot_read;
  uint64_t snapshot_generation;
  double snapshot_zoom;
} end_observation;

// Drains the queue and reads the published snapshot from inside the callback.
// The callback runs after the MapLibre update that ended the transition
// returns, so the events of that update, its render update included, are
// queued.
static void observe_end(
  mln_test_transition_end* probe, const mln_camera_transition_end* end
) {
  end_observation* observation = probe->context;
  mln_event_batch batch = MLN_HANDLE_NULL;
  while (mln_runtime_drain_events(observation->runtime, &batch, NULL) ==
         MLN_STATUS_OK) {
    mln_event_batch_view view = {.size = sizeof(view)};
    if (mln_event_batch_get(batch, &view, NULL) == MLN_STATUS_OK) {
      for (size_t index = 0; index < view.event_count; index += 1) {
        const mln_runtime_event* event =
          (const mln_runtime_event*)((const char*)view.events +
                                     (index * view.event_size));
        if (
          event->type == MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE &&
          event->generation == end->generation
        ) {
          observation->did_change_queued = true;
        }
        if (
          event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE &&
          event->generation >= end->generation
        ) {
          observation->render_update_queued = true;
        }
      }
    }
    mln_event_batch_release(batch);
    batch = MLN_HANDLE_NULL;
  }
  mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
  if (
    mln_map_get_snapshot(observation->map, &snapshot, NULL) == MLN_STATUS_OK
  ) {
    observation->snapshot_read = true;
    observation->snapshot_generation = snapshot.generation;
    observation->snapshot_zoom = snapshot.camera.zoom;
  }
}

typedef struct render_wait {
  const mln_test_render_fixture* fixture;
  mln_test_transition_end* probe;
  bool demand_pending;
  bool failed;
} render_wait;

// Keeps one frame demand in flight until the handler releases. Each rendered
// frame advances the transition on MapLibre's clock.
static bool transition_ended(void* context) {
  render_wait* wait = context;
  if (wait->demand_pending) {
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    const mln_status drained = mln_render_session_drain_frame_results(
      wait->fixture->session, &batch, NULL
    );
    if (drained == MLN_STATUS_NOT_READY) {
      return false;
    }
    if (drained != MLN_STATUS_OK) {
      wait->failed = true;
      return true;
    }
    mln_render_frame_batch_release(batch);
    wait->demand_pending = false;
  }
  if (atomic_load(&wait->probe->releases) > 0) {
    return true;
  }
  mln_frame_demand demand = mln_frame_demand_default();
  if (
    mln_render_session_request_frame(wait->fixture->session, &demand, NULL) !=
    MLN_STATUS_OK
  ) {
    wait->failed = true;
    return true;
  }
  wait->demand_pending = true;
  return false;
}

// Renders frames until the handler of `probe` has run and released.
static void render_until_ended(
  const mln_test_render_fixture* fixture, mln_test_transition_end* probe
) {
  render_wait wait = {.fixture = fixture, .probe = probe};
  MLN_TEST_OK(mln_test_render_step_until(
    fixture, transition_ended, &wait, mln_test_deadline_default(),
    "a camera transition end handler"
  ));
  TEST_ASSERT_FALSE(wait.failed);
  TEST_ASSERT_FALSE(wait.demand_pending);
}

// Renders one more frame and waits for its result.
static bool settled_result(void* context) {
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  const mln_test_render_fixture* fixture = context;
  if (
    mln_render_session_drain_frame_results(fixture->session, &batch, NULL) !=
    MLN_STATUS_OK
  ) {
    return false;
  }
  mln_render_frame_batch_release(batch);
  return true;
}

// An ease with a duration completes only while frames render. Its handler runs
// once, from a task after the frame that ended it, when the camera change of
// its generation is already queued and the published snapshot shows the
// target. A later frame, the fence for the negative check, runs it no more.
static void a_rendered_ease_completes_at_its_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_drain_all(runtime);

  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  end_observation observation = {.runtime = runtime, .map = map};
  probe.hook = observe_end;
  probe.context = &observation;
  const mln_camera_update ease = ease_to_zoom(5.0, 20.0, &probe);
  const uint64_t started = update_camera_generation(map, &ease);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.calls));

  render_until_ended(&fixture, &probe);
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "ease");
  const uint64_t generation = atomic_load(&probe.generation);
  TEST_ASSERT_GREATER_THAN_UINT64(started, generation);
  TEST_ASSERT_TRUE(observation.did_change_queued);
  TEST_ASSERT_TRUE(observation.render_update_queued);
  TEST_ASSERT_TRUE(observation.snapshot_read);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
    generation, observation.snapshot_generation
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, observation.snapshot_zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, query_camera(map).zoom);

  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, settled_result, (void*)&fixture, mln_test_deadline_default(),
    "a fence frame"
  ));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.calls));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct immediate_case {
  const char* label;
  uint32_t mode;
  uint32_t animation_fields;
} immediate_case;

static const immediate_case immediate_cases[] = {
  {"jump", MLN_CAMERA_UPDATE_MODE_JUMP, 0},
  {"ease with a zero duration", MLN_CAMERA_UPDATE_MODE_EASE,
   MLN_ANIMATION_OPTION_DURATION},
  {"ease with no duration", MLN_CAMERA_UPDATE_MODE_EASE, 0},
  {"fly with a zero duration", MLN_CAMERA_UPDATE_MODE_FLY,
   MLN_ANIMATION_OPTION_DURATION},
};

static bool is_immediate_change(
  const mln_runtime_event* event, const char* messages, void* context
) {
  (void)messages;
  return event->type == MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE &&
         event->code == MLN_CAMERA_CHANGE_MODE_IMMEDIATE &&
         event->generation == *(const uint64_t*)context;
}

// A command with nothing to animate completes within itself: its handler runs
// before its completion, with the generation that the command published, and
// that generation's immediate camera change is queued.
static void an_immediate_command_completes_before_its_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  const size_t case_count = sizeof(immediate_cases) / sizeof(*immediate_cases);
  for (size_t index = 0; index < case_count; index += 1) {
    const immediate_case* row = &immediate_cases[index];
    mln_test_transition_end probe;
    mln_test_transition_end_init(&probe);
    mln_camera_update update = mln_camera_update_default();
    update.mode = row->mode;
    update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
    update.camera.zoom = 3.0 + (double)index;
    update.animation.fields = row->animation_fields;
    update.animation.end_handler = mln_test_transition_end_handler(&probe);
    const uint64_t generation = update_camera_generation(map, &update);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, atomic_load(&probe.calls), row->label);
    assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, row->label);
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(
      generation, atomic_load(&probe.generation), row->label
    );
    uint64_t expected = generation;
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      1,
      mln_test_drain_counting_matching(runtime, is_immediate_change, &expected),
      row->label
    );
  }

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The handler reports the end whatever the event mask selects.
static void the_handler_runs_whatever_the_event_mask_selects(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.event_mask = MLN_RUNTIME_EVENT_MASK_NONE;
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  const mln_camera_update eased = ease_to_zoom(6.0, 60000.0, &probe);
  update_camera(map, &eased);
  mln_camera_update jump = mln_camera_update_default();
  jump.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  jump.camera.zoom = 3.0;
  const uint64_t generation = update_camera_generation(map, &jump);
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "replaced");
  TEST_ASSERT_EQUAL_UINT64(generation, atomic_load(&probe.generation));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Without a render session a transition never advances, so each outcome below
// ends a transition that is still running.
static void cancel_transitions_commits_and_leaves_the_camera(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = mln_camera_update_default();
  update.camera = test_camera();
  update_camera(map, &update);

  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  const mln_camera_update eased = ease_to_zoom(18.0, 60000.0, &probe);
  update_camera(map, &eased);
  mln_test_completion cancel = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_cancel_transitions(map, &cancel.descriptor, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&cancel));
  // The cancelled transition reports its end before the command completes,
  // and leaves the camera where it stopped, short of the eased target.
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.calls));
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "cancelled");
  TEST_ASSERT_EQUAL_UINT64(
    mln_test_completion_generation(&cancel), atomic_load(&probe.generation)
  );
  mln_test_completion_destroy(&cancel);
  const double settled = read_settled_snapshot(runtime, map).camera.zoom;
  TEST_ASSERT_TRUE(settled < 18.0);
  TEST_ASSERT_EQUAL_DOUBLE(
    settled, read_settled_snapshot(runtime, map).camera.zoom
  );

  // Cancelling with nothing running commits and changes nothing.
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_transitions(map, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_DOUBLE(
    settled, read_settled_snapshot(runtime, map).camera.zoom
  );
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.calls));

  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(
    mln_map_cancel_transitions(MLN_HANDLE_NULL, &rejected, NULL)
  );
  MLN_TEST_INVALID(mln_map_cancel_transitions(map, NULL, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Cancelling an identity ends only the commands that carry it. Another
// command keeps animating, and a later camera write that replaces it
// completes it.
static void cancel_camera_transition_ends_only_its_identity(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = mln_camera_update_default();
  update.camera = test_camera();
  update_camera(map, &update);

  // Two commands share the zoom's identity, and the bearing has its own.
  mln_test_transition_end zoom;
  mln_test_transition_end_init(&zoom);
  mln_camera_update zoom_ease = ease_to_zoom(18.0, 60000.0, &zoom);
  with_transition_id(&zoom_ease.animation, UINT64_MAX);
  update_camera(map, &zoom_ease);
  mln_test_transition_end pitch;
  mln_test_transition_end_init(&pitch);
  mln_camera_update pitch_ease = mln_camera_update_default();
  pitch_ease.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  pitch_ease.camera.fields = MLN_CAMERA_OPTION_PITCH;
  pitch_ease.camera.pitch = 50.0;
  pitch_ease.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  pitch_ease.animation.duration_ms = 60000.0;
  pitch_ease.animation.end_handler = mln_test_transition_end_handler(&pitch);
  with_transition_id(&pitch_ease.animation, UINT64_MAX);
  update_camera(map, &pitch_ease);
  mln_test_transition_end bearing;
  mln_test_transition_end_init(&bearing);
  mln_camera_update bearing_ease = mln_camera_update_default();
  bearing_ease.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  bearing_ease.camera.fields = MLN_CAMERA_OPTION_BEARING;
  bearing_ease.camera.bearing = 90.0;
  bearing_ease.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  bearing_ease.animation.duration_ms = 60000.0;
  bearing_ease.animation.end_handler =
    mln_test_transition_end_handler(&bearing);
  with_transition_id(&bearing_ease.animation, 7);
  update_camera(map, &bearing_ease);

  MLN_TEST_AWAIT_OK(mln_map_cancel_camera_transition(
    map, UINT64_MAX, &completion.descriptor, NULL
  ));
  assert_ended(&zoom, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "zoom");
  assert_ended(&pitch, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "pitch");
  const mln_map_snapshot cancelled = read_settled_snapshot(runtime, map);
  TEST_ASSERT_TRUE(cancelled.camera.zoom < 18.0);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&bearing.calls));

  // An identity that no running command carries changes nothing.
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_camera_transition(map, 8, &completion.descriptor, NULL)
  );
  MLN_TEST_AWAIT_OK(mln_map_cancel_camera_transition(
    map, UINT64_MAX, &completion.descriptor, NULL
  ));
  const mln_map_snapshot unchanged = read_settled_snapshot(runtime, map);
  TEST_ASSERT_EQUAL_DOUBLE(cancelled.camera.zoom, unchanged.camera.zoom);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&bearing.calls));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&zoom.calls));

  mln_camera_update jump = mln_camera_update_default();
  jump.camera.fields = MLN_CAMERA_OPTION_BEARING;
  jump.camera.bearing = 45.0;
  update_camera(map, &jump);
  assert_ended(&bearing, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "bearing");

  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(
    mln_map_cancel_camera_transition(MLN_HANDLE_NULL, 7, &rejected, NULL)
  );
  MLN_TEST_INVALID(mln_map_cancel_camera_transition(map, 7, NULL, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A gesture phase only marks the map, except CANCEL, which also ends the
// transitions still running after its camera write.
static void gesture_phase_publishes_the_snapshot_flag(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_FALSE(read_settled_snapshot(runtime, map).gesture_in_progress);

  mln_camera_update update = mln_camera_update_default();
  update.camera = test_camera();
  update.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  update_camera(map, &update);
  TEST_ASSERT_TRUE(read_settled_snapshot(runtime, map).gesture_in_progress);

  update.gesture_phase = MLN_GESTURE_PHASE_UPDATE;
  update.camera.zoom = 12.0;
  update_camera(map, &update);
  TEST_ASSERT_TRUE(read_settled_snapshot(runtime, map).gesture_in_progress);

  update.gesture_phase = MLN_GESTURE_PHASE_END;
  update_camera(map, &update);
  mln_map_snapshot snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_FALSE(snapshot.gesture_in_progress);
  TEST_ASSERT_EQUAL_DOUBLE(12.0, snapshot.camera.zoom);

  // BEGIN leaves a running ease alone, and CANCEL ends it.
  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  const mln_camera_update eased = ease_to_zoom(3.0, 60000.0, &probe);
  update_camera(map, &eased);
  mln_camera_update gesture = mln_camera_update_default();
  gesture.camera.fields = MLN_CAMERA_OPTION_BEARING;
  gesture.camera.bearing = 20.0;
  gesture.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  update_camera(map, &gesture);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.calls));

  gesture.gesture_phase = MLN_GESTURE_PHASE_CANCEL;
  update_camera(map, &gesture);
  TEST_ASSERT_FALSE(read_settled_snapshot(runtime, map).gesture_in_progress);
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "gesture");

  update.gesture_phase = MLN_GESTURE_PHASE_CANCEL + 1;
  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_update_camera(map, &update, &rejected, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A zero-duration flight settles at once whatever its other animation fields
// say, so the committed camera shows the target.
static void a_flight_with_every_animation_field_commits(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_FLY;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = 6.0;
  update.animation.fields =
    MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_VELOCITY |
    MLN_ANIMATION_OPTION_MIN_ZOOM | MLN_ANIMATION_OPTION_EASING;
  update.animation.duration_ms = 0.0;
  update.animation.velocity = 1.2;
  update.animation.min_zoom = 1.0;
  update.animation.easing =
    (mln_unit_bezier){.x1 = 0.25, .y1 = 0.1, .x2 = 0.25, .y2 = 1.0};
  update_camera(map, &update);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 6.0, query_camera(map).zoom);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An animated delta lasting `duration_ms` that reports its end to `probe`.
static mln_camera_delta animated_delta(
  uint32_t fields, double duration_ms, mln_test_transition_end* probe
) {
  mln_camera_delta delta = mln_camera_delta_default();
  delta.fields = fields;
  delta.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  delta.animation.duration_ms = duration_ms;
  delta.animation.end_handler = mln_test_transition_end_handler(probe);
  return delta;
}

static void apply_delta(mln_map map, const mln_camera_delta* delta) {
  MLN_TEST_AWAIT_OK(
    mln_map_apply_camera_delta(map, delta, &completion.descriptor, NULL)
  );
}

// An animated delta that pans and zooms runs as two MapLibre transitions and
// runs its handler once, after both. A rendered delta reaches its target. A
// delta whose zoom a later jump replaces keeps running until a cancellation
// ends its pan.
static void an_animated_delta_reports_its_end_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  const mln_camera_options start = query_camera(map);

  mln_test_transition_end rendered;
  mln_test_transition_end_init(&rendered);
  mln_camera_delta delta = animated_delta(
    MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE, 20.0, &rendered
  );
  delta.offset = (mln_screen_point){.x = 40.0, .y = 0.0};
  delta.scale = 2.0;
  apply_delta(map, &delta);
  render_until_ended(&fixture, &rendered);
  assert_ended(&rendered, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "rendered");
  const mln_camera_options camera = query_camera(map);
  TEST_ASSERT_DOUBLE_WITHIN(1e-4, start.zoom + 1.0, camera.zoom);
  TEST_ASSERT_TRUE(camera.center.longitude < start.center.longitude);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&rendered.calls));

  mln_test_transition_end cancelled;
  mln_test_transition_end_init(&cancelled);
  delta = animated_delta(
    MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE, 60000.0, &cancelled
  );
  delta.offset = (mln_screen_point){.x = 40.0, .y = 0.0};
  delta.scale = 2.0;
  apply_delta(map, &delta);
  // A jump that replaces the zoom ends one of the two transitions, and the
  // pan keeps the command running.
  mln_camera_update jump = mln_camera_update_default();
  jump.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  jump.camera.zoom = start.zoom;
  update_camera(map, &jump);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&cancelled.calls));
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_transitions(map, &completion.descriptor, NULL)
  );
  assert_ended(&cancelled, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "delta");

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An animated delta starts from the camera that the running transition has
// reached and replaces its transition. With no frame between them, the second
// scale starts where the first did, so the camera ends one scale away from
// the start, and the replaced command completes at once.
static void an_animated_delta_replaces_the_running_one(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  const double start_zoom = query_camera(map).zoom;

  mln_test_transition_end replaced;
  mln_test_transition_end_init(&replaced);
  mln_camera_delta first =
    animated_delta(MLN_CAMERA_DELTA_SCALE, 20.0, &replaced);
  first.scale = 4.0;
  mln_test_transition_end replacing;
  mln_test_transition_end_init(&replacing);
  mln_camera_delta second =
    animated_delta(MLN_CAMERA_DELTA_SCALE, 20.0, &replacing);
  second.scale = 4.0;
  const mln_completion discard = mln_test_discard_completion();
  MLN_TEST_OK(mln_map_apply_camera_delta(map, &first, &discard, NULL));
  apply_delta(map, &second);
  assert_ended(&replaced, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "replaced");

  render_until_ended(&fixture, &replacing);
  assert_ended(
    &replacing, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "replacing"
  );
  // An animated scale ends within rounding of its target, which varies by
  // platform.
  TEST_ASSERT_DOUBLE_WITHIN(1e-4, start_zoom + 2.0, query_camera(map).zoom);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A delta carries its gesture phase like a camera update. A delta without
// components commits only its phase, and runs its handler before its
// completion, as a release without inertia needs.
static void a_delta_carries_its_gesture_phase(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = mln_camera_update_default();
  update.camera = test_camera();
  update_camera(map, &update);

  mln_camera_delta delta = mln_camera_delta_default();
  delta.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  apply_delta(map, &delta);
  mln_map_snapshot snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_TRUE(snapshot.gesture_in_progress);
  TEST_ASSERT_EQUAL_DOUBLE(11.0, snapshot.camera.zoom);

  delta.fields = MLN_CAMERA_DELTA_SCALE;
  delta.scale = 2.0;
  delta.gesture_phase = MLN_GESTURE_PHASE_UPDATE;
  apply_delta(map, &delta);
  snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_TRUE(snapshot.gesture_in_progress);
  TEST_ASSERT_EQUAL_DOUBLE(12.0, snapshot.camera.zoom);

  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  delta = mln_camera_delta_default();
  delta.animation.end_handler = mln_test_transition_end_handler(&probe);
  delta.gesture_phase = MLN_GESTURE_PHASE_END;
  apply_delta(map, &delta);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.calls));
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "release");
  snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_FALSE(snapshot.gesture_in_progress);
  TEST_ASSERT_EQUAL_DOUBLE(12.0, snapshot.camera.zoom);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Closing a map ends each command that it still animates as closed, with no
// generation, before the release completes. Disposal does the same.
static void a_map_close_ends_running_transitions(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_map_create_status(runtime, NULL, &map));
  mln_test_transition_end released;
  mln_test_transition_end_init(&released);
  const mln_camera_update eased = ease_to_zoom(6.0, 60000.0, &released);
  update_camera(map, &eased);
  mln_test_completion release = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_release(map, &release.descriptor, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&release));
  mln_test_completion_destroy(&release);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&released.calls));
  assert_ended(&released, MLN_CAMERA_TRANSITION_OUTCOME_CLOSED, "release");
  TEST_ASSERT_EQUAL_UINT64(0, atomic_load(&released.generation));

  mln_map disposed = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_map_create_status(runtime, NULL, &disposed));
  mln_test_transition_end dispose;
  mln_test_transition_end_init(&dispose);
  const mln_camera_update dispose_ease = ease_to_zoom(6.0, 60000.0, &dispose);
  update_camera(disposed, &dispose_ease);
  MLN_TEST_OK(mln_map_dispose(disposed, NULL));
  assert_ended(&dispose, MLN_CAMERA_TRANSITION_OUTCOME_CLOSED, "dispose");
  TEST_ASSERT_EQUAL_UINT64(0, atomic_load(&dispose.generation));

  mln_test_destroy_runtime(runtime);
}

typedef struct submitting_end {
  mln_map map;
  atomic_int status;
} submitting_end;

// Submits a jump from inside the end callback.
static void submit_from_end(
  mln_test_transition_end* probe, const mln_camera_transition_end* end
) {
  (void)end;
  submitting_end* state = probe->context;
  mln_camera_update jump = mln_camera_update_default();
  jump.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  jump.camera.zoom = 9.0;
  const mln_completion discard = mln_test_discard_completion();
  atomic_store(
    &state->status,
    (int)mln_map_update_camera(state->map, &jump, &discard, NULL)
  );
}

// The handler may submit a command, which runs after the one that ended the
// transition.
static void a_handler_may_submit_a_command(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  submitting_end state = {.map = map};
  atomic_init(&state.status, -1);
  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  probe.hook = submit_from_end;
  probe.context = &state;
  const mln_camera_update eased = ease_to_zoom(4.0, 0.0, &probe);
  update_camera(map, &eased);
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "submitting");
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&state.status));
  TEST_ASSERT_EQUAL_DOUBLE(
    9.0, read_settled_snapshot(runtime, map).camera.zoom
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Default animation options disable the handler. A rejected command runs
// neither callback and leaves user_data with the caller, and a disabled
// handler must not carry a release.
static void a_rejected_command_runs_no_handler(void) {
  const mln_animation_options defaults = mln_animation_options_default();
  TEST_ASSERT_NULL(defaults.end_handler.callback);
  TEST_ASSERT_NULL(defaults.end_handler.release_user_data);

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  mln_completion rejected = mln_test_discard_completion();

  mln_camera_update update = ease_to_zoom(4.0, 60000.0, &probe);
  update.gesture_phase = MLN_GESTURE_PHASE_CANCEL + 1;
  MLN_TEST_INVALID(mln_map_update_camera(map, &update, &rejected, NULL));
  mln_camera_delta delta = animated_delta(MLN_CAMERA_DELTA_SCALE, 10.0, &probe);
  delta.scale = -1.0;
  MLN_TEST_INVALID(mln_map_apply_camera_delta(map, &delta, &rejected, NULL));
  update = ease_to_zoom(4.0, 60000.0, &probe);
  MLN_TEST_INVALID(
    mln_map_update_camera(MLN_HANDLE_NULL, &update, &rejected, NULL)
  );
  update.animation.end_handler.callback = NULL;
  MLN_TEST_INVALID(mln_map_update_camera(map, &update, &rejected, NULL));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.calls));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.releases));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_rendered_ease_completes_at_its_target);
  RUN_TEST(an_immediate_command_completes_before_its_completion);
  RUN_TEST(the_handler_runs_whatever_the_event_mask_selects);
  RUN_TEST(a_flight_with_every_animation_field_commits);
  RUN_TEST(cancel_transitions_commits_and_leaves_the_camera);
  RUN_TEST(cancel_camera_transition_ends_only_its_identity);
  RUN_TEST(gesture_phase_publishes_the_snapshot_flag);
  RUN_TEST(an_animated_delta_reports_its_end_once);
  RUN_TEST(an_animated_delta_replaces_the_running_one);
  RUN_TEST(a_delta_carries_its_gesture_phase);
  RUN_TEST(a_map_close_ends_running_transitions);
  RUN_TEST(a_handler_may_submit_a_command);
  RUN_TEST(a_rejected_command_runs_no_handler);
}
