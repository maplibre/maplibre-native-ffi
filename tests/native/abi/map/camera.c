// The map camera and its constraints.

#include <math.h>

#include "support/test_support.h"

static mln_map create_square_map(mln_runtime runtime, uint32_t side) {
  mln_map_options options = mln_test_map_options();
  options.initial_extent =
    (mln_logical_extent){.width = side, .height = side, .scale_factor = 1.0};
  return mln_test_create_map_with_options(runtime, &options);
}

static mln_camera_options test_camera(void) {
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
                  MLN_CAMERA_OPTION_PADDING | MLN_CAMERA_OPTION_ANCHOR;
  camera.center.latitude = 37.7749;
  camera.center.longitude = -122.4194;
  camera.zoom = 11.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  camera.padding = (mln_edge_insets){1.0, 2.0, 3.0, 4.0};
  camera.anchor = (mln_screen_point){25.0, 30.0};
  return camera;
}

static void jump(mln_map map, mln_camera_options camera) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera = camera;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

static mln_camera_query_result query_camera(mln_map map) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_query_result));
  MLN_TEST_OK(mln_map_get_camera(map, &query.descriptor, NULL));
  mln_camera_query_result result = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &result, sizeof(result))
  );
  return result;
}

static mln_lat_lng coordinate_at(mln_map map, mln_screen_point point) {
  mln_map_projection projection = mln_test_map_create_projection(map);
  mln_lat_lng coordinate = {0};
  MLN_TEST_OK(
    mln_map_projection_lat_lng_for_pixel(projection, point, &coordinate, NULL)
  );
  mln_map_projection_release(projection);
  return coordinate;
}

static mln_status submit_update_row(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_completion discard = mln_test_discard_completion();
  return mln_map_update_camera(
    *(const mln_map*)context, descriptor, &discard, diagnostic
  );
}

static void update_undersized(void* descriptor) {
  ((mln_camera_update*)descriptor)->size -= 1;
}
static void update_bad_mode(void* descriptor) {
  ((mln_camera_update*)descriptor)->mode = MLN_CAMERA_UPDATE_MODE_FLY + 1;
}
// The update versions its embedded camera, so native ignores the camera's own
// size.
static void camera_without_size(void* descriptor) {
  ((mln_camera_update*)descriptor)->camera.size = 0;
}
static void camera_unknown_field(void* descriptor) {
  ((mln_camera_update*)descriptor)->camera.fields = UINT32_C(1) << 31;
}
static void camera_latitude_past_the_pole(void* descriptor) {
  mln_camera_options* camera = &((mln_camera_update*)descriptor)->camera;
  camera->fields = MLN_CAMERA_OPTION_CENTER;
  camera->center.latitude = 90.5;
}
static void camera_nan_zoom(void* descriptor) {
  mln_camera_options* camera = &((mln_camera_update*)descriptor)->camera;
  camera->fields = MLN_CAMERA_OPTION_ZOOM;
  camera->zoom = NAN;
}
static void camera_negative_padding(void* descriptor) {
  mln_camera_options* camera = &((mln_camera_update*)descriptor)->camera;
  camera->fields = MLN_CAMERA_OPTION_PADDING;
  camera->padding.right = -1.0;
}
static void camera_nan_anchor(void* descriptor) {
  mln_camera_options* camera = &((mln_camera_update*)descriptor)->camera;
  camera->fields = MLN_CAMERA_OPTION_ANCHOR;
  camera->anchor.y = NAN;
}
static void animation_unknown_field(void* descriptor) {
  ((mln_camera_update*)descriptor)->animation.fields = UINT32_C(1) << 31;
}
static void animation_duration_past_the_native_range(void* descriptor) {
  mln_animation_options* animation =
    &((mln_camera_update*)descriptor)->animation;
  animation->fields = MLN_ANIMATION_OPTION_DURATION;
  animation->duration_ms = 1e300;
}
static void animation_negative_max_duration(void* descriptor) {
  mln_animation_options* animation =
    &((mln_camera_update*)descriptor)->animation;
  animation->fields = MLN_ANIMATION_OPTION_MAX_DURATION;
  animation->max_duration_ms = -1.0;
}
static void animation_zero_velocity(void* descriptor) {
  mln_animation_options* animation =
    &((mln_camera_update*)descriptor)->animation;
  animation->fields = MLN_ANIMATION_OPTION_VELOCITY;
  animation->velocity = 0.0;
}
static void animation_easing_outside_the_unit_square(void* descriptor) {
  mln_animation_options* animation =
    &((mln_camera_update*)descriptor)->animation;
  animation->fields = MLN_ANIMATION_OPTION_EASING;
  animation->easing = (mln_unit_bezier){.x1 = 1.5, .x2 = 0.5};
}

static const mln_test_validation_case update_cases[] = {
  {"undersized update", update_undersized, MLN_STATUS_INVALID_ARGUMENT,
   "valid size"},
  {"mode out of range", update_bad_mode, MLN_STATUS_INVALID_ARGUMENT,
   "mode or gesture phase"},
  {"camera without size", camera_without_size, MLN_STATUS_OK, NULL},
  {"unknown camera field", camera_unknown_field, MLN_STATUS_INVALID_ARGUMENT,
   "unknown bits"},
  {"latitude past the pole", camera_latitude_past_the_pole,
   MLN_STATUS_INVALID_ARGUMENT, "within [-90, 90]"},
  {"NaN zoom", camera_nan_zoom, MLN_STATUS_INVALID_ARGUMENT, "must be finite"},
  {"negative padding", camera_negative_padding, MLN_STATUS_INVALID_ARGUMENT,
   "greater than or equal to 0"},
  {"NaN anchor", camera_nan_anchor, MLN_STATUS_INVALID_ARGUMENT,
   "screen point values must be finite"},
  {"unknown animation field", animation_unknown_field,
   MLN_STATUS_INVALID_ARGUMENT, "unknown bits"},
  {"duration past the native range", animation_duration_past_the_native_range,
   MLN_STATUS_INVALID_ARGUMENT, "native duration range"},
  {"negative max duration", animation_negative_max_duration,
   MLN_STATUS_INVALID_ARGUMENT, "native duration range"},
  {"zero velocity", animation_zero_velocity, MLN_STATUS_INVALID_ARGUMENT,
   "positive and finite"},
  {"easing outside the unit square", animation_easing_outside_the_unit_square,
   MLN_STATUS_INVALID_ARGUMENT, "within [0, 1]"},
};

static mln_status submit_fit_update_row(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_completion discard = mln_test_discard_completion();
  const mln_lat_lng_bounds bounds = {
    .southwest = {.latitude = -10.0, .longitude = -10.0},
    .northeast = {.latitude = 10.0, .longitude = 10.0},
  };
  return mln_map_fit_camera_to_lat_lng_bounds(
    *(const mln_map*)context, bounds, descriptor, &discard, diagnostic
  );
}

