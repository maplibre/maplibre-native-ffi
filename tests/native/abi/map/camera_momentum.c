// Gesture momentum: the motion, the gesture it extends, and how it ends.

#include <math.h>

#include "support/camera.h"
#include "support/frames.h"
#include "support/test_support.h"

static void apply_delta(mln_map map, const mln_camera_delta* delta) {
  MLN_TEST_AWAIT_OK(
    mln_map_apply_camera_delta(map, delta, &completion.descriptor, NULL)
  );
}

static void update_camera(mln_map map, const mln_camera_update* update) {
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, update, &completion.descriptor, NULL)
  );
}

static mln_map_snapshot read_snapshot(mln_map map) {
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  return snapshot;
}

static void begin_gesture(mln_map map) {
  mln_camera_delta delta = {.size = sizeof(mln_camera_delta)};
  delta.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  apply_delta(map, &delta);
}

// An end-phase delta that carries `momentum` and reports its end to `probe`.
static mln_camera_delta release(
  const mln_gesture_momentum* momentum, mln_test_transition_end* probe
) {
  mln_camera_delta delta = {.size = sizeof(mln_camera_delta)};
  delta.fields = MLN_CAMERA_DELTA_MOMENTUM;
  delta.momentum = *momentum;
  delta.gesture_phase = MLN_GESTURE_PHASE_END;
  delta.animation.end_handler = mln_test_transition_end_handler(probe);
  return delta;
}

typedef struct source_search {
  uint32_t type;
  mln_camera_change_source source;
  bool found;
} source_search;

static void find_source(const mln_runtime_event* event, void* context) {
  source_search* search = context;
  if (
    event->type == search->type &&
    event->payload_type == MLN_RUNTIME_EVENT_PAYLOAD_CAMERA_CHANGE &&
    event->payload.camera_change.source == search->source
  ) {
    search->found = true;
  }
}

// What a momentum's end handler saw from inside its callback.
typedef struct end_observation {
  mln_runtime runtime;
  mln_camera_velocity velocity;
  bool momentum_change_queued;
} end_observation;

// The callback runs after the MapLibre update that ended the momentum, so the
// camera events of that update are queued.
static void observe_end(
  mln_test_transition_end* probe, const mln_camera_transition_end* end
) {
  end_observation* observation = probe->context;
  observation->velocity = end->velocity;
  if (observation->runtime == MLN_HANDLE_NULL) return;
  source_search search = {
    .type = MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE,
    .source = MLN_CAMERA_CHANGE_SOURCE_MOMENTUM,
  };
  (void)mln_test_dispatch_events(observation->runtime, find_source, &search);
  observation->momentum_change_queued = search.found;
}

static void assert_ended(mln_test_transition_end* probe, uint32_t outcome) {
  TEST_ASSERT_TRUE(mln_test_wait_transition_end(probe));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe->calls));
  TEST_ASSERT_EQUAL_UINT32(outcome, atomic_load(&probe->outcome));
}

typedef struct render_wait {
  const mln_test_render_fixture* fixture;
  mln_test_transition_end* probe;
  bool demand_pending;
  bool failed;
} render_wait;

// Keeps one frame demand in flight until the handler releases. Each rendered
// frame advances the momentum on MapLibre's clock.
static bool momentum_ended(void* context) {
  render_wait* wait = context;
  if (wait->demand_pending) {
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    if (
      mln_test_drain_frame_results(wait->fixture->session, &batch, NULL) !=
      MLN_STATUS_OK
    ) {
      wait->failed = true;
      return true;
    }
    if (batch == MLN_HANDLE_NULL) return false;
    mln_render_frame_batch_release(batch);
    wait->demand_pending = false;
  }
  if (atomic_load(&wait->probe->releases) > 0) return true;
  mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
  if (
    mln_render_session_demand_frame(wait->fixture->session, &demand, NULL) !=
    MLN_STATUS_OK
  ) {
    wait->failed = true;
    return true;
  }
  wait->demand_pending = true;
  return false;
}

static void render_until_ended(
  const mln_test_render_fixture* fixture, mln_test_transition_end* probe
) {
  render_wait wait = {.fixture = fixture, .probe = probe};
  MLN_TEST_OK(mln_test_render_step_until(
    fixture, momentum_ended, &wait, mln_test_deadline_default(),
    "a momentum end handler"
  ));
  TEST_ASSERT_FALSE(wait.failed);
}

// The screen point where `coordinate` appears in the map's current view.
static mln_screen_point screen_point_of(mln_map map, mln_lat_lng coordinate) {
  const mln_map_snapshot snapshot = read_snapshot(map);
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_map_projection_create(&snapshot.view, &projection, NULL));
  mln_screen_point point = {0};
  MLN_TEST_OK(
    mln_map_projection_pixel_for_lat_lng(projection, coordinate, &point, NULL)
  );
  mln_map_projection_release(projection);
  return point;
}

typedef struct momentum_fixture {
  mln_runtime runtime;
  mln_map map;
  mln_test_render_fixture render;
} momentum_fixture;

