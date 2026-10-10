// The map camera: absolute updates round-trip through the snapshot and the
// ordered query, relative deltas follow their documented conventions and apply
// atomically, fits round-trip through the visible bounds, constraints clamp
// what they select, and ordered conversions and scales observe the committed
// camera.

#include <math.h>

#include "support/test_support.h"

static mln_map create_square_map(mln_runtime runtime, uint32_t side) {
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = side, .height = side, .scale_factor = 1.0};
  return mln_test_create_map_with_options(runtime, &options);
}

static mln_camera_options test_camera(void) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH |
                  MLN_CAMERA_OPTION_PADDING | MLN_CAMERA_OPTION_ANCHOR;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 11.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  camera.padding = (mln_edge_insets){1.0, 2.0, 3.0, 4.0};
  camera.anchor = (mln_screen_point){25.0, 30.0};
  return camera;
}

static void jump(mln_map map, mln_camera_options camera) {
  mln_camera_update update = mln_camera_update_default();
  update.camera = camera;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

static mln_camera_query_result query_camera(mln_map map) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_camera_query_result));
  MLN_TEST_OK(mln_map_camera_query(map, &query.descriptor, NULL));
  mln_camera_query_result result = {.size = sizeof(mln_camera_query_result)};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &result, sizeof(result))
  );
  return result;
}

static mln_lat_lng coordinate_at(mln_map map, mln_screen_point point) {
  mln_test_completion query = mln_test_completion_default(sizeof(mln_lat_lng));
  MLN_TEST_OK(mln_map_lat_lng_for_pixel(map, point, &query.descriptor, NULL));
  mln_lat_lng coordinate = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &coordinate, sizeof(coordinate))
  );
  return coordinate;
}