static void fit_update_undersized(void* descriptor) {
  ((mln_camera_fit_update*)descriptor)->size -= 1;
}
static void fit_update_bad_mode(void* descriptor) {
  ((mln_camera_fit_update*)descriptor)->mode = MLN_CAMERA_UPDATE_MODE_FLY + 1;
}
// The update versions its embedded fit options, so native ignores their own
// size.
static void fit_without_size(void* descriptor) {
  ((mln_camera_fit_update*)descriptor)->fit.size = 0;
}
static void fit_unknown_field(void* descriptor) {
  ((mln_camera_fit_update*)descriptor)->fit.fields = UINT32_C(1) << 31;
}
static void fit_negative_padding(void* descriptor) {
  mln_camera_fit_options* fit = &((mln_camera_fit_update*)descriptor)->fit;
  fit->fields = MLN_CAMERA_FIT_OPTION_PADDING;
  fit->padding.left = -1.0;
}
static void fit_nan_pitch(void* descriptor) {
  mln_camera_fit_options* fit = &((mln_camera_fit_update*)descriptor)->fit;
  fit->fields = MLN_CAMERA_FIT_OPTION_PITCH;
  fit->pitch = NAN;
}
static void fit_animation_zero_velocity(void* descriptor) {
  mln_animation_options* animation =
    &((mln_camera_fit_update*)descriptor)->animation;
  animation->fields = MLN_ANIMATION_OPTION_VELOCITY;
  animation->velocity = 0.0;
}

static const mln_test_validation_case fit_update_cases[] = {
  {"undersized update", fit_update_undersized, MLN_STATUS_INVALID_ARGUMENT,
   "valid size"},
  {"mode out of range", fit_update_bad_mode, MLN_STATUS_INVALID_ARGUMENT,
   "mode is invalid"},
  {"fit without size", fit_without_size, MLN_STATUS_OK, NULL},
  {"unknown fit field", fit_unknown_field, MLN_STATUS_INVALID_ARGUMENT,
   "unknown bits"},
  {"negative padding", fit_negative_padding, MLN_STATUS_INVALID_ARGUMENT,
   "greater than or equal to 0"},
  {"NaN pitch", fit_nan_pitch, MLN_STATUS_INVALID_ARGUMENT, "must be finite"},
  {"zero velocity", fit_animation_zero_velocity, MLN_STATUS_INVALID_ARGUMENT,
   "positive and finite"},
};

// One absolute camera field and the value MapLibre must hand back for it.
typedef struct camera_row {
  const char* label;
  mln_camera_options camera;
} camera_row;

static const camera_row camera_rows[] = {
  {"center",
   {.size = sizeof(mln_camera_options),
    .fields = MLN_CAMERA_OPTION_CENTER,
    .center = {.latitude = -33.8688, .longitude = 151.2093}}},
  {"zoom",
   {.size = sizeof(mln_camera_options),
    .fields = MLN_CAMERA_OPTION_ZOOM,
    .zoom = 7.25}},
  {"bearing",
   {.size = sizeof(mln_camera_options),
    .fields = MLN_CAMERA_OPTION_BEARING,
    .bearing = -45.0}},
  {"pitch",
   {.size = sizeof(mln_camera_options),
    .fields = MLN_CAMERA_OPTION_PITCH,
    .pitch = 40.0}},
  {"padding",
   {.size = sizeof(mln_camera_options),
    .fields = MLN_CAMERA_OPTION_PADDING,
    .padding = {.top = 4.0, .left = 3.0, .bottom = 2.0, .right = 1.0}}},
  {"roll",
   {.size = sizeof(mln_camera_options),
    .fields = MLN_CAMERA_OPTION_ROLL,
    .roll = 15.0}},
  {"field of view",
   {.size = sizeof(mln_camera_options),
    .fields = MLN_CAMERA_OPTION_FOV,
    .field_of_view = 45.0}},
};

static void assert_camera_field(
  const char* label, const mln_camera_options* sent, const mln_camera* got
) {
  if ((sent->fields & MLN_CAMERA_OPTION_CENTER) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->center.latitude, got->center.latitude, label
    );
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->center.longitude, got->center.longitude, label
    );
  }
  if ((sent->fields & MLN_CAMERA_OPTION_ZOOM) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, sent->zoom, got->zoom, label);
  }
  if ((sent->fields & MLN_CAMERA_OPTION_BEARING) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, sent->bearing, got->bearing, label);
  }
  if ((sent->fields & MLN_CAMERA_OPTION_PITCH) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, sent->pitch, got->pitch, label);
  }
  if ((sent->fields & MLN_CAMERA_OPTION_PADDING) != 0U) {
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->padding.top, got->padding.top, label
    );
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->padding.left, got->padding.left, label
    );
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->padding.bottom, got->padding.bottom, label
    );
    TEST_ASSERT_EQUAL_DOUBLE_MESSAGE(
      sent->padding.right, got->padding.right, label
    );
  }
  if ((sent->fields & MLN_CAMERA_OPTION_ROLL) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, sent->roll, got->roll, label);
  }
  if ((sent->fields & MLN_CAMERA_OPTION_FOV) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->field_of_view, got->field_of_view, label
    );
  }
}

static void every_camera_field_round_trips_through_the_snapshot(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  for (size_t index = 0; index < sizeof(camera_rows) / sizeof(*camera_rows);
       index += 1) {
    const camera_row* row = &camera_rows[index];
    jump(map, row->camera);
    const mln_camera_query_result queried = query_camera(map);
    assert_camera_field(row->label, &row->camera, &queried.camera);

    mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
    MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
      queried.generation, snapshot.generation
    );
    assert_camera_field(row->label, &row->camera, &snapshot.view.camera);

    for (size_t earlier = 0; earlier < index; earlier += 1) {
      assert_camera_field(
        camera_rows[earlier].label, &camera_rows[earlier].camera,
        &queried.camera
      );
    }
  }

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void queued_immediate_deltas_compose_exactly(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  jump(map, test_camera());

  const mln_completion discard = mln_test_discard_completion();
  mln_camera_delta delta = {.size = sizeof(mln_camera_delta)};
  delta.fields = MLN_CAMERA_DELTA_OFFSET;
  delta.offset = (mln_screen_point){.x = 5.0, .y = -3.0};
  MLN_TEST_OK(mln_map_apply_camera_delta(map, &delta, &discard, NULL));
  delta.fields = MLN_CAMERA_DELTA_SCALE | MLN_CAMERA_DELTA_ANCHOR;
  delta.scale = 2.0;
  delta.anchor = (mln_screen_point){.x = 25.0, .y = 30.0};
  MLN_TEST_OK(mln_map_apply_camera_delta(map, &delta, &discard, NULL));
  delta.fields = MLN_CAMERA_DELTA_PITCH;
  delta.pitch = 5.0;
  MLN_TEST_OK(mln_map_apply_camera_delta(map, &delta, &discard, NULL));
  delta.fields = MLN_CAMERA_DELTA_BEARING;
  delta.bearing = 10.0;
  for (int index = 0; index < 4; index += 1) {
    MLN_TEST_OK(mln_map_apply_camera_delta(map, &delta, &discard, NULL));
  }
  MLN_TEST_AWAIT_OK(
    mln_map_apply_camera_delta(map, &delta, &completion.descriptor, NULL)
  );

  const mln_camera_query_result result = query_camera(map);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 12.0, result.camera.zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 62.0, result.camera.bearing);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 35.0, result.camera.pitch);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Each row's check follows mln_camera_delta's documented convention for the
