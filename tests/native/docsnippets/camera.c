// Snippets for the docs site's Camera page. They compile with the native
// tests, which never run them.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static atomic_bool finished;
static mln_camera_options fitted;

static void on_done(void* user_data, const mln_completion_result* result) {
  (void)user_data;
  (void)result;
  atomic_store(&finished, true);
}

static void wait_for_done(void) {
  while (!atomic_exchange(&finished, false)) {
  }
}

static void use_camera(const mln_camera* camera) { (void)camera; }

static void fly_ended(bool completed) { (void)completed; }

// #region read
static void on_camera(void* user_data, const mln_completion_result* result) {
  if (result->status == MLN_STATUS_OK) {
    const mln_camera_query_result* query = result->value;
    use_camera(&query->camera);
  }
  on_done(user_data, result);
}
// #endregion read

// #region animate
static void on_fly_end(void* user_data, const mln_camera_transition_end* end) {
  (void)user_data;
  // Runs on a library thread.
  fly_ended(end->outcome == MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED);
}
// #endregion animate

// #region fit
static void on_fit(void* user_data, const mln_completion_result* result) {
  if (result->status == MLN_STATUS_OK) {
    // The result is valid only during this callback.
    const mln_camera_options* camera = result->value;
    fitted = *camera;
  }
  on_done(user_data, result);
}
// #endregion fit

// #region gesture
static void begin_gesture(mln_map map) {
  mln_map_cancel_transitions(map, NULL, NULL);

  const mln_camera_delta begin = {
    .size = sizeof(mln_camera_delta),
    .gesture_phase = MLN_GESTURE_PHASE_BEGIN,
  };
  mln_map_apply_camera_delta(map, &begin, NULL, NULL);
}

static void apply_drag(mln_map map, mln_screen_point movement) {
  const mln_camera_delta drag = {
    .size = sizeof(mln_camera_delta),
    .fields = MLN_CAMERA_DELTA_OFFSET,
    .offset = movement,
    .gesture_phase = MLN_GESTURE_PHASE_UPDATE,
  };
  mln_map_apply_camera_delta(map, &drag, NULL, NULL);
}

static void apply_pinch(mln_map map, double scale, mln_screen_point center) {
  const mln_camera_delta pinch = {
    .size = sizeof(mln_camera_delta),
    .fields = MLN_CAMERA_DELTA_SCALE | MLN_CAMERA_DELTA_ANCHOR,
    .scale = scale,
    .anchor = center,
    .gesture_phase = MLN_GESTURE_PHASE_UPDATE,
  };
  mln_map_apply_camera_delta(map, &pinch, NULL, NULL);
}

static void end_gesture(mln_map map) {
  const mln_camera_delta end = {
    .size = sizeof(mln_camera_delta),
    .gesture_phase = MLN_GESTURE_PHASE_END,
  };
  mln_map_apply_camera_delta(map, &end, NULL, NULL);
}
// #endregion gesture

void mln_doc_camera(mln_map map);

void mln_doc_camera(mln_map map) {
  // #region read
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  mln_map_get_snapshot(map, &snapshot, NULL);
  use_camera(&snapshot.view.camera);

  const mln_completion read = {
    .size = sizeof(mln_completion), .callback = on_camera
  };
  mln_map_get_camera(map, &read, NULL);
  // #endregion read
  wait_for_done();

  // #region jump
  const mln_camera_update jump = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_JUMP,
    .camera = {
      .size = sizeof(mln_camera_options),
      .fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM,
      .center = {.latitude = 48.8566, .longitude = 2.3522},
      .zoom = 12,
    },
  };
  mln_map_update_camera(map, &jump, NULL, NULL);
  // #endregion jump

  // #region animate
  const mln_camera_update fly = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_FLY,
    .camera =
      {
        .size = sizeof(mln_camera_options),
        .fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM,
        .center = {.latitude = 40.7128, .longitude = -74.0060},
        .zoom = 10,
      },
    .animation = {
      .fields = MLN_ANIMATION_OPTION_DURATION,
      .duration_ms = 3000,
      .end_handler = {.callback = on_fly_end},
    },
  };
  mln_map_update_camera(map, &fly, NULL, NULL);
  // #endregion animate

  // #region fit
  const mln_lat_lng_bounds bounds = {
    .southwest = {.latitude = 45.8, .longitude = 5.9},
    .northeast = {.latitude = 47.8, .longitude = 10.5},
  };
  const mln_completion fit = {
    .size = sizeof(mln_completion), .callback = on_fit
  };
  // NULL fit options use the defaults.
  mln_map_camera_for_lat_lng_bounds(map, bounds, NULL, &fit, NULL);
  // #endregion fit
  wait_for_done();

  // #region fit
  const mln_camera_update ease = {
    .size = sizeof(mln_camera_update),
    .mode = MLN_CAMERA_UPDATE_MODE_EASE,
    .camera = fitted,
    .animation = {.fields = MLN_ANIMATION_OPTION_DURATION, .duration_ms = 500},
  };
  mln_map_update_camera(map, &ease, NULL, NULL);
  // #endregion fit

  // #region limit
  const mln_bound_options limits = {
    .size = sizeof(mln_bound_options),
    .fields = MLN_BOUND_OPTION_BOUNDS | MLN_BOUND_OPTION_MIN_ZOOM |
              MLN_BOUND_OPTION_MAX_ZOOM,
    .bounds = bounds,
    .min_zoom = 5,
    .max_zoom = 16,
  };
  mln_map_set_bounds(map, &limits, NULL, NULL);
  // #endregion limit

  begin_gesture(map);
  apply_drag(map, (mln_screen_point){.x = 12, .y = -4});
  apply_pinch(map, 1.1, (mln_screen_point){.x = 200, .y = 150});
  end_gesture(map);

  // #region convert
  mln_map_snapshot current = {.size = sizeof(mln_map_snapshot)};
  mln_map_get_snapshot(map, &current, NULL);

  mln_map_projection projection = MLN_HANDLE_NULL;
  mln_map_projection_create(&current.view, &projection, NULL);

  mln_lat_lng coordinate;
  mln_map_projection_lat_lng_for_pixel(
    projection, (mln_screen_point){.x = 100, .y = 200}, &coordinate, NULL
  );

  mln_screen_point point;
  mln_map_projection_pixel_for_lat_lng(projection, coordinate, &point, NULL);

  mln_map_projection_release(projection);
  // #endregion convert
}