static double meters_per_pixel(mln_map map, double latitude) {
  mln_test_completion query = mln_test_completion_default(sizeof(double));
  MLN_TEST_OK(
    mln_map_meters_per_pixel_at_latitude(map, latitude, &query.descriptor, NULL)
  );
  double meters = 0.0;
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &meters, sizeof(meters))
  );
  return meters;
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
static void update_bad_gesture(void* descriptor) {
  ((mln_camera_update*)descriptor)->gesture_phase =
    MLN_GESTURE_PHASE_CANCEL + 1;
}
static void camera_undersized(void* descriptor) {
  ((mln_camera_update*)descriptor)->camera.size -= 1;
}
static void camera_unknown_field(void* descriptor) {
  ((mln_camera_update*)descriptor)->camera.fields = UINT32_C(1) << 31;
}
static void camera_latitude_past_the_pole(void* descriptor) {
  mln_camera_options* camera = &((mln_camera_update*)descriptor)->camera;
  camera->fields = MLN_CAMERA_OPTION_CENTER;
  camera->latitude = 90.5;
}
static void camera_nan_zoom(void* descriptor) {
  mln_camera_options* camera = &((mln_camera_update*)descriptor)->camera;
  camera->fields = MLN_CAMERA_OPTION_ZOOM;
  camera->zoom = NAN;
}
static void camera_infinite_fov(void* descriptor) {
  mln_camera_options* camera = &((mln_camera_update*)descriptor)->camera;
  camera->fields = MLN_CAMERA_OPTION_FOV;
  camera->field_of_view = INFINITY;
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
static void animation_undersized(void* descriptor) {
  ((mln_camera_update*)descriptor)->animation.size -= 1;
}
static void animation_unknown_field(void* descriptor) {
  ((mln_camera_update*)descriptor)->animation.fields = UINT32_C(1) << 31;
}
static void animation_negative_duration(void* descriptor) {
  mln_animation_options* animation =
    &((mln_camera_update*)descriptor)->animation;
  animation->fields = MLN_ANIMATION_OPTION_DURATION;
  animation->duration_ms = -1.0;
}
static void animation_duration_past_the_native_range(void* descriptor) {
  mln_animation_options* animation =
    &((mln_camera_update*)descriptor)->animation;
  animation->fields = MLN_ANIMATION_OPTION_DURATION;
  animation->duration_ms = 1e300;
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
  {"gesture phase out of range", update_bad_gesture,
   MLN_STATUS_INVALID_ARGUMENT, "mode or gesture phase"},
  {"undersized camera", camera_undersized, MLN_STATUS_INVALID_ARGUMENT,
   "size is too small"},
  {"unknown camera field", camera_unknown_field, MLN_STATUS_INVALID_ARGUMENT,
   "unknown bits"},
  {"latitude past the pole", camera_latitude_past_the_pole,
   MLN_STATUS_INVALID_ARGUMENT, "within [-90, 90]"},
  {"NaN zoom", camera_nan_zoom, MLN_STATUS_INVALID_ARGUMENT, "must be finite"},
  {"infinite field of view", camera_infinite_fov, MLN_STATUS_INVALID_ARGUMENT,
   "must be finite"},
  {"negative padding", camera_negative_padding, MLN_STATUS_INVALID_ARGUMENT,
   "greater than or equal to 0"},
  {"NaN anchor", camera_nan_anchor, MLN_STATUS_INVALID_ARGUMENT,
   "screen point values must be finite"},
  {"undersized animation", animation_undersized, MLN_STATUS_INVALID_ARGUMENT,
   "size is too small"},
  {"unknown animation field", animation_unknown_field,
   MLN_STATUS_INVALID_ARGUMENT, "unknown bits"},
  {"negative duration", animation_negative_duration,
   MLN_STATUS_INVALID_ARGUMENT, "native duration range"},
  {"duration past the native range", animation_duration_past_the_native_range,
   MLN_STATUS_INVALID_ARGUMENT, "native duration range"},
  {"zero velocity", animation_zero_velocity, MLN_STATUS_INVALID_ARGUMENT,
   "positive and finite"},
  {"easing outside the unit square", animation_easing_outside_the_unit_square,
   MLN_STATUS_INVALID_ARGUMENT, "within [0, 1]"},
};

// The update is copied at submission: editing it afterwards changes nothing.
// The ordered query and the snapshot both observe the commit, and the query
// reports the generation that published it.
static void camera_snapshot_command_copy_and_disposition_are_ordered(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  mln_map_snapshot before = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_snapshot_get(map, &before, NULL));

  mln_camera_update update = mln_camera_update_default();
  update.camera = test_camera();
  update.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  update.animation.fields |= MLN_ANIMATION_OPTION_TRANSITION_ID;
  update.animation.transition_id = UINT64_C(77);
  mln_test_completion command = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_update_camera(map, &update, &command.descriptor, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&command));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_COMMITTED, mln_test_completion_disposition(&command)
  );
  const uint64_t command_generation = mln_test_completion_generation(&command);
  TEST_ASSERT_GREATER_THAN_UINT64(before.generation, command_generation);
  mln_test_completion_destroy(&command);

  update.camera.longitude = 12.0;
  update.camera.zoom = 1.0;
  update.camera.padding.left = 999.0;

  const mln_camera_query_result result = query_camera(map);
  TEST_ASSERT_EQUAL_DOUBLE(-122.4194, result.camera.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(11.0, result.camera.zoom);
  TEST_ASSERT_EQUAL_DOUBLE(2.0, result.camera.padding.left);
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(command_generation, result.generation);

  mln_map_snapshot after = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_snapshot_get(map, &after, NULL));
  TEST_ASSERT_GREATER_OR_EQUAL_UINT64(command_generation, after.generation);
  TEST_ASSERT_EQUAL_DOUBLE(-122.4194, after.camera.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(11.0, after.camera.zoom);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// One absolute camera field and the value MapLibre must hand back for it.
typedef struct camera_row {
  const char* label;
  mln_camera_options camera;
} camera_row;

static const camera_row camera_rows[] = {
  {"center",
   {.size = sizeof(mln_camera_options),
    .fields = MLN_CAMERA_OPTION_CENTER,
    .latitude = -33.8688,
    .longitude = 151.2093}},
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
  const char* label, const mln_camera_options* sent,
  const mln_camera_options* got
) {
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(
    sent->fields, got->fields & sent->fields, label
  );
  if ((sent->fields & MLN_CAMERA_OPTION_CENTER) != 0U) {
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->latitude, got->latitude, label
    );
    TEST_ASSERT_DOUBLE_WITHIN_MESSAGE(
      1e-9, sent->longitude, got->longitude, label
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

// Each field a jump selects reads back from the ordered query, the map
// snapshot, and the camera snapshot, and the fields it leaves out keep their
// values.
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
    MLN_TEST_OK(mln_map_snapshot_get(map, &snapshot, NULL));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT64(
      queried.generation, snapshot.generation
    );
    assert_camera_field(row->label, &row->camera, &snapshot.camera);

    // Every earlier row's field is still in place.
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

// Immediate deltas resolve against the camera as each one runs, so a queue of
// them composes exactly without awaiting any but the last.
static void queued_immediate_deltas_compose_exactly(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  jump(map, test_camera());

  const mln_completion discard = mln_test_discard_completion();
  mln_camera_delta delta = mln_camera_delta_default();
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

// One relative command. Its check follows mln_camera_delta's documented
// convention for the fields it selects.
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
  {"pan right", MLN_CAMERA_DELTA_OFFSET, {.x = 64.0, .y = 0.0}},
  {"pan down", MLN_CAMERA_DELTA_OFFSET, {.x = 0.0, .y = 48.0}},
  {"pan up and left", MLN_CAMERA_DELTA_OFFSET, {.x = -32.0, .y = -16.0}},
  {"scale in", MLN_CAMERA_DELTA_SCALE, {0}, 2.0},
  {"scale out", MLN_CAMERA_DELTA_SCALE, {0}, 0.25},
  {"scale in about a corner",
   MLN_CAMERA_DELTA_SCALE | MLN_CAMERA_DELTA_ANCHOR,
   {0},
   2.0,
   .anchor = {.x = 0.0, .y = 0.0}},
  {"scale out about an edge",
   MLN_CAMERA_DELTA_SCALE | MLN_CAMERA_DELTA_ANCHOR,
   {0},
   0.5,
   .anchor = {.x = 256.0, .y = 128.0}},
  {"rotate clockwise", MLN_CAMERA_DELTA_BEARING, .bearing = 30.0},
  {"rotate counterclockwise", MLN_CAMERA_DELTA_BEARING, .bearing = -50.0},
  {"rotate about a corner", MLN_CAMERA_DELTA_BEARING | MLN_CAMERA_DELTA_ANCHOR,
   .bearing = 20.0, .anchor = {.x = 32.0, .y = 32.0}},
  {"tilt away from straight down", MLN_CAMERA_DELTA_PITCH, .pitch = 20.0},
  {"tilt back toward straight down", MLN_CAMERA_DELTA_PITCH, .pitch = -5.0},
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

// OFFSET moves the map content by its offset, so the coordinate at the
// offset's opposite lands on the center. SCALE multiplies the scale, adding
// log2(scale) to the zoom, and BEARING and PITCH add their degrees: a positive
// pitch tilts away from straight down. An anchored change keeps the coordinate
// under the anchor in place; an unanchored one keeps the center that the pan
// left.
static void every_camera_delta_field_follows_its_convention(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 256);
  mln_camera_options start = mln_camera_options_default();
  start.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                 MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  start.latitude = 10.0;
  start.longitude = 20.0;
  start.zoom = 4.0;
  jump(map, start);
  const mln_screen_point center = {.x = 128.0, .y = 128.0};

  for (size_t index = 0; index < sizeof(delta_rows) / sizeof(*delta_rows);
       index += 1) {
    const delta_row* row = &delta_rows[index];
    const bool pans = (row->fields & MLN_CAMERA_DELTA_OFFSET) != 0U;
    const bool anchored = (row->fields & MLN_CAMERA_DELTA_ANCHOR) != 0U;
    const mln_camera_options before = query_camera(map).camera;
    const mln_lat_lng before_center = {before.latitude, before.longitude};
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

    mln_camera_delta delta = mln_camera_delta_default();
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
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(
      MLN_COMMAND_DISPOSITION_COMMITTED,
      mln_test_completion_disposition(&completion), row->label
    );
    mln_test_completion_destroy(&completion);

    const mln_camera_options after = query_camera(map).camera;
    const mln_lat_lng after_center = {after.latitude, after.longitude};
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

static mln_map_projection create_projection(mln_map map) {
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_map_projection_create(map, &projection, NULL));
  return projection;
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

// A positive offset moves the content right and down, so the coordinate that
// was at the center follows the pointer.
static void an_offset_moves_the_content_with_the_pointer(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 256);
  mln_camera_options start = mln_camera_options_default();
  start.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  start.latitude = 10.0;
  start.longitude = 20.0;
  start.zoom = 4.0;
  jump(map, start);
  const mln_camera_options before = query_camera(map).camera;

  mln_camera_delta delta = mln_camera_delta_default();
  delta.fields = MLN_CAMERA_DELTA_OFFSET;
  delta.offset = (mln_screen_point){.x = 10.0, .y = 6.0};
  MLN_TEST_AWAIT_OK(
    mln_map_apply_camera_delta(map, &delta, &completion.descriptor, NULL)
  );

  mln_map_projection projection = create_projection(map);
  const mln_screen_point moved = projected_pixel(
    projection,
    (mln_lat_lng){.latitude = before.latitude, .longitude = before.longitude}
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 138.0, moved.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 134.0, moved.y);
  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// One delta turns and tilts about an off-center anchor in one command: the
// coordinate under the anchor stays there, and bearing and pitch each move by
// their delta.
static void one_delta_turns_and_tilts_about_its_anchor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 256);
  mln_camera_options start = mln_camera_options_default();
  start.fields =
    MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM | MLN_CAMERA_OPTION_PITCH;
  start.latitude = 10.0;
  start.longitude = 20.0;
  start.zoom = 4.0;
  start.pitch = 10.0;
  jump(map, start);
  const mln_screen_point anchor = {.x = 64.0, .y = 192.0};
  const mln_lat_lng under_anchor = coordinate_at(map, anchor);

  mln_camera_delta delta = mln_camera_delta_default();
  delta.fields =
    MLN_CAMERA_DELTA_BEARING | MLN_CAMERA_DELTA_PITCH | MLN_CAMERA_DELTA_ANCHOR;
  delta.bearing = 25.0;
  delta.pitch = 15.0;
  delta.anchor = anchor;
  MLN_TEST_AWAIT_OK(
    mln_map_apply_camera_delta(map, &delta, &completion.descriptor, NULL)
  );

  mln_map_projection projection = create_projection(map);
  const mln_screen_point kept = projected_pixel(projection, under_anchor);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, anchor.x, kept.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, anchor.y, kept.y);
  mln_camera_options camera = mln_camera_options_default();
  MLN_TEST_OK(mln_map_projection_get_camera(projection, &camera, NULL));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 25.0, camera.bearing);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 25.0, camera.pitch);
  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
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
  probe->read_status = mln_map_snapshot_get(probe->map, &probe->snapshot, NULL);
  probe->inner.callback(probe->inner.user_data, result);
}