// fields it selects.
typedef struct delta_row {
  const char* label;
  uint32_t fields;
  mln_screen_point offset;
  double scale;
  double bearing;
  double pitch;
  mln_screen_point anchor;
} delta_row;

static const delta_row delta_rows[] = {
  {"pan up and left", MLN_CAMERA_DELTA_OFFSET, {.x = -32.0, .y = -16.0}},
  {"scale out about an edge",
   MLN_CAMERA_DELTA_SCALE | MLN_CAMERA_DELTA_ANCHOR,
   {0},
   0.5,
   .anchor = {.x = 256.0, .y = 128.0}},
  {"rotate clockwise", MLN_CAMERA_DELTA_BEARING, .bearing = 30.0},
  {"tilt away from straight down", MLN_CAMERA_DELTA_PITCH, .pitch = 20.0},
  {"tilt about a point", MLN_CAMERA_DELTA_PITCH | MLN_CAMERA_DELTA_ANCHOR,
   .pitch = 10.0, .anchor = {.x = 96.0, .y = 200.0}},
  {"scale, rotate, and tilt about a point",
   MLN_CAMERA_DELTA_SCALE | MLN_CAMERA_DELTA_BEARING | MLN_CAMERA_DELTA_PITCH |
     MLN_CAMERA_DELTA_ANCHOR,
   {0},
   1.5,
   -15.0,
   -10.0,
   {.x = 180.0, .y = 160.0}},
  {"pan, then scale and rotate about the center",
   MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE | MLN_CAMERA_DELTA_BEARING,
   {.x = 20.0, .y = -12.0},
   0.5,
   40.0},
};

static void assert_same_coordinate(
  const char* label, mln_lat_lng expected, mln_lat_lng actual
) {
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    1e-6, expected.latitude, actual.latitude, label
  );
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    1e-6, expected.longitude, actual.longitude, label
  );
}

static void every_camera_delta_field_follows_its_convention(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 256);
  mln_camera_options start = {.size = sizeof(mln_camera_options)};
  start.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                 MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  start.center.latitude = 10.0;
  start.center.longitude = 20.0;
  start.zoom = 4.0;
  jump(map, start);
  const mln_screen_point center = {.x = 128.0, .y = 128.0};

  for (size_t index = 0; index < sizeof(delta_rows) / sizeof(*delta_rows);
       index += 1) {
    const delta_row* row = &delta_rows[index];
    const bool pans = (row->fields & MLN_CAMERA_DELTA_OFFSET) != 0U;
    const bool anchored = (row->fields & MLN_CAMERA_DELTA_ANCHOR) != 0U;
    const mln_camera before = query_camera(map).camera;
    const mln_lat_lng before_center = before.center;
    const mln_lat_lng moved_to_center =
      pans
        ? coordinate_at(
            map, (
                   mln_screen_point
                 ){.x = center.x - row->offset.x, .y = center.y - row->offset.y}
          )
        : before_center;
    const mln_lat_lng under_anchor =
      anchored ? coordinate_at(map, row->anchor) : before_center;

    mln_camera_delta delta = {.size = sizeof(mln_camera_delta)};
    delta.fields = row->fields;
    delta.offset = row->offset;
    delta.scale = row->scale;
    delta.bearing = row->bearing;
    delta.pitch = row->pitch;
    delta.anchor = row->anchor;
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_OK_MESSAGE(
      mln_map_apply_camera_delta(map, &delta, &completion.descriptor, NULL),
      row->label
    );
    MLN_TEST_OK_MESSAGE(mln_test_completion_finish(&completion), row->label);
    mln_test_completion_destroy(&completion);

    const mln_camera after = query_camera(map).camera;
    const mln_lat_lng after_center = after.center;
    const double expected_zoom =
      before.zoom +
      ((row->fields & MLN_CAMERA_DELTA_SCALE) != 0U ? log2(row->scale) : 0.0);
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, expected_zoom, after.zoom, row->label
    );
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, before.bearing + row->bearing, after.bearing, row->label
    );
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, before.pitch + row->pitch, after.pitch, row->label
    );
    if (anchored) {
      assert_same_coordinate(
        row->label, under_anchor, coordinate_at(map, row->anchor)
      );
      TEST_ASSERT_FALSE_MESSAGE(
        fabs(after_center.longitude - before_center.longitude) < 1e-6 &&
          fabs(after_center.latitude - before_center.latitude) < 1e-6,
        row->label
      );
    } else {
      assert_same_coordinate(row->label, moved_to_center, after_center);
    }
  }

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_screen_point projected_pixel(
  mln_map_projection projection, mln_lat_lng coordinate
) {
  mln_screen_point point = {0};
  MLN_TEST_OK(
    mln_map_projection_pixel_for_lat_lng(projection, coordinate, &point, NULL)
  );
  return point;
}

// Forwards a command's completion after copying the map snapshot. The
// completion runs before anything else can publish, so the copy is the
// snapshot that the command published.
typedef struct snapshot_probe {
  mln_completion inner;
  mln_map map;
  mln_status read_status;
  mln_map_snapshot snapshot;
} snapshot_probe;

static void read_snapshot_then_complete(
  void* user_data, const mln_completion_result* result
) {
  snapshot_probe* probe = user_data;
  probe->snapshot = (mln_map_snapshot){.size = sizeof(mln_map_snapshot)};
  probe->read_status = mln_map_get_snapshot(probe->map, &probe->snapshot, NULL);
  probe->inner.callback(probe->inner.user_data, result);
}

static void release_snapshot_probe(void* user_data) {
  const snapshot_probe* probe = user_data;
  if (probe->inner.release_user_data != NULL) {
    probe->inner.release_user_data(probe->inner.user_data);
  }
}

