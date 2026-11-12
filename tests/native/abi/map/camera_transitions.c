// Camera transitions, animated deltas, and gesture phases.

#include "support/camera.h"
#include "support/frames.h"
#include "support/test_support.h"

static mln_camera_options test_camera(void) {
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
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
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
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

static void with_transition_id(
  mln_animation_options* animation, uint64_t transition_id
) {
  animation->fields |= MLN_ANIMATION_OPTION_TRANSITION_ID;
  animation->transition_id = transition_id;
}

static mln_map_snapshot read_settled_snapshot(
  mln_runtime runtime, mln_map map
) {
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  return snapshot;
}

static mln_camera query_camera(mln_map map) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_query_result));
  MLN_TEST_OK(mln_map_get_camera(map, &query.descriptor, NULL));
  mln_camera_query_result result = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &result, sizeof(result))
  );
  return result.camera;
}

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
  bool snapshot_read;
  uint64_t snapshot_generation;
  double snapshot_zoom;
} end_observation;

typedef struct did_change_search {
  end_observation* observation;
  uint64_t generation;
} did_change_search;

static void find_did_change(const mln_runtime_event* event, void* context) {
  did_change_search* search = context;
  if (
    event->type == MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE &&
    event->generation == search->generation
  ) {
    search->observation->did_change_queued = true;
  }
}

// The callback runs after the MapLibre update that ended the transition
// returns, so the camera events of that update are queued.
static void observe_end(
  mln_test_transition_end* probe, const mln_camera_transition_end* end
) {
  end_observation* observation = probe->context;
  did_change_search search = {
    .observation = observation, .generation = end->generation
  };
  (void)mln_test_dispatch_events(
    observation->runtime, find_did_change, &search
  );
  mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
  if (
    mln_map_get_snapshot(observation->map, &snapshot, NULL) == MLN_STATUS_OK
  ) {
    observation->snapshot_read = true;
    observation->snapshot_generation = snapshot.generation;
    observation->snapshot_zoom = snapshot.view.camera.zoom;
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
    const mln_status drained =
      mln_test_drain_frame_results(wait->fixture->session, &batch, NULL);
    if (drained != MLN_STATUS_OK) {
      wait->failed = true;
      return true;
    }
    if (batch == MLN_HANDLE_NULL) return false;
    mln_render_frame_batch_release(batch);
    wait->demand_pending = false;
  }
  if (atomic_load(&wait->probe->releases) > 0) {
    return true;
  }
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
    mln_test_drain_frame_results(fixture->session, &batch, NULL) !=
      MLN_STATUS_OK ||
    batch == MLN_HANDLE_NULL
  ) {
    return false;
  }
  mln_render_frame_batch_release(batch);
  return true;
}

