// Snippets for the docs site's Camera page. The native build compiles
// them and never runs them.

#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static atomic_bool finished;

static void on_done(
  void* user_data, const mln_completion_result* result
) {
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
static void on_camera(
  void* user_data, const mln_completion_result* result
) {
  if (result->status == MLN_STATUS_OK) {
    const mln_camera_query_result* query = result->value;
    use_camera(&query->camera);
  }
  on_done(user_data, result);
}
// #endregion read

// #region animate
static void on_fly_end(
  void* user_data, const mln_camera_transition_end* end
) {
  (void)user_data;
  // Runs on a library thread.
  fly_ended(end->outcome == MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED);
}
// #endregion animate

// #region gesture
static mln_gesture_recognizer create_recognizer(mln_map map) {
  mln_gesture_recognizer recognizer = MLN_HANDLE_NULL;
  mln_gesture_recognizer_create(map, NULL, &recognizer, NULL);
  return recognizer;
}

static void schedule_tick(const mln_gesture_result* result) {
  (void)result;  // On the next frame, or at result->next_tick_ms.
}

static void pointer_down(
  mln_gesture_recognizer recognizer, uint32_t id,
  mln_screen_point position, double time_ms
) {
  const mln_gesture_input input = {
    .size = sizeof(mln_gesture_input),
    .type = MLN_GESTURE_INPUT_POINTER_DOWN,
    .data.pointer_down = {
      .id = id, .kind = MLN_POINTER_KIND_TOUCH, .position = position
    },
  };
  mln_gesture_result result = {.size = sizeof(mln_gesture_result)};
  mln_gesture_recognizer_handle(
    recognizer, &input, time_ms, &result, NULL
  );
  schedule_tick(&result);
}

static void pointer_move(
  mln_gesture_recognizer recognizer, uint32_t id,
  mln_screen_point position, double time_ms
) {
  const mln_gesture_input input = {
    .size = sizeof(mln_gesture_input),
    .type = MLN_GESTURE_INPUT_POINTER_MOVE,
    .data.pointer_move = {.id = id, .position = position},
  };
  mln_gesture_result result = {.size = sizeof(mln_gesture_result)};
  mln_gesture_recognizer_handle(
    recognizer, &input, time_ms, &result, NULL
  );
  schedule_tick(&result);
}

static void tick(mln_gesture_recognizer recognizer, double time_ms) {
  mln_gesture_result result = {.size = sizeof(mln_gesture_result)};
  mln_gesture_recognizer_tick(recognizer, time_ms, &result, NULL);
  schedule_tick(&result);
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
  const mln_camera_fit_update fit = {
    .size = sizeof(mln_camera_fit_update),
    .mode = MLN_CAMERA_UPDATE_MODE_EASE,
    .animation = {
      .fields = MLN_ANIMATION_OPTION_DURATION, .duration_ms = 500
    },
  };
  mln_map_fit_camera_to_lat_lng_bounds(map, bounds, &fit, NULL, NULL);
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

  mln_gesture_recognizer recognizer = create_recognizer(map);
  pointer_down(
    recognizer, 1, (mln_screen_point){.x = 100, .y = 100}, 0
  );
  pointer_move(
    recognizer, 1, (mln_screen_point){.x = 112, .y = 96}, 16
  );
  tick(recognizer, 32);
  mln_gesture_recognizer_release(recognizer);

  // #region convert
  mln_map_snapshot current = {.size = sizeof(mln_map_snapshot)};
  mln_map_get_snapshot(map, &current, NULL);

  mln_map_projection projection = MLN_HANDLE_NULL;
  mln_map_projection_create(&current.view, &projection, NULL);

  mln_lat_lng coordinate;
  mln_map_projection_lat_lng_for_pixel(
    projection, (mln_screen_point){.x = 100, .y = 200}, &coordinate,
    NULL
  );

  mln_screen_point point;
  mln_map_projection_pixel_for_lat_lng(
    projection, coordinate, &point, NULL
  );

  mln_map_projection_release(projection);
  // #endregion convert
}