// A two-finger gesture passes its centroid movement as offset and its new
// centroid as anchor.
static void an_immediate_anchored_pan_keeps_the_dragged_coordinate(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 256);
  mln_camera_options start = {.size = sizeof(mln_camera_options)};
  start.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  start.center.latitude = 10.0;
  start.center.longitude = 20.0;
  start.zoom = 4.0;
  jump(map, start);
  const mln_screen_point offset = {.x = 30.0, .y = -20.0};
  const mln_screen_point anchor = {.x = 64.0, .y = 192.0};
  const mln_lat_lng dragged = coordinate_at(
    map, (mln_screen_point){.x = anchor.x - offset.x, .y = anchor.y - offset.y}
  );
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_test_drain_all(runtime);
  mln_map_snapshot before = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &before, NULL));

  mln_test_completion completion = mln_test_completion_default(0);
  snapshot_probe probe = {.inner = completion.descriptor, .map = map};
  const mln_completion probed = {
    .size = sizeof(mln_completion),
    .callback = read_snapshot_then_complete,
    .user_data = &probe,
    .release_user_data = release_snapshot_probe,
  };
  mln_camera_delta delta = {.size = sizeof(mln_camera_delta)};
  delta.fields = MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE |
                 MLN_CAMERA_DELTA_BEARING | MLN_CAMERA_DELTA_ANCHOR;
  delta.offset = offset;
  delta.scale = 2.0;
  delta.bearing = 20.0;
  delta.anchor = anchor;
  MLN_TEST_OK(mln_map_apply_camera_delta(map, &delta, &probed, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  const uint64_t generation = mln_test_completion_generation(&completion);
  mln_test_completion_destroy(&completion);

  MLN_TEST_OK(probe.read_status);
  TEST_ASSERT_EQUAL_UINT64(generation, probe.snapshot.generation);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, probe.snapshot.view.camera.zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 20.0, probe.snapshot.view.camera.bearing);

  mln_map_projection projection = mln_test_map_create_projection(map);
  const mln_screen_point kept = projected_pixel(projection, dragged);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, anchor.x, kept.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, anchor.y, kept.y);
  mln_map_projection_release(projection);

  MLN_TEST_OK(mln_test_map_fence(map));
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  TEST_ASSERT_EQUAL_UINT64(
    before.latest_render_update_generation + 1,
    snapshot.latest_render_update_generation
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_status submit_delta_row(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_completion discard = mln_test_discard_completion();
  return mln_map_apply_camera_delta(
    *(const mln_map*)context, descriptor, &discard, diagnostic
  );
}

static void delta_undersized(void* descriptor) {
  ((mln_camera_delta*)descriptor)->size -= 1;
}
static void delta_unknown_field(void* descriptor) {
  ((mln_camera_delta*)descriptor)->fields = MLN_CAMERA_DELTA_MOMENTUM << 1U;
}
static void delta_bad_gesture(void* descriptor) {
  ((mln_camera_delta*)descriptor)->gesture_phase = MLN_GESTURE_PHASE_CANCEL + 1;
}
static void delta_unselected_garbage(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->offset.x = INFINITY;
  delta->scale = -1.0;
  delta->bearing = NAN;
  delta->pitch = NAN;
  delta->anchor.y = NAN;
}
static void delta_infinite_offset(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_OFFSET;
  delta->offset.x = INFINITY;
}
static void delta_zero_scale(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_SCALE;
  delta->scale = 0.0;
}
static void delta_infinite_bearing(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_BEARING;
  delta->bearing = INFINITY;
}
static void delta_anchor_alone(void* descriptor) {
  ((mln_camera_delta*)descriptor)->fields = MLN_CAMERA_DELTA_ANCHOR;
}
static void delta_animated_anchored_pan(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE |
                  MLN_CAMERA_DELTA_BEARING | MLN_CAMERA_DELTA_ANCHOR;
  delta->scale = 2.0;
  delta->bearing = 20.0;
  delta->animation.fields = MLN_ANIMATION_OPTION_DURATION;
  delta->animation.duration_ms = 300.0;
}

static void delta_momentum_outside_end(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_MOMENTUM;
  delta->gesture_phase = MLN_GESTURE_PHASE_UPDATE;
}
static void delta_momentum_with_duration(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_MOMENTUM;
  delta->gesture_phase = MLN_GESTURE_PHASE_END;
  delta->animation.fields = MLN_ANIMATION_OPTION_DURATION;
  delta->animation.duration_ms = 300.0;
}
static void delta_momentum_infinite_velocity(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_MOMENTUM;
  delta->gesture_phase = MLN_GESTURE_PHASE_END;
  delta->momentum.zoom.fields = MLN_ANCHORED_MOMENTUM_VELOCITY;
  delta->momentum.zoom.velocity = INFINITY;
}
static void delta_momentum_negative_option(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_MOMENTUM;
  delta->gesture_phase = MLN_GESTURE_PHASE_END;
  delta->momentum.pan.decay.fields = MLN_MOMENTUM_DECAY_MIN_SPEED;
  delta->momentum.pan.decay.min_speed = -1.0;
}
static void delta_momentum_zero_half_life(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_MOMENTUM;
  delta->gesture_phase = MLN_GESTURE_PHASE_END;
  delta->momentum.zoom.decay.fields = MLN_MOMENTUM_DECAY_HALF_LIFE_MS;
  delta->momentum.zoom.decay.half_life_ms = 0.0;
}
static void delta_momentum_eased_without_distance(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_MOMENTUM;
  delta->gesture_phase = MLN_GESTURE_PHASE_END;
  mln_momentum_decay* decay = &delta->momentum.zoom.decay;
  decay->fields = MLN_MOMENTUM_DECAY_MODEL | MLN_MOMENTUM_DECAY_DURATION_MS;
  decay->model = MLN_MOMENTUM_DECAY_MODEL_EASE;
  decay->duration_ms = 300.0;
}
static void delta_momentum_snap_without_spacing(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_MOMENTUM;
  delta->gesture_phase = MLN_GESTURE_PHASE_END;
  delta->momentum.bearing.fields = MLN_ANCHORED_MOMENTUM_SNAP;
  delta->momentum.bearing.snap.fields = MLN_MOMENTUM_SNAP_SPACING;
  delta->momentum.bearing.snap.spacing = 0.0;
}
static void delta_momentum_unselected_garbage(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_MOMENTUM;
  delta->gesture_phase = MLN_GESTURE_PHASE_END;
  delta->momentum.pan.velocity.x = NAN;
  delta->momentum.zoom.anchor.y = INFINITY;
  delta->momentum.zoom.decay.min_speed = -1.0;
  delta->momentum.bearing.snap.tolerance = -1.0;
}

static void delta_momentum_overshooting_curve(void* descriptor) {
  delta_momentum_eased_without_distance(descriptor);
  mln_momentum_decay* decay =
    &((mln_camera_delta*)descriptor)->momentum.zoom.decay;
  decay->fields |= MLN_MOMENTUM_DECAY_DISTANCE | MLN_MOMENTUM_DECAY_EASING;
  decay->distance = 1.0;
  decay->easing =
    (mln_unit_bezier){.x1 = 1.0 / 3, .y1 = 2.0, .x2 = 2.0 / 3, .y2 = 2.0};
}

static const mln_test_validation_case delta_cases[] = {
  {"undersized", delta_undersized, MLN_STATUS_INVALID_ARGUMENT, "valid size"},
  {"unknown field", delta_unknown_field, MLN_STATUS_INVALID_ARGUMENT,
   "unknown bits"},
  {"gesture phase out of range", delta_bad_gesture, MLN_STATUS_INVALID_ARGUMENT,
   "gesture phase is invalid"},
  {"unselected fields stay unread", delta_unselected_garbage, MLN_STATUS_OK,
   NULL},
  {"infinite offset", delta_infinite_offset, MLN_STATUS_INVALID_ARGUMENT,
   "must be finite"},
  {"zero scale", delta_zero_scale, MLN_STATUS_INVALID_ARGUMENT,
   "finite and positive"},
  {"infinite bearing", delta_infinite_bearing, MLN_STATUS_INVALID_ARGUMENT,
   "bearing and pitch must be finite"},
  {"anchor alone", delta_anchor_alone, MLN_STATUS_INVALID_ARGUMENT,
   "requires scale, bearing, or pitch"},
  {"animated anchored pan", delta_animated_anchored_pan,
   MLN_STATUS_INVALID_ARGUMENT, "only in an immediate delta"},
  {"momentum outside the end phase", delta_momentum_outside_end,
   MLN_STATUS_INVALID_ARGUMENT, "end phase carries momentum"},
  {"momentum with a duration", delta_momentum_with_duration,
   MLN_STATUS_INVALID_ARGUMENT, "sets only the transition ID"},
  {"infinite momentum velocity", delta_momentum_infinite_velocity,
   MLN_STATUS_INVALID_ARGUMENT, "velocities must be finite"},
  {"negative momentum option", delta_momentum_negative_option,
   MLN_STATUS_INVALID_ARGUMENT, "isn't negative"},
  {"zero momentum half-life", delta_momentum_zero_half_life,
   MLN_STATUS_INVALID_ARGUMENT, "positive half-life"},
  {"eased glide without a distance", delta_momentum_eased_without_distance,
   MLN_STATUS_INVALID_ARGUMENT, "needs a distance and a duration"},
  {"eased curve that overshoots", delta_momentum_overshooting_curve,
   MLN_STATUS_INVALID_ARGUMENT, "control points are within [0, 1]"},
  {"snap without a spacing", delta_momentum_snap_without_spacing,
   MLN_STATUS_INVALID_ARGUMENT, "positive spacing"},
  {"unselected momentum fields stay unread", delta_momentum_unselected_garbage,
   MLN_STATUS_OK, NULL},
};

static mln_camera_options finish_camera_query(mln_test_completion* query) {
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  MLN_TEST_OK(mln_test_completion_finish_value(query, &camera, sizeof(camera)));
  return camera;
}

static mln_camera_options camera_for_bounds(
  mln_map map, mln_lat_lng_bounds bounds, const mln_camera_fit_options* fit
) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_options));
  MLN_TEST_OK(
    mln_map_camera_for_lat_lng_bounds(map, bounds, fit, &query.descriptor, NULL)
  );
  return finish_camera_query(&query);
}