static void release_snapshot_probe(void* user_data) {
  const snapshot_probe* probe = user_data;
  if (probe->inner.release_user_data != NULL) {
    probe->inner.release_user_data(probe->inner.user_data);
  }
}

typedef struct render_update_match {
  mln_map map;
  uint64_t generation;
} render_update_match;

static bool is_render_update_at(
  const mln_runtime_event* event, const char* messages, void* context
) {
  (void)messages;
  const render_update_match* match = context;
  return event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE &&
         event->source == match->map && event->generation == match->generation;
}

// A delta that pans and zooms publishes both in one snapshot and announces one
// render update for them.
static void a_delta_publishes_its_pan_and_zoom_together(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 256);
  mln_camera_options start = mln_camera_options_default();
  start.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  start.latitude = 10.0;
  start.longitude = 20.0;
  start.zoom = 4.0;
  jump(map, start);
  const mln_screen_point offset = {.x = 30.0, .y = -20.0};
  const mln_lat_lng moved_to_center = coordinate_at(
    map, (mln_screen_point){.x = 128.0 - offset.x, .y = 128.0 - offset.y}
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_test_drain_all(runtime);

  mln_test_completion completion = mln_test_completion_default(0);
  snapshot_probe probe = {.inner = completion.descriptor, .map = map};
  const mln_completion probed = {
    .size = sizeof(mln_completion),
    .callback = read_snapshot_then_complete,
    .user_data = &probe,
    .release_user_data = release_snapshot_probe,
  };
  mln_camera_delta delta = mln_camera_delta_default();
  delta.fields = MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE;
  delta.offset = offset;
  delta.scale = 4.0;
  MLN_TEST_OK(mln_map_apply_camera_delta(map, &delta, &probed, NULL));
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  const uint64_t generation = mln_test_completion_generation(&completion);
  mln_test_completion_destroy(&completion);

  MLN_TEST_OK(probe.read_status);
  TEST_ASSERT_EQUAL_UINT64(generation, probe.snapshot.generation);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 6.0, probe.snapshot.camera.zoom);
  assert_same_coordinate(
    "published center", moved_to_center,
    (mln_lat_lng){
      .latitude = probe.snapshot.camera.latitude,
      .longitude = probe.snapshot.camera.longitude
    }
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  render_update_match match = {.map = map, .generation = generation};
  TEST_ASSERT_EQUAL_size_t(
    1, mln_test_drain_counting_matching(runtime, is_render_update_at, &match)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static bool is_render_update_of(
  const mln_runtime_event* event, const char* messages, void* context
) {
  (void)messages;
  return event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE &&
         event->source == *(const mln_map*)context;
}

// A two-finger gesture passes its centroid movement as offset and its new
// centroid as anchor. The pan moves the coordinate at anchor - offset to the
// anchor, and the zoom and turn keep it there, all in one render update.
static void an_immediate_anchored_pan_keeps_the_dragged_coordinate(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 256);
  mln_camera_options start = mln_camera_options_default();
  start.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  start.latitude = 10.0;
  start.longitude = 20.0;
  start.zoom = 4.0;
  jump(map, start);
  const mln_screen_point offset = {.x = 30.0, .y = -20.0};
  const mln_screen_point anchor = {.x = 64.0, .y = 192.0};
  const mln_lat_lng dragged = coordinate_at(
    map, (mln_screen_point){.x = anchor.x - offset.x, .y = anchor.y - offset.y}
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_test_drain_all(runtime);

  mln_camera_delta delta = mln_camera_delta_default();
  delta.fields = MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE |
                 MLN_CAMERA_DELTA_BEARING | MLN_CAMERA_DELTA_ANCHOR;
  delta.offset = offset;
  delta.scale = 2.0;
  delta.bearing = 20.0;
  delta.anchor = anchor;
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_map_apply_camera_delta(map, &delta, &completion.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  const uint64_t generation = mln_test_completion_generation(&completion);
  mln_test_completion_destroy(&completion);

  mln_map_projection projection = create_projection(map);
  const mln_screen_point kept = projected_pixel(projection, dragged);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, anchor.x, kept.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, anchor.y, kept.y);
  mln_camera_options camera = mln_camera_options_default();
  MLN_TEST_OK(mln_map_projection_get_camera(projection, &camera, NULL));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 5.0, camera.zoom);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 20.0, camera.bearing);
  MLN_TEST_OK(mln_map_projection_close(projection, NULL));

  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_snapshot_get(map, &snapshot, NULL));
  TEST_ASSERT_EQUAL_UINT64(
    generation, snapshot.latest_render_update_generation
  );
  TEST_ASSERT_EQUAL_size_t(
    1, mln_test_drain_counting_matching(runtime, is_render_update_of, &map)
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
  ((mln_camera_delta*)descriptor)->fields = MLN_CAMERA_DELTA_ANCHOR << 1U;
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
static void delta_negative_scale(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_SCALE;
  delta->scale = -2.0;
}
static void delta_nan_scale(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_SCALE;
  delta->scale = NAN;
}
static void delta_infinite_bearing(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_BEARING;
  delta->bearing = INFINITY;
}
static void delta_nan_pitch(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_PITCH;
  delta->pitch = NAN;
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
static void delta_nan_anchor(void* descriptor) {
  mln_camera_delta* delta = descriptor;
  delta->fields = MLN_CAMERA_DELTA_BEARING | MLN_CAMERA_DELTA_ANCHOR;
  delta->bearing = 5.0;
  delta->anchor.x = NAN;
}
static void delta_undersized_animation(void* descriptor) {
  ((mln_camera_delta*)descriptor)->animation.size -= 1;
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
  {"negative scale", delta_negative_scale, MLN_STATUS_INVALID_ARGUMENT,
   "finite and positive"},
  {"NaN scale", delta_nan_scale, MLN_STATUS_INVALID_ARGUMENT,
   "finite and positive"},
  {"infinite bearing", delta_infinite_bearing, MLN_STATUS_INVALID_ARGUMENT,
   "bearing and pitch must be finite"},
  {"NaN pitch", delta_nan_pitch, MLN_STATUS_INVALID_ARGUMENT,
   "bearing and pitch must be finite"},
  {"anchor alone", delta_anchor_alone, MLN_STATUS_INVALID_ARGUMENT,
   "requires scale, bearing, or pitch"},
  {"animated anchored pan", delta_animated_anchored_pan,
   MLN_STATUS_INVALID_ARGUMENT, "only in an immediate delta"},
  {"NaN anchor", delta_nan_anchor, MLN_STATUS_INVALID_ARGUMENT,
   "must be finite"},
  {"undersized animation", delta_undersized_animation,
   MLN_STATUS_INVALID_ARGUMENT, "size is too small"},
};

static mln_camera_options finish_camera_query(mln_test_completion* query) {
  mln_camera_options camera = mln_camera_options_default();
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
  mln_map map, const mln_camera_options* camera, bool unwrapped
) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_lat_lng_bounds));
  MLN_TEST_OK(
    unwrapped
      ? mln_map_lat_lng_bounds_for_camera_unwrapped(
          map, camera, &query.descriptor, NULL
        )
      : mln_map_lat_lng_bounds_for_camera(map, camera, &query.descriptor, NULL)
  );
  mln_lat_lng_bounds bounds = {0};
  MLN_TEST_OK(
    mln_test_completion_finish_value(&query, &bounds, sizeof(bounds))
  );
  return bounds;
}

