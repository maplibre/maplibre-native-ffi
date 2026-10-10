// Camera transitions and gestures: an ease or an animated delta advances only
// as frames render and reports its end once, a cancellation stops it where it
// stands, and gesture phases mark the map around a camera write.

#include "support/test_support.h"

static mln_camera_options test_camera(void) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
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

// An ease toward `zoom` that lasts `duration_ms` and reports its end under
// `transition_id`.
static mln_camera_update ease_to_zoom(
  double zoom, double duration_ms, uint64_t transition_id
) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  update.animation.fields =
    MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_TRANSITION_ID;
  update.animation.duration_ms = duration_ms;
  update.animation.transition_id = transition_id;
  return update;
}

static mln_map_snapshot read_settled_snapshot(
  mln_runtime runtime, mln_map map
) {
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_snapshot_get(map, &snapshot, NULL));
  return snapshot;
}

// An ordered camera read. The transition-finished event is queued from inside
// the worker task that applies the last frame, before that task publishes the
// map snapshot, so only a worker-ordered read observes the final camera as soon
// as the event is drained.
static mln_camera_options query_camera(mln_map map) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_query_result));
  MLN_TEST_OK(mln_map_camera_query(map, &query.descriptor, NULL));
  mln_camera_query_result result = {.size = sizeof(mln_camera_query_result)};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &result, sizeof(result))
  );
  return result.camera;
}

typedef struct transition_wait {
  mln_runtime runtime;
  const mln_test_render_fixture* fixture;
  uint64_t transition_id;
  bool demand_pending;
  size_t finished;
  bool failed;
} transition_wait;

// Accepts the transition-finished events for the ID that `context` points to.
static bool is_finished_transition(
  const mln_runtime_event* event, const char* messages, void* context
) {
  (void)messages;
  return event->type == MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED &&
         event->payload.camera_transition_finished.transition_id ==
           *(const uint64_t*)context;
}

// Drains the queue and counts the transition-finished events for one ID.
static size_t drain_finished(mln_runtime runtime, uint64_t transition_id) {
  return mln_test_drain_counting_matching(
    runtime, is_finished_transition, &transition_id
  );
}

// Keeps one frame demand in flight until the transition reports its end. Each
// rendered frame advances the transition on MapLibre's clock.
static bool transition_finished(void* context) {
  transition_wait* wait = context;
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
  wait->finished += drain_finished(wait->runtime, wait->transition_id);
  if (wait->finished > 0) {
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

// An ease with a duration completes only while frames render: it reaches its
// target, reports its transition ID once, and a later frame, the fence for the
// negative check, reports nothing more.
static void a_rendered_ease_completes_at_its_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_drain_all(runtime);

  const mln_camera_update ease = ease_to_zoom(5.0, 20.0, 31);
  update_camera(map, &ease);

  transition_wait wait = {
    .runtime = runtime, .fixture = &fixture, .transition_id = 31
  };
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, transition_finished, &wait, mln_test_deadline_default(),
    "a transition-finished event"
  ));
  TEST_ASSERT_FALSE(wait.failed);
  TEST_ASSERT_EQUAL_size_t(1, wait.finished);
  TEST_ASSERT_EQUAL_DOUBLE(5.0, query_camera(map).zoom);

  // transition_finished collects its demand before it reports the end, so no
  // frame is in flight here.
  TEST_ASSERT_FALSE(wait.demand_pending);
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, settled_result, (void*)&fixture, mln_test_deadline_default(),
    "a fence frame"
  ));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(0, drain_finished(runtime, 31));

  mln_test_render_fixture_destroy(&fixture);
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
  mln_test_drain_all(runtime);

  const mln_camera_update eased = ease_to_zoom(18.0, 60000.0, 41);
  update_camera(map, &eased);
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_transitions(map, &completion.descriptor, NULL)
  );
  // The cancelled transition reports its end, leaves the camera where it
  // stopped, short of the eased target, and the camera stays there.
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(1, drain_finished(runtime, 41));
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

  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(
    mln_map_cancel_transitions(MLN_HANDLE_NULL, &rejected, NULL)
  );
  MLN_TEST_INVALID(mln_map_cancel_transitions(map, NULL, NULL));
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
  mln_test_drain_all(runtime);
  const mln_camera_update eased = ease_to_zoom(3.0, 60000.0, 51);
  update_camera(map, &eased);
  mln_camera_update gesture = mln_camera_update_default();
  gesture.camera.fields = MLN_CAMERA_OPTION_BEARING;
  gesture.camera.bearing = 20.0;
  gesture.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  update_camera(map, &gesture);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(0, drain_finished(runtime, 51));

  gesture.gesture_phase = MLN_GESTURE_PHASE_CANCEL;
  update_camera(map, &gesture);
  TEST_ASSERT_FALSE(read_settled_snapshot(runtime, map).gesture_in_progress);
  TEST_ASSERT_EQUAL_size_t(1, drain_finished(runtime, 51));

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