static mln_lat_lng_bounds bounds_for_camera(
  mln_map map, const mln_camera_options* camera
) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_lat_lng_bounds));
  MLN_TEST_OK(
    mln_map_lat_lng_bounds_for_camera(map, camera, &query.descriptor, NULL)
  );
  mln_lat_lng_bounds bounds = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &bounds, sizeof(bounds))
  );
  return bounds;
}

static void camera_fits_round_trip_through_the_visible_bounds(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 512);
  const mln_lat_lng_bounds region = {
    .southwest = {.latitude = 35.0, .longitude = -125.0},
    .northeast = {.latitude = 39.0, .longitude = -120.0},
  };

  const mln_camera_options fitted = camera_for_bounds(map, region, NULL);
  TEST_ASSERT_TRUE(fitted.fields & MLN_CAMERA_OPTION_CENTER);
  TEST_ASSERT_TRUE(fitted.fields & MLN_CAMERA_OPTION_ZOOM);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, -122.5, fitted.center.longitude);

  const mln_lat_lng_bounds visible = bounds_for_camera(map, &fitted);
  TEST_ASSERT_TRUE(
    visible.southwest.latitude <= region.southwest.latitude + 1e-6
  );
  TEST_ASSERT_TRUE(
    visible.southwest.longitude <= region.southwest.longitude + 1e-6
  );
  TEST_ASSERT_TRUE(
    visible.northeast.latitude >= region.northeast.latitude - 1e-6
  );
  TEST_ASSERT_TRUE(
    visible.northeast.longitude >= region.northeast.longitude - 1e-6
  );

  const mln_lat_lng corners[] = {region.southwest, region.northeast};
  mln_test_completion from_coordinates =
    mln_test_completion_default(sizeof(mln_camera_options));
  MLN_TEST_OK(mln_map_camera_for_lat_lngs(
    map, corners, 2, NULL, &from_coordinates.descriptor, NULL
  ));
  const mln_camera_options coordinate_fit =
    finish_camera_query(&from_coordinates);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, fitted.center.latitude, coordinate_fit.center.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, fitted.center.longitude, coordinate_fit.center.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, fitted.zoom, coordinate_fit.zoom);

  mln_test_completion from_geometry =
    mln_test_completion_default(sizeof(mln_camera_options));
  MLN_TEST_OK(mln_map_camera_for_geometry(
    map,
    MLN_BUFFER_LITERAL(
      "{\"type\":\"LineString\",\"coordinates\":[[-125,35],[-120,39]]}"
    ),
    NULL, &from_geometry.descriptor, NULL
  ));
  const mln_camera_options geometry_fit = finish_camera_query(&from_geometry);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, fitted.center.latitude, geometry_fit.center.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-9, fitted.center.longitude, geometry_fit.center.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, fitted.zoom, geometry_fit.zoom);

  // Padding leaves less of the viewport for the region, so the fit zooms out.
  mln_camera_fit_options fit = {.size = sizeof(mln_camera_fit_options)};
  fit.fields = MLN_CAMERA_FIT_OPTION_PADDING | MLN_CAMERA_FIT_OPTION_BEARING |
               MLN_CAMERA_FIT_OPTION_PITCH;
  fit.padding =
    (mln_edge_insets){.top = 64, .left = 64, .bottom = 64, .right = 64};
  fit.bearing = 5.0;
  fit.pitch = 15.0;
  const mln_camera_options padded = camera_for_bounds(map, region, &fit);
  TEST_ASSERT_LESS_THAN_DOUBLE(fitted.zoom, padded.zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, padded.bearing);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.0, padded.pitch);

  const mln_camera current = query_camera(map).camera;
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, current.zoom);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The three ways to name the area that a fit frames: bounds, coordinates, and
// a geometry, each covering the same region.
typedef enum fit_area { FIT_BOUNDS, FIT_COORDINATES, FIT_GEOMETRY } fit_area;

static const fit_area fit_areas[] = {FIT_BOUNDS, FIT_COORDINATES, FIT_GEOMETRY};
static const char* const fit_area_labels[] = {
  "bounds", "coordinates", "geometry"
};

// Wider than it is tall, so a map's width and height limit its fit
// differently.
static const mln_lat_lng_bounds fit_region = {
  .southwest = {.latitude = 35.0, .longitude = -125.0},
  .northeast = {.latitude = 39.0, .longitude = -115.0},
};

static mln_status submit_fit_query(
  mln_map map, fit_area area, const mln_camera_fit_options* fit,
  const mln_completion* completion
) {
  const mln_lat_lng corners[] = {fit_region.southwest, fit_region.northeast};
  switch (area) {
    case FIT_BOUNDS:
      return mln_map_camera_for_lat_lng_bounds(
        map, fit_region, fit, completion, NULL
      );
    case FIT_COORDINATES:
      return mln_map_camera_for_lat_lngs(
        map, corners, 2, fit, completion, NULL
      );
    case FIT_GEOMETRY:
      return mln_map_camera_for_geometry(
        map,
        MLN_BUFFER_LITERAL(
          "{\"type\":\"LineString\",\"coordinates\":[[-125,35],[-115,39]]}"
        ),
        fit, completion, NULL
      );
  }
  return MLN_STATUS_INVALID_ARGUMENT;
}