// Fills `fixture` where it stays: the render session's wakes count into it.
static void create_fixture(momentum_fixture* fixture) {
  *fixture = (momentum_fixture){.runtime = mln_test_create_runtime()};
  fixture->map = mln_test_create_map(fixture->runtime);
  mln_test_load_style_and_wait(fixture->map, mln_test_background_style_json);
  TEST_ASSERT_TRUE(
    mln_test_render_fixture_create_offscreen(fixture->map, &fixture->render)
  );
}

static void destroy_fixture(momentum_fixture* fixture) {
  mln_test_render_fixture_destroy(&fixture->render);
  mln_test_destroy_map(fixture->map);
  mln_test_destroy_runtime(fixture->runtime);
}

// The content moves (v - s) / k, for a release at speed v that stops at speed
// s and slows at rate k = ln 2 / half-life. A short half-life keeps the glide
// to a few frames.
static void a_release_glides_and_then_ends_the_gesture(void) {
  momentum_fixture fixture;
  create_fixture(&fixture);
  const mln_lat_lng center = read_snapshot(fixture.map).view.camera.center;
  const mln_screen_point before = screen_point_of(fixture.map, center);

  begin_gesture(fixture.map);
  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  end_observation observation = {.runtime = fixture.runtime};
  probe.hook = observe_end;
  probe.context = &observation;
  mln_gesture_momentum momentum = {0};
  momentum.pan.fields = MLN_PAN_MOMENTUM_VELOCITY;
  momentum.pan.velocity = (mln_screen_point){.x = 600.0, .y = 800.0};
  momentum.pan.decay.fields =
    MLN_MOMENTUM_DECAY_HALF_LIFE_MS | MLN_MOMENTUM_DECAY_STOP_SPEED;
  momentum.pan.decay.half_life_ms = 20.0;
  momentum.pan.decay.stop_speed = 100.0;
  const mln_camera_delta delta = release(&momentum, &probe);
  apply_delta(fixture.map, &delta);
  TEST_ASSERT_TRUE(read_snapshot(fixture.map).gesture_in_progress);

  render_until_ended(&fixture.render, &probe);
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, observation.velocity.pan.x);
  TEST_ASSERT_TRUE(observation.momentum_change_queued);
  TEST_ASSERT_FALSE(read_snapshot(fixture.map).gesture_in_progress);

  const double distance = (1000.0 - 100.0) / (0.69314718055994530942 / 0.02);
  const mln_screen_point after = screen_point_of(fixture.map, center);
  TEST_ASSERT_DOUBLE_WITHIN(0.5, before.x + 0.6 * distance, after.x);
  TEST_ASSERT_DOUBLE_WITHIN(0.5, before.y + 0.8 * distance, after.y);

  destroy_fixture(&fixture);
}

// A new gesture stops the momentum and reports each part's speed, while the
// new gesture keeps the map's gesture flag. With a ten-minute half-life, a
// frame that runs before the new gesture barely slows the parts.
static void a_new_gesture_stops_momentum_and_reports_its_speed(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  begin_gesture(map);
  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  end_observation observation = {0};
  probe.hook = observe_end;
  probe.context = &observation;
  mln_gesture_momentum momentum = {0};
  momentum.pan.fields = MLN_PAN_MOMENTUM_VELOCITY;
  momentum.pan.velocity = (mln_screen_point){.x = 1000.0, .y = -500.0};
  momentum.zoom.fields = MLN_ANCHORED_MOMENTUM_VELOCITY;
  momentum.zoom.velocity = 1.0;
  momentum.bearing.fields = MLN_ANCHORED_MOMENTUM_VELOCITY;
  momentum.bearing.velocity = -90.0;
  momentum.pitch.fields = MLN_ANCHORED_MOMENTUM_VELOCITY;
  momentum.pitch.velocity = 20.0;
  mln_momentum_decay* decays[] = {
    &momentum.pan.decay, &momentum.zoom.decay, &momentum.bearing.decay,
    &momentum.pitch.decay
  };
  for (size_t i = 0; i < sizeof(decays) / sizeof(*decays); ++i) {
    decays[i]->fields =
      MLN_MOMENTUM_DECAY_HALF_LIFE_MS | MLN_MOMENTUM_DECAY_MAX_DISTANCE;
    decays[i]->half_life_ms = 600000.0;
    decays[i]->max_distance = 1e9;
  }
  const mln_camera_delta delta = release(&momentum, &probe);
  apply_delta(map, &delta);
  mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_TRUE(snapshot.gesture_in_progress);
  TEST_ASSERT_TRUE(snapshot.transition_in_progress);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.calls));

  begin_gesture(map);
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED);
  const mln_camera_velocity velocity = observation.velocity;
  TEST_ASSERT_DOUBLE_WITHIN(1.0, 1000.0, velocity.pan.x);
  TEST_ASSERT_DOUBLE_WITHIN(0.5, -500.0, velocity.pan.y);
  TEST_ASSERT_DOUBLE_WITHIN(1e-3, 1.0, velocity.zoom);
  TEST_ASSERT_DOUBLE_WITHIN(0.09, -90.0, velocity.bearing);
  TEST_ASSERT_DOUBLE_WITHIN(0.02, 20.0, velocity.pitch);
  snapshot = read_snapshot(map);
  TEST_ASSERT_TRUE(snapshot.gesture_in_progress);
  TEST_ASSERT_FALSE(snapshot.transition_in_progress);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_gesture_momentum long_pan_and_zoom(void) {
  mln_gesture_momentum momentum = {0};
  momentum.pan.fields = MLN_PAN_MOMENTUM_VELOCITY;
  momentum.pan.velocity = (mln_screen_point){.x = 1000.0, .y = 0.0};
  momentum.pan.decay.fields = MLN_MOMENTUM_DECAY_HALF_LIFE_MS;
  momentum.pan.decay.half_life_ms = 600000.0;
  momentum.zoom.fields = MLN_ANCHORED_MOMENTUM_VELOCITY;
  momentum.zoom.velocity = 1.0;
  momentum.zoom.decay.fields = MLN_MOMENTUM_DECAY_HALF_LIFE_MS;
  momentum.zoom.decay.half_life_ms = 600000.0;
  return momentum;
}