// A later frame is the fence for checking that the handler runs only once.
static void a_rendered_ease_completes_at_its_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
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
  TEST_ASSERT_TRUE(observation.snapshot_read);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
    generation, observation.snapshot_generation
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, observation.snapshot_zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, query_camera(map).zoom);

  mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
  demand.flags = MLN_FRAME_DEMAND_FORCE;
  MLN_TEST_OK(mln_render_session_demand_frame(fixture.session, &demand, NULL));
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, settled_result, (void*)&fixture, mln_test_deadline_default(),
    "a fence frame"
  ));
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.calls));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void an_unobserved_animation_publishes_its_final_snapshot(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_test_map_options();
  options.fields |= MLN_MAP_OPTION_EVENT_MASK;
  options.event_mask = MLN_RUNTIME_EVENT_FLAG_NONE;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  MLN_TEST_OK(mln_test_map_set_style_json(map, mln_test_background_style_json));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));

  const mln_camera_update ease = ease_to_zoom(5.0, 20.0, NULL);
  const uint64_t started = update_camera_generation(map, &ease);
  mln_map_snapshot initial = {.size = sizeof(initial)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &initial, NULL));
  TEST_ASSERT_TRUE(initial.transition_in_progress);

  mln_render_frame_result result;
  do {
    mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
    demand.flags |= MLN_FRAME_DEMAND_WAIT_FOR_UPDATE;
    MLN_TEST_OK(
      mln_render_session_demand_frame(fixture.session, &demand, NULL)
    );
    mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture);
    result = mln_test_render_batch_result(batch, 0);
    mln_render_frame_batch_release(batch);
    TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  } while (result.needs_repaint);

  mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, snapshot.view.camera.zoom);
  TEST_ASSERT_FALSE(snapshot.transition_in_progress);
  TEST_ASSERT_GREATER_THAN_UINT64(started, snapshot.generation);
  TEST_ASSERT_GREATER_THAN_UINT64(
    initial.latest_render_update_generation,
    snapshot.latest_render_update_generation
  );
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
    result.map_update_generation, snapshot.latest_render_update_generation
  );

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
  // A valid easing and minimum zoom leave a zero-duration flight immediate.
  {"fly with a zero duration", MLN_CAMERA_UPDATE_MODE_FLY,
   MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_MIN_ZOOM |
     MLN_ANIMATION_OPTION_EASING},
};

static bool is_immediate_change(const mln_runtime_event* event, void* context) {
  return event->type == MLN_RUNTIME_EVENT_MAP_CAMERA_DID_CHANGE &&
         event->payload.camera_change.mode ==
           MLN_CAMERA_CHANGE_MODE_IMMEDIATE &&
         event->generation == *(const uint64_t*)context;
}