static mln_status submit_fit_command(
  mln_map map, fit_area area, const mln_camera_fit_update* update,
  const mln_completion* completion
) {
  const mln_lat_lng corners[] = {fit_region.southwest, fit_region.northeast};
  switch (area) {
    case FIT_BOUNDS:
      return mln_map_fit_camera_to_lat_lng_bounds(
        map, fit_region, update, completion, NULL
      );
    case FIT_COORDINATES:
      return mln_map_fit_camera_to_lat_lngs(
        map, corners, 2, update, completion, NULL
      );
    case FIT_GEOMETRY:
      return mln_map_fit_camera_to_geometry(
        map,
        MLN_BUFFER_LITERAL(
          "{\"type\":\"LineString\",\"coordinates\":[[-125,35],[-115,39]]}"
        ),
        update, completion, NULL
      );
  }
  return MLN_STATUS_INVALID_ARGUMENT;
}

static mln_camera_options query_fit(
  mln_map map, fit_area area, const mln_camera_fit_options* fit
) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_options));
  MLN_TEST_OK(submit_fit_query(map, area, fit, &query.descriptor));
  return finish_camera_query(&query);
}

static void assert_camera_matches_fit(
  const mln_camera* camera, const mln_camera_options* fit, const char* label
) {
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    1e-9, fit->center.latitude, camera->center.latitude, label
  );
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
    1e-9, fit->center.longitude, camera->center.longitude, label
  );
  TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, fit->zoom, camera->zoom, label);
}

static void a_fit_command_moves_the_camera_where_the_query_fits(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 512);
  mln_camera_fit_update update = {.size = sizeof(mln_camera_fit_update)};
  update.fit.size = sizeof(mln_camera_fit_options);
  update.fit.fields = MLN_CAMERA_FIT_OPTION_PADDING |
                      MLN_CAMERA_FIT_OPTION_BEARING |
                      MLN_CAMERA_FIT_OPTION_PITCH;
  update.fit.padding =
    (mln_edge_insets){.top = 64, .left = 32, .bottom = 16, .right = 8};
  update.fit.bearing = 5.0;
  update.fit.pitch = 15.0;
  mln_camera_options start = {.size = sizeof(mln_camera_options)};
  start.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                 MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
                 MLN_CAMERA_OPTION_PADDING;
  for (size_t index = 0; index < 3; index += 1) {
    const char* label = fit_area_labels[index];
    jump(map, start);
    const mln_camera_options fit =
      query_fit(map, fit_areas[index], &update.fit);
    MLN_TEST_AWAIT_OK(
      submit_fit_command(map, fit_areas[index], &update, &completion.descriptor)
    );
    const mln_camera camera = query_camera(map).camera;
    assert_camera_matches_fit(&camera, &fit, label);
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, 5.0, camera.bearing, label);
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, 15.0, camera.pitch, label);
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(1e-9, 64.0, camera.padding.top, label);
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The command resolves its camera when the map runs it, so it fits the size
// that a resize sent just before it leaves.
static void a_fit_command_fits_the_map_that_earlier_commands_leave(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 512);
  mln_camera_fit_update update = {.size = sizeof(mln_camera_fit_update)};
  for (size_t index = 0; index < 3; index += 1) {
    const char* label = fit_area_labels[index];
    MLN_TEST_AWAIT_OK(mln_map_resize(
      map, (mln_logical_size){.width = 512, .height = 512},
      &completion.descriptor, NULL
    ));
    const mln_camera_options square = query_fit(map, fit_areas[index], NULL);

    MLN_TEST_OK(mln_map_resize(
      map, (mln_logical_size){.width = 1024, .height = 512}, NULL, NULL
    ));
    MLN_TEST_AWAIT_OK(
      submit_fit_command(map, fit_areas[index], &update, &completion.descriptor)
    );
    const mln_camera camera = query_camera(map).camera;
    const mln_camera_options wide = query_fit(map, fit_areas[index], NULL);
    assert_camera_matches_fit(&camera, &wide, label);
    TEST_ASSERT_GREATER_THAN_DOUBLE_MESSAGE(
      square.zoom + 0.5, camera.zoom, label
    );
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct padding_row {
  const char* label;
  mln_edge_insets padding;
  mln_status expected;
} padding_row;

static const padding_row padding_rows[] = {
  {"left and right fill the width",
   {.top = 0, .left = 256, .bottom = 0, .right = 256},
   MLN_STATUS_INVALID_ARGUMENT},
  {"top and bottom pass the height",
   {.top = 400, .left = 0, .bottom = 200, .right = 0},
   MLN_STATUS_INVALID_ARGUMENT},
  {"just under the width and height",
   {.top = 255.5, .left = 255.5, .bottom = 255.5, .right = 255.5},
   MLN_STATUS_OK},
};

static void fits_reject_padding_that_leaves_no_room(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 512);
  const size_t row_count = sizeof(padding_rows) / sizeof(*padding_rows);
  for (size_t row_index = 0; row_index < row_count; row_index += 1) {
    const padding_row* row = &padding_rows[row_index];
    mln_camera_fit_update update = {.size = sizeof(mln_camera_fit_update)};
    update.fit.size = sizeof(mln_camera_fit_options);
    update.fit.fields = MLN_CAMERA_FIT_OPTION_PADDING;
    update.fit.padding = row->padding;
    for (size_t index = 0; index < 3; index += 1) {
      char label[96];
      snprintf(
        label, sizeof(label), "%s, %s", row->label, fit_area_labels[index]
      );
      mln_test_completion query =
        mln_test_completion_default(sizeof(mln_camera_options));
      MLN_TEST_OK(
        submit_fit_query(map, fit_areas[index], &update.fit, &query.descriptor)
      );
      TEST_ASSERT_EQUAL_INT_MESSAGE(
        row->expected, mln_test_completion_finish(&query), label
      );
      mln_test_completion_destroy(&query);

      const mln_camera before = query_camera(map).camera;
      mln_test_completion command = mln_test_completion_default(0);
      MLN_TEST_OK(
        submit_fit_command(map, fit_areas[index], &update, &command.descriptor)
      );
      TEST_ASSERT_EQUAL_INT_MESSAGE(
        row->expected, mln_test_completion_finish(&command), label
      );
      if (row->expected != MLN_STATUS_OK) {
        TEST_ASSERT_NOT_NULL_MESSAGE(
          strstr(mln_test_completion_diagnostic(&command), "padding"), label
        );
        const mln_camera after = query_camera(map).camera;
        TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
          &before, &after, sizeof(before), label
        );
      }
      mln_test_completion_destroy(&command);
    }
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void visible_bounds_unwrap_across_the_antimeridian(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 512);
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.center.latitude = 0.0;
  // A center on the antimeridian itself unprojects to 180 or -180 depending
  // on floating-point rounding, which flips the hull to the other side, so the
  // center sits just west of it.
  camera.center.longitude = 170.0;
  camera.zoom = 1.0;

  // At zoom 1 the world is 1024 pixels wide, so 512 pixels span 180 degrees.
  const mln_lat_lng_bounds hull = bounds_for_camera(map, &camera);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 80.0, hull.southwest.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 260.0, hull.northeast.longitude);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void visible_bounds_hold_every_corner_of_a_rotated_view(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 512);
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING;
  camera.center = (mln_lat_lng){.latitude = 0.0, .longitude = 0.0};
  camera.zoom = 3.0;
  camera.bearing = 0.0;
  const mln_lat_lng_bounds upright = bounds_for_camera(map, &camera);

  // At zoom 3 the world is 4096 pixels wide, so 512 pixels span 45 degrees.
  // Turned 45 degrees, the square view's corners reach sqrt(2) times as far.
  camera.bearing = 45.0;
  const mln_lat_lng_bounds rotated = bounds_for_camera(map, &camera);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, 45.0, upright.northeast.longitude - upright.southwest.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, 45.0 * sqrt(2.0),
    rotated.northeast.longitude - rotated.southwest.longitude
  );
  TEST_ASSERT_GREATER_THAN_DOUBLE(
    upright.northeast.latitude, rotated.northeast.latitude
  );
  TEST_ASSERT_LESS_THAN_DOUBLE(
    upright.southwest.latitude, rotated.southwest.latitude
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static double jumped_longitude(mln_map map, double longitude) {
  mln_camera_options camera = {.size = sizeof(mln_camera_options)};
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.center.latitude = 0.0;
  camera.center.longitude = longitude;
  camera.zoom = 2.0;
  jump(map, camera);
  return query_camera(map).camera.center.longitude;
}

static mln_camera_bounds read_bounds(mln_runtime runtime, mln_map map) {
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));
  return snapshot.bounds;
}