static bool near(double expected, double actual) {
  return fabs(expected - actual) < 1e-6;
}

// A camera fitted to bounds shows exactly those bounds: the visible box
// contains them and meets them on the side that limited the fit. The three fit
// inputs agree on the same region, and the fit options reach the result.
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
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, -122.5, fitted.longitude);

  const mln_lat_lng_bounds visible = bounds_for_camera(map, &fitted, false);
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
  const bool meets_east_and_west =
    near(region.southwest.longitude, visible.southwest.longitude) &&
    near(region.northeast.longitude, visible.northeast.longitude);
  const bool meets_north_and_south =
    near(region.southwest.latitude, visible.southwest.latitude) &&
    near(region.northeast.latitude, visible.northeast.latitude);
  TEST_ASSERT_TRUE_MESSAGE(
    meets_east_and_west || meets_north_and_south,
    "the fitted camera shows more than the region on every side"
  );

  // With no bearing or pitch, the unwrapped hull is the same box.
  const mln_lat_lng_bounds hull = bounds_for_camera(map, &fitted, true);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, visible.southwest.latitude, hull.southwest.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, visible.southwest.longitude, hull.southwest.longitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, visible.northeast.latitude, hull.northeast.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, visible.northeast.longitude, hull.northeast.longitude
  );

  const mln_lat_lng corners[] = {region.southwest, region.northeast};
  mln_test_completion from_coordinates =
    mln_test_completion_default(sizeof(mln_camera_options));
  MLN_TEST_OK(mln_map_camera_for_lat_lngs(
    map, corners, 2, NULL, &from_coordinates.descriptor, NULL
  ));
  const mln_camera_options coordinate_fit =
    finish_camera_query(&from_coordinates);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, fitted.latitude, coordinate_fit.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, fitted.longitude, coordinate_fit.longitude);
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
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, fitted.latitude, geometry_fit.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, fitted.longitude, geometry_fit.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, fitted.zoom, geometry_fit.zoom);

  // Padding leaves less of the viewport for the region, so the fit zooms out.
  // The fitted bearing and pitch are the ones the options ask for.
  mln_camera_fit_options fit = mln_camera_fit_options_default();
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

  // A fit is a query: the map camera stays where it was.
  const mln_camera_options current = query_camera(map).camera;
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, current.zoom);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A viewport across the antimeridian reports a wrapped box that keeps its
// longitudes inside -180 to 180 and an unwrapped hull that extends past 180
// on the shortest path through the center.
static void visible_bounds_unwrap_across_the_antimeridian(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime, 512);
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.latitude = 0.0;
  // A center on the antimeridian itself unprojects to 180 or -180 depending
  // on floating-point rounding, which flips the hull to the other side, so the
  // center sits just west of it.
  camera.longitude = 170.0;
  camera.zoom = 1.0;

  // At zoom 1 the world is 1024 pixels wide, so 512 pixels span 180 degrees.
  const mln_lat_lng_bounds hull = bounds_for_camera(map, &camera, true);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 80.0, hull.southwest.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 260.0, hull.northeast.longitude);

  const mln_lat_lng_bounds wrapped = bounds_for_camera(map, &camera, false);
  TEST_ASSERT_TRUE(wrapped.southwest.longitude >= -180.0);
  TEST_ASSERT_TRUE(wrapped.northeast.longitude <= 180.0);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, hull.southwest.latitude, wrapped.southwest.latitude
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, hull.northeast.latitude, wrapped.northeast.latitude
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static double jumped_longitude(mln_map map, double longitude) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.latitude = 0.0;
  camera.longitude = longitude;
  camera.zoom = 2.0;
  jump(map, camera);
  return query_camera(map).camera.longitude;
}