// An animated delta lasting `duration_ms` that reports its end under
// `transition_id`.
static mln_camera_delta animated_delta(
  uint32_t fields, double duration_ms, uint64_t transition_id
) {
  mln_camera_delta delta = mln_camera_delta_default();
  delta.fields = fields;
  delta.animation.fields =
    MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_TRANSITION_ID;
  delta.animation.duration_ms = duration_ms;
  delta.animation.transition_id = transition_id;
  return delta;
}

static void apply_delta(mln_map map, const mln_camera_delta* delta) {
  MLN_TEST_AWAIT_OK(
    mln_map_apply_camera_delta(map, delta, &completion.descriptor, NULL)
  );
}

// Renders frames until the transition reports its end, and returns how many
// times it did.
static size_t render_until_finished(
  mln_runtime runtime, const mln_test_render_fixture* fixture,
  uint64_t transition_id
) {
  transition_wait wait = {
    .runtime = runtime, .fixture = fixture, .transition_id = transition_id
  };
  MLN_TEST_OK(mln_test_render_step_until(
    fixture, transition_finished, &wait, mln_test_deadline_default(),
    "a transition-finished event"
  ));
  TEST_ASSERT_FALSE(wait.failed);
  TEST_ASSERT_FALSE(wait.demand_pending);
  return wait.finished;
}

// An animated delta that pans and zooms runs as two MapLibre transitions and
// reports its end once, after both. A rendered delta reaches its target, and a
// cancelled one ends both transitions in one step, where reporting each of them
// would show.
static void an_animated_delta_reports_its_end_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  const mln_camera_options start = query_camera(map);
  mln_test_drain_all(runtime);

  mln_camera_delta delta =
    animated_delta(MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE, 20.0, 71);
  delta.offset = (mln_screen_point){.x = 40.0, .y = 0.0};
  delta.scale = 2.0;
  apply_delta(map, &delta);
  TEST_ASSERT_EQUAL_size_t(1, render_until_finished(runtime, &fixture, 71));
  const mln_camera_options rendered = query_camera(map);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, start.zoom + 1.0, rendered.zoom);
  TEST_ASSERT_TRUE(rendered.longitude < start.longitude);

  delta = animated_delta(
    MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE, 60000.0, 72
  );
  delta.offset = (mln_screen_point){.x = 40.0, .y = 0.0};
  delta.scale = 2.0;
  apply_delta(map, &delta);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(0, drain_finished(runtime, 72));
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_transitions(map, &completion.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(1, drain_finished(runtime, 72));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An animated delta starts from the camera that the running transition has
// reached and replaces its transition. With no frame between them, the second
// scale starts where the first did, so the camera ends one scale away from
// the start, and the replaced transition reports its end at once.
static void an_animated_delta_replaces_the_running_one(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  const double start_zoom = query_camera(map).zoom;
  mln_test_drain_all(runtime);

  mln_camera_delta first = animated_delta(MLN_CAMERA_DELTA_SCALE, 20.0, 61);
  first.scale = 4.0;
  mln_camera_delta second = animated_delta(MLN_CAMERA_DELTA_SCALE, 20.0, 62);
  second.scale = 4.0;
  const mln_completion discard = mln_test_discard_completion();
  MLN_TEST_OK(mln_map_apply_camera_delta(map, &first, &discard, NULL));
  apply_delta(map, &second);
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_size_t(1, drain_finished(runtime, 61));

  TEST_ASSERT_EQUAL_size_t(1, render_until_finished(runtime, &fixture, 62));
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, start_zoom + 2.0, query_camera(map).zoom);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A delta carries its gesture phase like a camera update. A delta without
// components commits only its phase.
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
  TEST_ASSERT_TRUE(read_settled_snapshot(runtime, map).gesture_in_progress);

  delta.gesture_phase = MLN_GESTURE_PHASE_END;
  apply_delta(map, &delta);
  snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_FALSE(snapshot.gesture_in_progress);
  TEST_ASSERT_EQUAL_DOUBLE(13.0, snapshot.camera.zoom);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_rendered_ease_completes_at_its_target);
  RUN_TEST(a_flight_with_every_animation_field_commits);
  RUN_TEST(cancel_transitions_commits_and_leaves_the_camera);
  RUN_TEST(gesture_phase_publishes_the_snapshot_flag);
  RUN_TEST(an_animated_delta_reports_its_end_once);
  RUN_TEST(an_animated_delta_replaces_the_running_one);
  RUN_TEST(a_delta_carries_its_gesture_phase);
}