static void camera_calls_reject_what_they_cannot_express(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  {
    mln_completion rejected = mln_test_discard_completion();
    MLN_TEST_INVALID(mln_map_update_camera(map, NULL, &rejected, NULL));
    MLN_TEST_INVALID(mln_map_get_camera(map, NULL, NULL));

    const mln_camera_update defaults = {
      .size = sizeof(mln_camera_update),
      .camera = {.size = sizeof(mln_camera_options)}
    };
    mln_test_run_validation_table(
      update_cases, sizeof(update_cases) / sizeof(*update_cases), &defaults,
      sizeof(defaults), submit_update_row, &map
    );
  }
  {
    const mln_camera_delta defaults = {.size = sizeof(mln_camera_delta)};
    mln_test_run_validation_table(
      delta_cases, sizeof(delta_cases) / sizeof(*delta_cases), &defaults,
      sizeof(defaults), submit_delta_row, &map
    );
    mln_completion discard = mln_test_discard_completion();
    MLN_TEST_INVALID(mln_map_apply_camera_delta(map, NULL, &discard, NULL));
  }
  {
    mln_completion operation = mln_test_discard_completion();
    mln_camera_fit_options fit = {.size = sizeof(mln_camera_fit_options)};
    fit.size = sizeof(mln_camera_fit_options) - 1;
    const mln_lat_lng_bounds bounds = {
      .southwest = {.latitude = -10.0, .longitude = -10.0},
      .northeast = {.latitude = 10.0, .longitude = 10.0},
    };
    MLN_TEST_INVALID(
      mln_map_camera_for_lat_lng_bounds(map, bounds, &fit, &operation, NULL)
    );
    const mln_lat_lng_bounds inverted = {
      .southwest = bounds.northeast, .northeast = bounds.southwest
    };
    MLN_TEST_INVALID(mln_map_camera_for_lat_lng_bounds(
      map, inverted, NULL, &operation, MLN_TEST_DIAGNOSTIC
    ));
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "southwest"));
    fit = (mln_camera_fit_options){.size = sizeof(mln_camera_fit_options)};
    fit.fields = MLN_CAMERA_FIT_OPTION_BEARING;
    fit.bearing = NAN;
    MLN_TEST_INVALID(
      mln_map_camera_for_lat_lng_bounds(map, bounds, &fit, &operation, NULL)
    );
    const mln_lat_lng coordinate = {.latitude = 0.0, .longitude = 0.0};
    MLN_TEST_INVALID(
      mln_map_camera_for_lat_lngs(map, NULL, 1, NULL, &operation, NULL)
    );
    MLN_TEST_INVALID(
      mln_map_camera_for_lat_lngs(map, &coordinate, 0, NULL, &operation, NULL)
    );
    MLN_TEST_INVALID(
      mln_map_camera_for_lat_lngs(map, &coordinate, 1, NULL, NULL, NULL)
    );
    MLN_TEST_INVALID(mln_map_camera_for_geometry(
      map, (mln_buffer_view){0}, NULL, &operation, NULL
    ));
    MLN_TEST_INVALID(
      mln_map_lat_lng_bounds_for_camera(map, NULL, &operation, NULL)
    );
    mln_camera_options undersized = {.size = sizeof(mln_camera_options)};
    undersized.size -= 1;
    MLN_TEST_INVALID(
      mln_map_lat_lng_bounds_for_camera(map, &undersized, &operation, NULL)
    );
  }
  {
    const mln_camera_fit_update defaults = {
      .size = sizeof(mln_camera_fit_update)
    };
    mln_test_run_validation_table(
      fit_update_cases, sizeof(fit_update_cases) / sizeof(*fit_update_cases),
      &defaults, sizeof(defaults), submit_fit_update_row, &map
    );
    mln_completion command = mln_test_discard_completion();
    const mln_lat_lng_bounds bounds = {
      .southwest = {.latitude = 10.0, .longitude = -10.0},
      .northeast = {.latitude = -10.0, .longitude = 10.0},
    };
    MLN_TEST_INVALID(mln_map_fit_camera_to_lat_lng_bounds(
      map, bounds, &defaults, &command, MLN_TEST_DIAGNOSTIC
    ));
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "southwest"));
    MLN_TEST_INVALID(
      mln_map_fit_camera_to_lat_lng_bounds(map, bounds, NULL, &command, NULL)
    );
    const mln_lat_lng coordinate = {.latitude = 0.0, .longitude = 0.0};
    MLN_TEST_INVALID(
      mln_map_fit_camera_to_lat_lngs(map, NULL, 1, &defaults, &command, NULL)
    );
    MLN_TEST_INVALID(mln_map_fit_camera_to_lat_lngs(
      map, &coordinate, 0, &defaults, &command, NULL
    ));
    MLN_TEST_INVALID(
      mln_map_fit_camera_to_lat_lngs(map, &coordinate, 1, NULL, &command, NULL)
    );
    MLN_TEST_INVALID(mln_map_fit_camera_to_geometry(
      map, MLN_BUFFER_LITERAL("{\"type\":\"Point\""), &defaults, &command, NULL
    ));
    MLN_TEST_INVALID(mln_map_fit_camera_to_geometry(
      map, MLN_BUFFER_LITERAL("{\"type\":\"MultiPoint\",\"coordinates\":[]}"),
      &defaults, &command, MLN_TEST_DIAGNOSTIC
    ));
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "at least one"));
    MLN_TEST_INVALID(mln_map_fit_camera_to_geometry(
      map, MLN_BUFFER_LITERAL("{\"type\":\"Point\",\"coordinates\":[0,0]}"),
      NULL, &command, NULL
    ));
  }
  {
    mln_bound_options options = {.size = sizeof(mln_bound_options)};
    options.size = sizeof(mln_bound_options) - 1;
    mln_completion command = mln_test_discard_completion();
    MLN_TEST_INVALID(mln_map_set_bounds(map, NULL, &command, NULL));
    MLN_TEST_INVALID(mln_map_set_bounds(map, &options, &command, NULL));
    options = (mln_bound_options){.size = sizeof(mln_bound_options)};
    options.fields = UINT32_C(1) << 31;
    MLN_TEST_INVALID(mln_map_set_bounds(map, &options, &command, NULL));
    options = (mln_bound_options){.size = sizeof(mln_bound_options)};
    options.fields = MLN_BOUND_OPTION_BOUNDS | MLN_BOUND_OPTION_UNBOUNDED;
    MLN_TEST_INVALID(mln_map_set_bounds(map, &options, &command, NULL));
    options = (mln_bound_options){.size = sizeof(mln_bound_options)};
    options.fields = MLN_BOUND_OPTION_MIN_ZOOM | MLN_BOUND_OPTION_MAX_ZOOM;
    options.min_zoom = 10.0;
    options.max_zoom = 5.0;
    MLN_TEST_INVALID(
      mln_map_set_bounds(map, &options, &command, MLN_TEST_DIAGNOSTIC)
    );
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "min_zoom"));
  }
  {
    mln_free_camera_options options = {.size = sizeof(mln_free_camera_options)};
    options.size = sizeof(mln_free_camera_options) - 1;
    mln_completion completion = mln_test_discard_completion();
    MLN_TEST_INVALID(
      mln_map_set_free_camera_options(map, NULL, &completion, NULL)
    );
    MLN_TEST_INVALID(
      mln_map_set_free_camera_options(map, &options, &completion, NULL)
    );
    options =
      (mln_free_camera_options){.size = sizeof(mln_free_camera_options)};
    options.fields = UINT32_C(1) << 31;
    MLN_TEST_INVALID(
      mln_map_set_free_camera_options(map, &options, &completion, NULL)
    );
    options =
      (mln_free_camera_options){.size = sizeof(mln_free_camera_options)};
    options.fields = MLN_FREE_CAMERA_OPTION_POSITION;
    options.position.z = NAN;
    MLN_TEST_INVALID(
      mln_map_set_free_camera_options(map, &options, &completion, NULL)
    );
    options =
      (mln_free_camera_options){.size = sizeof(mln_free_camera_options)};
    options.fields = MLN_FREE_CAMERA_OPTION_ORIENTATION;
    options.orientation = (mln_quaternion){0};
    MLN_TEST_INVALID(mln_map_set_free_camera_options(
      map, &options, &completion, MLN_TEST_DIAGNOSTIC
    ));
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "zero length"));
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Starts a camera update toward zoom 1 from zoom 0, and reads the zoom once the
// command has started.
static double zoom_after(mln_map map, mln_camera_update update) {
  mln_camera_options start = {.size = sizeof(mln_camera_options)};
  start.fields = MLN_CAMERA_OPTION_ZOOM;
  start.zoom = 0.0;
  jump(map, start);
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = 1.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  const double zoom = query_camera(map).camera.zoom;
  MLN_TEST_AWAIT_OK(
    mln_map_cancel_camera_transitions(map, &completion.descriptor, NULL)
  );
  return zoom;
}