static void an_immediate_command_completes_before_its_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  const size_t case_count = sizeof(immediate_cases) / sizeof(*immediate_cases);
  for (size_t index = 0; index < case_count; index += 1) {
    const immediate_case* row = &immediate_cases[index];
    mln_test_transition_end probe;
    mln_test_transition_end_init(&probe);
    mln_camera_update update = {
      .size = sizeof(mln_camera_update),
      .camera = {.size = sizeof(mln_camera_options)}
    };
    update.mode = row->mode;
    update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
    update.camera.zoom = 3.0 + (double)index;
    update.animation.fields = row->animation_fields;
    update.animation.min_zoom = 1.0;
    update.animation.easing =
      (mln_unit_bezier){.x1 = 0.25, .y1 = 0.1, .x2 = 0.25, .y2 = 1.0};
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

static void the_handler_runs_whatever_the_event_mask_selects(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_test_map_options();
  options.fields |= MLN_MAP_OPTION_EVENT_MASK;
  options.event_mask = MLN_RUNTIME_EVENT_FLAG_NONE;
  mln_map map = mln_test_create_map_with_options(runtime, &options);

  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  const mln_camera_update eased = ease_to_zoom(6.0, 60000.0, &probe);
  update_camera(map, &eased);
  mln_camera_update jump = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  jump.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  jump.camera.zoom = 3.0;
  const uint64_t generation = update_camera_generation(map, &jump);
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_REPLACED, "replaced");
  TEST_ASSERT_EQUAL_UINT64(generation, atomic_load(&probe.generation));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A fly animates its center and zoom together, so a jump of its zoom replaces
// all of it. An ease animates its fields apart, so a jump of one of them
// leaves it running.
static void a_later_change_replaces_a_running_one(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera = test_camera();
  update_camera(map, &update);

  mln_test_transition_end first;
  mln_test_transition_end_init(&first);
  const mln_camera_update first_ease = ease_to_zoom(18.0, 60000.0, &first);
  update_camera(map, &first_ease);
  mln_test_transition_end second;
  mln_test_transition_end_init(&second);
  const mln_camera_update second_ease = ease_to_zoom(4.0, 60000.0, &second);
  update_camera(map, &second_ease);
  assert_ended(&first, MLN_CAMERA_TRANSITION_OUTCOME_REPLACED, "first ease");
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&second.calls));

  mln_test_transition_end flight;
  mln_test_transition_end_init(&flight);
  mln_camera_update fly = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  fly.mode = MLN_CAMERA_UPDATE_MODE_FLY;
  fly.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  fly.camera.center = (mln_lat_lng){.latitude = 40.0, .longitude = -74.0};
  fly.camera.zoom = 9.0;
  fly.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  fly.animation.duration_ms = 60000.0;
  fly.animation.end_handler = mln_test_transition_end_handler(&flight);
  update_camera(map, &fly);
  assert_ended(&second, MLN_CAMERA_TRANSITION_OUTCOME_REPLACED, "second ease");

  mln_test_transition_end jumped;
  mln_test_transition_end_init(&jumped);
  mln_camera_update jump = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  jump.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  jump.camera.zoom = 7.0;
  jump.animation.end_handler = mln_test_transition_end_handler(&jumped);
  update_camera(map, &jump);
  assert_ended(&flight, MLN_CAMERA_TRANSITION_OUTCOME_REPLACED, "fly");
  assert_ended(&jumped, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "jump");
  TEST_ASSERT_FALSE(read_settled_snapshot(runtime, map).transition_in_progress);

  mln_test_transition_end partial;
  mln_test_transition_end_init(&partial);
  mln_camera_update ease = fly;
  ease.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  ease.animation.end_handler = mln_test_transition_end_handler(&partial);
  update_camera(map, &ease);
  jump.animation.end_handler = (mln_camera_transition_handler){0};
  update_camera(map, &jump);
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&partial.calls));
  TEST_ASSERT_TRUE(read_settled_snapshot(runtime, map).transition_in_progress);
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_camera_transitions(map, &completion.descriptor, NULL)
  );
  assert_ended(&partial, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "partial");

  // A zoom limit below the camera's zoom jumps to the limit.
  mln_test_transition_end limited;
  mln_test_transition_end_init(&limited);
  const mln_camera_update zoom_in = ease_to_zoom(18.0, 60000.0, &limited);
  update_camera(map, &zoom_in);
  mln_bound_options bounds = {.size = sizeof(mln_bound_options)};
  bounds.fields = MLN_BOUND_OPTION_MAX_ZOOM;
  bounds.max_zoom = 5.0;
  MLN_TEST_AWAIT_OK(
    mln_map_set_bounds(map, &bounds, &completion.descriptor, NULL)
  );
  assert_ended(&limited, MLN_CAMERA_TRANSITION_OUTCOME_REPLACED, "bounds");
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, query_camera(map).zoom);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void free_camera_options_and_a_constraining_resize_cancel(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera = test_camera();
  update_camera(map, &update);

  mln_test_transition_end placed;
  mln_test_transition_end_init(&placed);
  const mln_camera_update eased = ease_to_zoom(18.0, 60000.0, &placed);
  update_camera(map, &eased);
  mln_free_camera_options free_camera = {
    .size = sizeof(mln_free_camera_options)
  };
  free_camera.fields = MLN_FREE_CAMERA_OPTION_POSITION;
  free_camera.position = (mln_vec3){.x = 0.25, .y = 0.25, .z = 0.5};
  MLN_TEST_AWAIT_OK(mln_map_set_free_camera_options(
    map, &free_camera, &completion.descriptor, NULL
  ));
  assert_ended(&placed, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "free camera");

  // The 64-pixel view fits inside the bounds, east of their center, and a
  // wider view crosses their east edge, so the resize moves the camera.
  mln_map_viewport_options viewport = {.size = sizeof(viewport)};
  viewport.fields = MLN_MAP_VIEWPORT_OPTION_CONSTRAIN_MODE;
  viewport.constrain_mode = MLN_CONSTRAIN_MODE_SCREEN;
  MLN_TEST_AWAIT_OK(
    mln_map_set_viewport_options(map, &viewport, &completion.descriptor, NULL)
  );
  update.camera = (mln_camera_options){.size = sizeof(mln_camera_options)};
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                         MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  update.camera.center = (mln_lat_lng){.latitude = 0.0, .longitude = 9.0};
  update.camera.zoom = 5.0;
  update_camera(map, &update);
  mln_bound_options bounds = {.size = sizeof(mln_bound_options)};
  bounds.fields = MLN_BOUND_OPTION_BOUNDS;
  bounds.bounds = (mln_lat_lng_bounds){{-10.0, -10.0}, {10.0, 10.0}};
  MLN_TEST_AWAIT_OK(
    mln_map_set_bounds(map, &bounds, &completion.descriptor, NULL)
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 9.0, query_camera(map).center.longitude);

  mln_test_transition_end resized;
  mln_test_transition_end_init(&resized);
  mln_camera_update pitch = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  pitch.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  pitch.camera.fields = MLN_CAMERA_OPTION_PITCH;
  pitch.camera.pitch = 20.0;
  pitch.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  pitch.animation.duration_ms = 60000.0;
  pitch.animation.end_handler = mln_test_transition_end_handler(&resized);
  update_camera(map, &pitch);
  MLN_TEST_AWAIT_OK(mln_map_resize(
    map, (mln_logical_extent){.width = 128, .height = 64, .scale_factor = 1.0},
    &completion.descriptor, NULL
  ));
  assert_ended(&resized, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "resize");
  TEST_ASSERT_TRUE(query_camera(map).center.longitude < 9.0);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Without a render session a transition never advances, so each outcome below
// ends a transition that is still running.
static void cancel_camera_transitions_commits_and_leaves_the_camera(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera = test_camera();
  update_camera(map, &update);

  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  const mln_camera_update eased = ease_to_zoom(18.0, 60000.0, &probe);
  update_camera(map, &eased);
  mln_test_completion cancel = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_cancel_camera_transitions(map, &cancel.descriptor, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&cancel));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.calls));
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "cancelled");
  TEST_ASSERT_EQUAL_UINT64(
    mln_test_completion_generation(&cancel), atomic_load(&probe.generation)
  );
  mln_test_completion_destroy(&cancel);
  const double settled = read_settled_snapshot(runtime, map).view.camera.zoom;
  TEST_ASSERT_TRUE(settled < 18.0);
  TEST_ASSERT_EQUAL_DOUBLE(
    settled, read_settled_snapshot(runtime, map).view.camera.zoom
  );

  MLN_TEST_AWAIT_OK(
    mln_map_cancel_camera_transitions(map, &completion.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_DOUBLE(
    settled, read_settled_snapshot(runtime, map).view.camera.zoom
  );
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.calls));

  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(
    mln_map_cancel_camera_transitions(MLN_HANDLE_NULL, &rejected, NULL)
  );
  mln_completion invalid_completion = {.size = sizeof(mln_completion)};
  MLN_TEST_INVALID(
    mln_map_cancel_camera_transitions(map, &invalid_completion, NULL)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void cancel_camera_transitions_with_id_ends_only_its_identity(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
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
  mln_camera_update pitch_ease = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
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
  mln_camera_update bearing_ease = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  bearing_ease.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  bearing_ease.camera.fields = MLN_CAMERA_OPTION_BEARING;
  bearing_ease.camera.bearing = 90.0;
  bearing_ease.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  bearing_ease.animation.duration_ms = 60000.0;
  bearing_ease.animation.end_handler =
    mln_test_transition_end_handler(&bearing);
  with_transition_id(&bearing_ease.animation, 7);
  update_camera(map, &bearing_ease);
  TEST_ASSERT_TRUE(read_settled_snapshot(runtime, map).transition_in_progress);

  MLN_TEST_AWAIT_OK(mln_map_cancel_camera_transitions_with_id(
    map, UINT64_MAX, &completion.descriptor, NULL
  ));
  assert_ended(&zoom, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "zoom");
  assert_ended(&pitch, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "pitch");
  const mln_map_snapshot cancelled = read_settled_snapshot(runtime, map);
  TEST_ASSERT_TRUE(cancelled.view.camera.zoom < 18.0);
  TEST_ASSERT_TRUE(cancelled.transition_in_progress);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&bearing.calls));

  MLN_TEST_AWAIT_OK(mln_map_cancel_camera_transitions_with_id(
    map, 8, &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_cancel_camera_transitions_with_id(
    map, UINT64_MAX, &completion.descriptor, NULL
  ));
  const mln_map_snapshot unchanged = read_settled_snapshot(runtime, map);
  TEST_ASSERT_EQUAL_DOUBLE(
    cancelled.view.camera.zoom, unchanged.view.camera.zoom
  );
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&bearing.calls));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&zoom.calls));

  mln_camera_update jump = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  jump.camera.fields = MLN_CAMERA_OPTION_BEARING;
  jump.camera.bearing = 45.0;
  update_camera(map, &jump);
  assert_ended(&bearing, MLN_CAMERA_TRANSITION_OUTCOME_REPLACED, "bearing");
  TEST_ASSERT_FALSE(read_settled_snapshot(runtime, map).transition_in_progress);

  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_map_cancel_camera_transitions_with_id(
    MLN_HANDLE_NULL, 7, &rejected, NULL
  ));
  mln_completion invalid_completion = {.size = sizeof(mln_completion)};
  MLN_TEST_INVALID(
    mln_map_cancel_camera_transitions_with_id(map, 7, &invalid_completion, NULL)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void gesture_phase_publishes_the_snapshot_flag(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_FALSE(read_settled_snapshot(runtime, map).gesture_in_progress);

  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
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
  TEST_ASSERT_EQUAL_DOUBLE(12.0, snapshot.view.camera.zoom);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A gesture that starts with UPDATE keeps a running animation of other fields,
// BEGIN cancels it before its own change, and CANCEL cancels animations that
// started during the gesture.
static void a_gesture_begin_cancels_running_transitions(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera = test_camera();
  update_camera(map, &update);

  mln_test_transition_end kept;
  mln_test_transition_end_init(&kept);
  const mln_camera_update eased = ease_to_zoom(3.0, 60000.0, &kept);
  update_camera(map, &eased);
  mln_camera_update gesture = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  gesture.camera.fields = MLN_CAMERA_OPTION_BEARING;
  gesture.camera.bearing = 20.0;
  gesture.gesture_phase = MLN_GESTURE_PHASE_UPDATE;
  update_camera(map, &gesture);
  mln_map_snapshot snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_TRUE(snapshot.gesture_in_progress);
  TEST_ASSERT_TRUE(snapshot.transition_in_progress);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 20.0, snapshot.view.camera.bearing);
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&kept.calls));

  gesture.camera.bearing = 30.0;
  gesture.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  update_camera(map, &gesture);
  assert_ended(&kept, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "begin");
  snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_TRUE(snapshot.gesture_in_progress);
  TEST_ASSERT_FALSE(snapshot.transition_in_progress);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 30.0, snapshot.view.camera.bearing);

  mln_test_transition_end started;
  mln_test_transition_end_init(&started);
  const mln_camera_update during = ease_to_zoom(3.0, 60000.0, &started);
  update_camera(map, &during);
  gesture.gesture_phase = MLN_GESTURE_PHASE_CANCEL;
  update_camera(map, &gesture);
  TEST_ASSERT_FALSE(read_settled_snapshot(runtime, map).gesture_in_progress);
  assert_ended(&started, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "cancel");

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_camera_delta animated_delta(
  uint32_t fields, double duration_ms, mln_test_transition_end* probe
) {
  mln_animation_options animation = {};
  animation.fields = MLN_ANIMATION_OPTION_DURATION;
  animation.duration_ms = duration_ms;
  animation.end_handler = mln_test_transition_end_handler(probe);
  mln_camera_delta delta = {.size = sizeof(mln_camera_delta)};
  delta.fields = fields;
  delta.animation = animation;
  return delta;
}