static mln_bound_options read_bounds(mln_runtime runtime, mln_map map) {
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  MLN_TEST_OK(mln_map_snapshot_get(map, &snapshot, NULL));
  return snapshot.bounds;
}

static const mln_lat_lng san_francisco = {
  .latitude = 37.7749, .longitude = -122.4194
};

// Each zoom level halves the ground distance a pixel covers, and at one zoom a
// pixel covers less ground the farther it is from the equator, in proportion
// to the cosine of the latitude.
static void meters_per_pixel_halves_per_zoom_and_shrinks_toward_the_poles(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = 3.0;
  jump(map, camera);

  const double equator = meters_per_pixel(map, 0.0);
  const double mid = meters_per_pixel(map, 45.0);
  const double high = meters_per_pixel(map, -60.0);
  TEST_ASSERT_GREATER_THAN_DOUBLE(mid, equator);
  TEST_ASSERT_GREATER_THAN_DOUBLE(high, mid);
  TEST_ASSERT_DOUBLE_WITHIN(equator * 1e-9, equator * sqrt(0.5), mid);
  TEST_ASSERT_DOUBLE_WITHIN(equator * 1e-9, equator * 0.5, high);

  camera.zoom = 4.0;
  jump(map, camera);
  TEST_ASSERT_DOUBLE_WITHIN(mid * 1e-9, mid / 2.0, meters_per_pixel(map, 45.0));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Every camera entry point checks its arguments before it submits: updates,
// deltas, fits, constraints, free camera options, and conversions.
static void camera_calls_reject_what_they_cannot_express(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  {
    mln_completion rejected = mln_test_discard_completion();
    MLN_TEST_INVALID(mln_map_update_camera(map, NULL, &rejected, NULL));
    MLN_TEST_INVALID(mln_map_camera_query(map, NULL, NULL));

    const mln_camera_update defaults = mln_camera_update_default();
    mln_test_run_validation_table(
      update_cases, sizeof(update_cases) / sizeof(*update_cases), &defaults,
      sizeof(defaults), submit_update_row, &map
    );
  }
  {
    const mln_camera_delta defaults = mln_camera_delta_default();
    mln_test_run_validation_table(
      delta_cases, sizeof(delta_cases) / sizeof(*delta_cases), &defaults,
      sizeof(defaults), submit_delta_row, &map
    );
    mln_completion discard = mln_test_discard_completion();
    MLN_TEST_INVALID(mln_map_apply_camera_delta(map, NULL, &discard, NULL));
  }
  {
    mln_completion operation = mln_test_discard_completion();
    mln_camera_fit_options fit = mln_camera_fit_options_default();
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
    fit = mln_camera_fit_options_default();
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
    MLN_TEST_INVALID(
      mln_map_lat_lng_bounds_for_camera_unwrapped(map, NULL, &operation, NULL)
    );
    mln_camera_options undersized = mln_camera_options_default();
    undersized.size -= 1;
    MLN_TEST_INVALID(mln_map_lat_lng_bounds_for_camera_unwrapped(
      map, &undersized, &operation, NULL
    ));
  }
  {
    mln_bound_options options = mln_bound_options_default();
    options.size = sizeof(mln_bound_options) - 1;
    mln_completion command = mln_test_discard_completion();
    MLN_TEST_INVALID(mln_map_set_bounds(map, NULL, &command, NULL));
    MLN_TEST_INVALID(mln_map_set_bounds(map, &options, &command, NULL));
    options = mln_bound_options_default();
    options.fields = UINT32_C(1) << 31;
    MLN_TEST_INVALID(mln_map_set_bounds(map, &options, &command, NULL));
    options = mln_bound_options_default();
    options.fields = MLN_BOUND_OPTION_BOUNDS | MLN_BOUND_OPTION_UNBOUNDED;
    MLN_TEST_INVALID(mln_map_set_bounds(map, &options, &command, NULL));
    options = mln_bound_options_default();
    options.fields = MLN_BOUND_OPTION_MIN_ZOOM | MLN_BOUND_OPTION_MAX_ZOOM;
    options.min_zoom = 10.0;
    options.max_zoom = 5.0;
    MLN_TEST_INVALID(
      mln_map_set_bounds(map, &options, &command, MLN_TEST_DIAGNOSTIC)
    );
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "min_zoom"));
    options = mln_bound_options_default();
    options.fields = MLN_BOUND_OPTION_MIN_PITCH | MLN_BOUND_OPTION_MAX_PITCH;
    options.min_pitch = 40.0;
    options.max_pitch = 20.0;
    MLN_TEST_INVALID(
      mln_map_set_bounds(map, &options, &command, MLN_TEST_DIAGNOSTIC)
    );
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "min_pitch"));
  }
  {
    mln_free_camera_options options = mln_free_camera_options_default();
    options.size = sizeof(mln_free_camera_options) - 1;
    mln_completion completion = mln_test_discard_completion();
    MLN_TEST_INVALID(
      mln_map_set_free_camera_options(map, NULL, &completion, NULL)
    );
    MLN_TEST_INVALID(
      mln_map_set_free_camera_options(map, &options, &completion, NULL)
    );
    options = mln_free_camera_options_default();
    options.fields = UINT32_C(1) << 31;
    MLN_TEST_INVALID(
      mln_map_set_free_camera_options(map, &options, &completion, NULL)
    );
    options = mln_free_camera_options_default();
    options.fields = MLN_FREE_CAMERA_OPTION_POSITION;
    options.position.z = NAN;
    MLN_TEST_INVALID(
      mln_map_set_free_camera_options(map, &options, &completion, NULL)
    );
    options = mln_free_camera_options_default();
    options.fields = MLN_FREE_CAMERA_OPTION_ORIENTATION;
    options.orientation = (mln_quaternion){0};
    MLN_TEST_INVALID(mln_map_set_free_camera_options(
      map, &options, &completion, MLN_TEST_DIAGNOSTIC
    ));
    TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "zero length"));
  }
  {
    mln_completion operation = mln_test_discard_completion();
    MLN_TEST_INVALID(mln_map_pixel_for_lat_lng(map, san_francisco, NULL, NULL));
    MLN_TEST_INVALID(mln_map_pixel_for_lat_lng(
      map, (mln_lat_lng){.latitude = 91.0}, &operation, NULL
    ));
    MLN_TEST_INVALID(mln_map_lat_lng_for_pixel(
      map, (mln_screen_point){.x = 0.0, .y = 0.0}, NULL, NULL
    ));
    MLN_TEST_INVALID(mln_map_lat_lng_for_pixel(
      map, (mln_screen_point){.x = NAN, .y = 0.0}, &operation, NULL
    ));
    MLN_TEST_INVALID(mln_map_lat_lng_for_pixel_unwrapped(
      map, (mln_screen_point){.x = 0.0, .y = 0.0}, NULL, NULL
    ));
    MLN_TEST_INVALID(
      mln_map_pixels_for_lat_lngs(map, NULL, 1, &operation, NULL)
    );
    MLN_TEST_INVALID(
      mln_map_lat_lngs_for_pixels(map, NULL, 1, &operation, NULL)
    );
    MLN_TEST_INVALID(
      mln_map_lat_lngs_for_pixels_unwrapped(map, NULL, 1, &operation, NULL)
    );
    MLN_TEST_INVALID(
      mln_map_meters_per_pixel_at_latitude(map, 0.0, NULL, NULL)
    );
    MLN_TEST_INVALID(
      mln_map_meters_per_pixel_at_latitude(map, 91.0, &operation, NULL)
    );
    MLN_TEST_INVALID(
      mln_map_meters_per_pixel_at_latitude(map, NAN, &operation, NULL)
    );
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Bounds constrain later camera commands: zoom and pitch limits clamp a jump
// that asks for more, and world bounds stop a jump that unbounded wraps. The
// unbounded state is distinct from world bounds, which the
// southwest/northeast pair alone cannot express. A free camera orientation
// replaces the camera's pitch and bearing.
static void camera_constraints_and_free_camera_reach_later_commands(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  // The unbounded state is distinct from world bounds, which the
  // southwest/northeast pair alone cannot express.
  {
    mln_bound_options snapshot = read_bounds(runtime, map);
    TEST_ASSERT_TRUE(snapshot.fields & MLN_BOUND_OPTION_UNBOUNDED);
    TEST_ASSERT_FALSE(snapshot.fields & MLN_BOUND_OPTION_BOUNDS);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, -160.0, jumped_longitude(map, 200.0));

    mln_bound_options world = mln_bound_options_default();
    world.fields = MLN_BOUND_OPTION_BOUNDS;
    world.bounds.southwest.latitude = -90.0;
    world.bounds.southwest.longitude = -180.0;
    world.bounds.northeast.latitude = 90.0;
    world.bounds.northeast.longitude = 180.0;
    MLN_TEST_AWAIT_OK(
      mln_map_set_bounds(map, &world, &completion.descriptor, NULL)
    );

    snapshot = read_bounds(runtime, map);
    TEST_ASSERT_TRUE(snapshot.fields & MLN_BOUND_OPTION_BOUNDS);
    TEST_ASSERT_FALSE(snapshot.fields & MLN_BOUND_OPTION_UNBOUNDED);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 180.0, snapshot.bounds.northeast.longitude);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 180.0, jumped_longitude(map, 200.0));

    mln_bound_options unbounded = mln_bound_options_default();
    unbounded.fields = MLN_BOUND_OPTION_UNBOUNDED;
    MLN_TEST_AWAIT_OK(
      mln_map_set_bounds(map, &unbounded, &completion.descriptor, NULL)
    );

    snapshot = read_bounds(runtime, map);
    TEST_ASSERT_TRUE(snapshot.fields & MLN_BOUND_OPTION_UNBOUNDED);
    TEST_ASSERT_FALSE(snapshot.fields & MLN_BOUND_OPTION_BOUNDS);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, -160.0, jumped_longitude(map, 200.0));
  }
  // Zoom and pitch limits clamp a later jump that asks for more.
  {
    mln_bound_options limits = mln_bound_options_default();
    limits.fields = MLN_BOUND_OPTION_MIN_ZOOM | MLN_BOUND_OPTION_MAX_ZOOM |
                    MLN_BOUND_OPTION_MAX_PITCH;
    limits.min_zoom = 3.0;
    limits.max_zoom = 6.0;
    limits.max_pitch = 20.0;
    MLN_TEST_AWAIT_OK(
      mln_map_set_bounds(map, &limits, &completion.descriptor, NULL)
    );

    mln_camera_options camera = mln_camera_options_default();
    camera.fields = MLN_CAMERA_OPTION_ZOOM | MLN_CAMERA_OPTION_PITCH;
    camera.zoom = 12.0;
    camera.pitch = 50.0;
    jump(map, camera);
    mln_camera_options clamped = query_camera(map).camera;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 6.0, clamped.zoom);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 20.0, clamped.pitch);

    camera.zoom = 1.0;
    jump(map, camera);
    clamped = query_camera(map).camera;
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 3.0, clamped.zoom);
  }
  // A free camera orientation replaces the camera's pitch and bearing: the
  // identity quaternion looks straight down with north up.
  {
    mln_camera_options camera = mln_camera_options_default();
    camera.fields = MLN_CAMERA_OPTION_ZOOM | MLN_CAMERA_OPTION_PITCH |
                    MLN_CAMERA_OPTION_BEARING;
    camera.zoom = 4.0;
    camera.pitch = 30.0;
    camera.bearing = 20.0;
    jump(map, camera);

    mln_free_camera_options options = mln_free_camera_options_default();
    options.fields = MLN_FREE_CAMERA_OPTION_ORIENTATION;
    options.orientation =
      (mln_quaternion){.x = 0.0, .y = 0.0, .z = 0.0, .w = 1.0};
    MLN_TEST_AWAIT_OK(mln_map_set_free_camera_options(
      map, &options, &completion.descriptor, NULL
    ));
    const mln_camera_options applied = query_camera(map).camera;
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, applied.pitch);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, applied.bearing);
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(camera_calls_reject_what_they_cannot_express);
  RUN_TEST(camera_snapshot_command_copy_and_disposition_are_ordered);
  RUN_TEST(every_camera_field_round_trips_through_the_snapshot);
  RUN_TEST(queued_immediate_deltas_compose_exactly);
  RUN_TEST(every_camera_delta_field_follows_its_convention);
  RUN_TEST(an_offset_moves_the_content_with_the_pointer);
  RUN_TEST(one_delta_turns_and_tilts_about_its_anchor);
  RUN_TEST(a_delta_publishes_its_pan_and_zoom_together);
  RUN_TEST(an_immediate_anchored_pan_keeps_the_dragged_coordinate);
  RUN_TEST(camera_fits_round_trip_through_the_visible_bounds);
  RUN_TEST(visible_bounds_unwrap_across_the_antimeridian);
  RUN_TEST(camera_constraints_and_free_camera_reach_later_commands);
  RUN_TEST(meters_per_pixel_halves_per_zoom_and_shrinks_toward_the_poles);
}