// A fly animation longer than its max_duration_ms, or too slow to time at all,
// moves the camera at once. The durations that animate are long enough that
// the camera can't have reached the target when the zoom is read.
static void flights_too_long_to_run_jump(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 512);
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.mode = MLN_CAMERA_UPDATE_MODE_FLY;

  update.animation.fields = MLN_ANIMATION_OPTION_VELOCITY;
  update.animation.velocity = nextafter(0.0, 1.0);
  TEST_ASSERT_EQUAL_DOUBLE(1.0, zoom_after(map, update));
  update.animation.velocity = 1e-9;
  TEST_ASSERT_LESS_THAN_DOUBLE(1.0, zoom_after(map, update));
  update.animation.fields |= MLN_ANIMATION_OPTION_MAX_DURATION;
  update.animation.max_duration_ms = 1000.0;
  TEST_ASSERT_EQUAL_DOUBLE(1.0, zoom_after(map, update));

  update.animation.fields =
    MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_MAX_DURATION;
  update.animation.duration_ms = 1e9;
  update.animation.max_duration_ms = 1e9 - 1.0;
  TEST_ASSERT_EQUAL_DOUBLE(1.0, zoom_after(map, update));
  update.animation.max_duration_ms = 1e9;
  TEST_ASSERT_LESS_THAN_DOUBLE(1.0, zoom_after(map, update));

  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.animation.max_duration_ms = 1.0;
  TEST_ASSERT_LESS_THAN_DOUBLE(1.0, zoom_after(map, update));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The unbounded state is distinct from world bounds, which the
// southwest/northeast pair alone cannot express: world bounds stop a jump that
// unbounded wraps. An unbounded center reports the whole world as its bounds.
static void unbounded_and_world_bounds_stay_distinct(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_bounds snapshot = read_bounds(runtime, map);
  TEST_ASSERT_TRUE(snapshot.unbounded);
  TEST_ASSERT_EQUAL_DOUBLE(-90.0, snapshot.bounds.southwest.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(-180.0, snapshot.bounds.southwest.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(90.0, snapshot.bounds.northeast.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(180.0, snapshot.bounds.northeast.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, -160.0, jumped_longitude(map, 200.0));

  mln_bound_options world = {.size = sizeof(mln_bound_options)};
  world.fields = MLN_BOUND_OPTION_BOUNDS;
  world.bounds.southwest.latitude = -90.0;
  world.bounds.southwest.longitude = -180.0;
  world.bounds.northeast.latitude = 90.0;
  world.bounds.northeast.longitude = 180.0;
  MLN_TEST_AWAIT_OK(
    mln_map_set_bounds(map, &world, &completion.descriptor, NULL)
  );

  snapshot = read_bounds(runtime, map);
  TEST_ASSERT_FALSE(snapshot.unbounded);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 180.0, snapshot.bounds.northeast.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 180.0, jumped_longitude(map, 200.0));

  mln_bound_options unbounded = {.size = sizeof(mln_bound_options)};
  unbounded.fields = MLN_BOUND_OPTION_UNBOUNDED;
  MLN_TEST_AWAIT_OK(
    mln_map_set_bounds(map, &unbounded, &completion.descriptor, NULL)
  );

  snapshot = read_bounds(runtime, map);
  TEST_ASSERT_TRUE(snapshot.unbounded);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, -160.0, jumped_longitude(map, 200.0));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(camera_calls_reject_what_they_cannot_express);
  RUN_TEST(flights_too_long_to_run_jump);
  RUN_TEST(every_camera_field_round_trips_through_the_snapshot);
  RUN_TEST(queued_immediate_deltas_compose_exactly);
  RUN_TEST(every_camera_delta_field_follows_its_convention);
  RUN_TEST(an_immediate_anchored_pan_keeps_the_dragged_coordinate);
  RUN_TEST(camera_fits_round_trip_through_the_visible_bounds);
  RUN_TEST(a_fit_command_moves_the_camera_where_the_query_fits);
  RUN_TEST(a_fit_command_fits_the_map_that_earlier_commands_leave);
  RUN_TEST(fits_reject_padding_that_leaves_no_room);
  RUN_TEST(visible_bounds_unwrap_across_the_antimeridian);
  RUN_TEST(visible_bounds_hold_every_corner_of_a_rotated_view);
  RUN_TEST(unbounded_and_world_bounds_stay_distinct);
}