static void apply_delta(mln_map map, const mln_camera_delta* delta) {
  MLN_TEST_AWAIT_OK(
    mln_map_apply_camera_delta(map, delta, &completion.descriptor, NULL)
  );
}

// An animated delta that pans and zooms runs as two MapLibre transitions.
static void an_animated_delta_reports_its_end_once(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  const mln_camera start = query_camera(map);

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
  const mln_camera camera = query_camera(map);
  TEST_ASSERT_DOUBLE_WITHIN(1e-4, start.zoom + 1.0, camera.zoom);
  TEST_ASSERT_TRUE(camera.center.longitude < start.center.longitude);
  MLN_TEST_OK(mln_test_map_fence(map));
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
  mln_camera_update jump = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  jump.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  jump.camera.zoom = start.zoom;
  update_camera(map, &jump);
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&cancelled.calls));
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_camera_transitions(map, &completion.descriptor, NULL)
  );
  assert_ended(&cancelled, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "delta");

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Applying the first delta can advance it before any frame renders, so an
// ordered read supplies the second scale's starting zoom.
static void an_animated_delta_replaces_the_running_one(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  mln_test_transition_end replaced;
  mln_test_transition_end_init(&replaced);
  mln_camera_delta first =
    animated_delta(MLN_CAMERA_DELTA_SCALE, 60000.0, &replaced);
  first.scale = 4.0;
  mln_test_transition_end replacing;
  mln_test_transition_end_init(&replacing);
  mln_camera_delta second =
    animated_delta(MLN_CAMERA_DELTA_SCALE, 20.0, &replacing);
  second.scale = 4.0;
  apply_delta(map, &first);
  const double replacement_zoom = query_camera(map).zoom;
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&replaced.calls));
  apply_delta(map, &second);
  assert_ended(&replaced, MLN_CAMERA_TRANSITION_OUTCOME_REPLACED, "replaced");

  render_until_ended(&fixture, &replacing);
  assert_ended(
    &replacing, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "replacing"
  );
  // An animated scale ends within rounding of its target, which varies by
  // platform.
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-4, replacement_zoom + 2.0, query_camera(map).zoom
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A delta without components runs its handler before its completion, as a
// release without inertia needs.
static void a_delta_carries_its_gesture_phase(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera = test_camera();
  update_camera(map, &update);
  mln_test_transition_end running;
  mln_test_transition_end_init(&running);
  mln_camera_update pitch = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  pitch.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  pitch.camera.fields = MLN_CAMERA_OPTION_PITCH;
  pitch.camera.pitch = 50.0;
  pitch.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  pitch.animation.duration_ms = 60000.0;
  pitch.animation.end_handler = mln_test_transition_end_handler(&running);
  update_camera(map, &pitch);

  mln_camera_delta delta = {.size = sizeof(mln_camera_delta)};
  delta.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  apply_delta(map, &delta);
  assert_ended(&running, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "begin");
  mln_map_snapshot snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_TRUE(snapshot.gesture_in_progress);
  TEST_ASSERT_FALSE(snapshot.transition_in_progress);
  TEST_ASSERT_EQUAL_DOUBLE(11.0, snapshot.view.camera.zoom);

  delta.fields = MLN_CAMERA_DELTA_SCALE;
  delta.scale = 2.0;
  delta.gesture_phase = MLN_GESTURE_PHASE_UPDATE;
  apply_delta(map, &delta);
  snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_TRUE(snapshot.gesture_in_progress);
  TEST_ASSERT_EQUAL_DOUBLE(12.0, snapshot.view.camera.zoom);

  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  delta = (mln_camera_delta){.size = sizeof(mln_camera_delta)};
  delta.animation.end_handler = mln_test_transition_end_handler(&probe);
  delta.gesture_phase = MLN_GESTURE_PHASE_END;
  apply_delta(map, &delta);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&probe.calls));
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "release");
  snapshot = read_settled_snapshot(runtime, map);
  TEST_ASSERT_FALSE(snapshot.gesture_in_progress);
  TEST_ASSERT_EQUAL_DOUBLE(12.0, snapshot.view.camera.zoom);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct transition_close_order {
  mln_test_transition_end* transition;
  mln_completion completion;
  atomic_bool closed_before_completion;
} transition_close_order;