// A command that sets a field stops only the parts that move it, and the
// momentum reports the replacement once its last part stops.
static void a_command_replaces_the_parts_it_moves(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  begin_gesture(map);
  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  const mln_gesture_momentum momentum = long_pan_and_zoom();
  const mln_camera_delta delta = release(&momentum, &probe);
  apply_delta(map, &delta);

  mln_camera_update jump = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  jump.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  jump.camera.zoom = 3.0;
  update_camera(map, &jump);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.calls));
  TEST_ASSERT_TRUE(read_snapshot(map).gesture_in_progress);

  jump.camera.fields = MLN_CAMERA_OPTION_CENTER;
  jump.camera.center = (mln_lat_lng){.latitude = 10.0, .longitude = 20.0};
  update_camera(map, &jump);
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_REPLACED);
  const mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_FALSE(snapshot.gesture_in_progress);
  TEST_ASSERT_FALSE(snapshot.transition_in_progress);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A release below the default minimum pan speed ends the gesture at once.
static void a_slow_release_ends_the_gesture_at_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  begin_gesture(map);
  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  mln_gesture_momentum momentum = {0};
  momentum.pan.fields = MLN_PAN_MOMENTUM_VELOCITY;
  momentum.pan.velocity = (mln_screen_point){.x = 150.0, .y = 0.0};
  const mln_camera_delta delta = release(&momentum, &probe);
  apply_delta(map, &delta);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.calls));
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED);
  const mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_FALSE(snapshot.gesture_in_progress);
  TEST_ASSERT_FALSE(snapshot.transition_in_progress);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The bearing settles on the nearest target after a gesture that rotated the
// map, and stays after one that didn't.
static void a_rotating_gesture_settles_on_a_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_gesture_momentum momentum = {0};
  momentum.bearing.fields = MLN_ANCHORED_MOMENTUM_SNAP;
  momentum.bearing.snap.fields =
    MLN_MOMENTUM_SNAP_SPACING | MLN_MOMENTUM_SNAP_DURATION_MS;
  momentum.bearing.snap.spacing = 90.0;
  momentum.bearing.snap.duration_ms = 0.0;

  mln_camera_delta rotate = {.size = sizeof(mln_camera_delta)};
  rotate.fields = MLN_CAMERA_DELTA_BEARING;
  rotate.bearing = 85.0;
  rotate.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  apply_delta(map, &rotate);
  mln_test_transition_end snapped;
  mln_test_transition_end_init(&snapped);
  const mln_camera_delta settle = release(&momentum, &snapped);
  apply_delta(map, &settle);
  assert_ended(&snapped, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED);
  mln_map_snapshot snapshot = read_snapshot(map);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 90.0, snapshot.view.camera.bearing);
  TEST_ASSERT_FALSE(snapshot.gesture_in_progress);

  mln_camera_update turn = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  turn.camera.fields = MLN_CAMERA_OPTION_BEARING;
  turn.camera.bearing = 87.0;
  update_camera(map, &turn);
  begin_gesture(map);
  mln_test_transition_end kept;
  mln_test_transition_end_init(&kept);
  const mln_camera_delta stay = release(&momentum, &kept);
  apply_delta(map, &stay);
  assert_ended(&kept, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED);
  snapshot = read_snapshot(map);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 87.0, snapshot.view.camera.bearing);
  TEST_ASSERT_FALSE(snapshot.gesture_in_progress);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_release_glides_and_then_ends_the_gesture);
  RUN_TEST(a_new_gesture_stops_momentum_and_reports_its_speed);
  RUN_TEST(a_command_replaces_the_parts_it_moves);
  RUN_TEST(a_slow_release_ends_the_gesture_at_once);
  RUN_TEST(a_rotating_gesture_settles_on_a_target);
}