static void observe_transition_close(
  void* user_data, const mln_completion_result* result
) {
  transition_close_order* order = user_data;
  atomic_store(
    &order->closed_before_completion,
    atomic_load(&order->transition->calls) == 1 &&
      atomic_load(&order->transition->releases) == 1 &&
      atomic_load(&order->transition->outcome) ==
        MLN_CAMERA_TRANSITION_OUTCOME_CLOSED
  );
  order->completion.callback(order->completion.user_data, result);
}

static void release_transition_close(void* user_data) {
  transition_close_order* order = user_data;
  order->completion.release_user_data(order->completion.user_data);
}

static void a_map_release_or_dispose_ends_running_transitions(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = MLN_HANDLE_NULL;
  const mln_map_options map_options = mln_test_map_options();
  MLN_TEST_OK(mln_test_map_create_status(runtime, &map_options, &map));
  mln_test_transition_end released;
  mln_test_transition_end_init(&released);
  const mln_camera_update eased = ease_to_zoom(6.0, 60000.0, &released);
  update_camera(map, &eased);
  mln_test_completion release = mln_test_completion_default(0);
  transition_close_order order = {
    .transition = &released,
    .completion = release.descriptor,
  };
  atomic_init(&order.closed_before_completion, false);
  const mln_completion observed = {
    .size = sizeof(observed),
    .callback = observe_transition_close,
    .user_data = &order,
    .release_user_data = release_transition_close,
  };
  MLN_TEST_OK(mln_map_release(map, &observed, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&release));
  mln_test_completion_destroy(&release);
  TEST_ASSERT_TRUE(atomic_load(&order.closed_before_completion));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&released.calls));
  assert_ended(&released, MLN_CAMERA_TRANSITION_OUTCOME_CLOSED, "release");
  TEST_ASSERT_EQUAL_UINT64(0, atomic_load(&released.generation));

  mln_map disposed = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_map_create_status(runtime, &map_options, &disposed));
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

static void submit_from_end(
  mln_test_transition_end* probe, const mln_camera_transition_end* end
) {
  (void)end;
  submitting_end* state = probe->context;
  mln_camera_update jump = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  jump.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  jump.camera.zoom = 9.0;
  const mln_completion discard = mln_test_discard_completion();
  atomic_store(
    &state->status,
    (int)mln_map_update_camera(state->map, &jump, &discard, NULL)
  );
}

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
    9.0, read_settled_snapshot(runtime, map).view.camera.zoom
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_rejected_command_runs_no_handler(void) {
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
  MLN_TEST_OK(mln_test_map_fence(map));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.calls));
  TEST_ASSERT_EQUAL_INT(0, atomic_load(&probe.releases));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static const mln_lat_lng_bounds fit_region = {
  .southwest = {.latitude = 35.0, .longitude = -125.0},
  .northeast = {.latitude = 39.0, .longitude = -115.0},
};

static void a_rendered_fit_ease_ends_where_the_query_fits(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  mln_test_drain_all(runtime);

  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_options));
  MLN_TEST_OK(mln_map_camera_for_lat_lng_bounds(
    map, fit_region, NULL, &query.descriptor, NULL
  ));
  mln_camera_options fit = {.size = sizeof(mln_camera_options)};
  MLN_TEST_OK(mln_test_completion_finish_value(&query, &fit, sizeof(fit)));

  mln_test_transition_end probe;
  mln_test_transition_end_init(&probe);
  mln_camera_fit_update update = {.size = sizeof(mln_camera_fit_update)};
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  update.animation.duration_ms = 20.0;
  update.animation.end_handler = mln_test_transition_end_handler(&probe);
  MLN_TEST_AWAIT_OK(mln_map_fit_camera_to_lat_lng_bounds(
    map, fit_region, &update, &completion.descriptor, NULL
  ));

  render_until_ended(&fixture, &probe);
  assert_ended(&probe, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "fit ease");
  const mln_camera camera = query_camera(map);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, fit.center.latitude, camera.center.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, fit.center.longitude, camera.center.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, fit.zoom, camera.zoom);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A fit jump ends as soon as the camera moves, and a fit that fails ends as
// cancelled without moving the camera.
static void a_fit_command_ends_its_handler_when_it_runs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_test_transition_end jumped;
  mln_test_transition_end_init(&jumped);
  mln_camera_fit_update update = {.size = sizeof(mln_camera_fit_update)};
  update.animation.end_handler = mln_test_transition_end_handler(&jumped);
  MLN_TEST_AWAIT_OK(mln_map_fit_camera_to_lat_lng_bounds(
    map, fit_region, &update, &completion.descriptor, NULL
  ));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&jumped.calls));
  assert_ended(&jumped, MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED, "fit jump");
  const mln_camera fitted = query_camera(map);

  mln_test_transition_end failed;
  mln_test_transition_end_init(&failed);
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.fit.fields = MLN_CAMERA_FIT_OPTION_PADDING;
  update.fit.padding = (mln_edge_insets){.top = 64.0};
  update.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  update.animation.duration_ms = 60000.0;
  update.animation.end_handler = mln_test_transition_end_handler(&failed);
  mln_test_completion command = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_fit_camera_to_lat_lng_bounds(
    map, fit_region, &update, &command.descriptor, NULL
  ));
  MLN_TEST_INVALID(mln_test_completion_finish(&command));
  mln_test_completion_destroy(&command);
  assert_ended(&failed, MLN_CAMERA_TRANSITION_OUTCOME_CANCELLED, "fit failure");
  const mln_camera after = query_camera(map);
  TEST_ASSERT_EQUAL_DOUBLE(fitted.zoom, after.zoom);
  TEST_ASSERT_EQUAL_DOUBLE(fitted.center.latitude, after.center.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(fitted.center.longitude, after.center.longitude);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_rendered_ease_completes_at_its_target);
  RUN_TEST(an_unobserved_animation_publishes_its_final_snapshot);
  RUN_TEST(an_immediate_command_completes_before_its_completion);
  RUN_TEST(the_handler_runs_whatever_the_event_mask_selects);
  RUN_TEST(a_later_change_replaces_a_running_one);
  RUN_TEST(free_camera_options_and_a_constraining_resize_cancel);
  RUN_TEST(cancel_camera_transitions_commits_and_leaves_the_camera);
  RUN_TEST(cancel_camera_transitions_with_id_ends_only_its_identity);
  RUN_TEST(gesture_phase_publishes_the_snapshot_flag);
  RUN_TEST(a_gesture_begin_cancels_running_transitions);
  RUN_TEST(an_animated_delta_reports_its_end_once);
  RUN_TEST(an_animated_delta_replaces_the_running_one);
  RUN_TEST(a_delta_carries_its_gesture_phase);
  RUN_TEST(a_map_release_or_dispose_ends_running_transitions);
  RUN_TEST(a_handler_may_submit_a_command);
  RUN_TEST(a_rejected_command_runs_no_handler);
  RUN_TEST(a_rendered_fit_ease_ends_where_the_query_fits);
  RUN_TEST(a_fit_command_ends_its_handler_when_it_runs);
}
