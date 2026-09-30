// Map reads and camera inputs whose success paths the camera and style files
// leave out: ordered pixel conversions, the loaded style document, global
// state, free-camera orientation, and the optional animation fields.

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static mln_map create_square_map(mln_runtime runtime) {
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = 512, .height = 512, .scale_factor = 1.0};
  return mln_test_create_map_with_options(runtime, &options);
}

static void jump(mln_map map, double latitude, double longitude, double zoom) {
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = latitude;
  update.camera.longitude = longitude;
  update.camera.zoom = zoom;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

static mln_screen_point pixel_of(mln_map map, mln_lat_lng coordinate) {
  mln_test_completion query =
    mln_test_completion_default(sizeof(mln_screen_point));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_pixel_for_lat_lng(map, coordinate, &query.descriptor, NULL)
  );
  mln_screen_point point = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&query, &point, sizeof(point))
  );
  return point;
}

static mln_lat_lng coordinate_of(mln_map map, mln_screen_point point) {
  mln_test_completion query = mln_test_completion_default(sizeof(mln_lat_lng));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_lat_lng_for_pixel(map, point, &query.descriptor, NULL)
  );
  mln_lat_lng coordinate = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&query, &coordinate, sizeof(coordinate))
  );
  return coordinate;
}

// The single and batched conversions agree with each other and invert through
// lat_lng_for_pixel, and the camera center lands at the map's center.
static void pixel_conversions_agree_and_invert(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime);
  jump(map, 0.0, 0.0, 2.0);

  const mln_lat_lng coordinates[] = {
    {.latitude = 0.0, .longitude = 0.0},
    {.latitude = 37.7749, .longitude = -122.4194},
  };
  const mln_screen_point center = pixel_of(map, coordinates[0]);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 256.0, center.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 256.0, center.y);
  const mln_screen_point city = pixel_of(map, coordinates[1]);
  const mln_lat_lng back = coordinate_of(map, city);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, coordinates[1].latitude, back.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, coordinates[1].longitude, back.longitude);

  mln_test_completion batch =
    mln_test_completion_default(2 * sizeof(mln_screen_point));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_pixels_for_lat_lngs(map, coordinates, 2, &batch.descriptor, NULL)
  );
  mln_screen_point points[2] = {0};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&batch));
  TEST_ASSERT_EQUAL_size_t(2, mln_test_completion_value_count(&batch));
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&batch, points, sizeof(points))
  );
  mln_test_completion_destroy(&batch);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, center.x, points[0].x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, center.y, points[0].y);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, city.x, points[1].x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, city.y, points[1].y);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_status read_text(
  mln_status (*query)(mln_map, const mln_completion*, mln_diagnostic*),
  mln_map map, char* out, size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, query(map, &completion.descriptor, NULL)
  );
  bool found = false;
  const mln_status status =
    mln_test_style_finish_text(&completion, out, capacity, &found);
  TEST_ASSERT_TRUE(found);
  return status;
}

static void the_loaded_style_document_reads_back(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"loaded-background\","
      "\"type\":\"background\"}]}"
    )
  );

  char document[512];
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    read_text(mln_map_loaded_style_json, map, document, sizeof(document))
  );
  TEST_ASSERT_NOT_NULL(strstr(document, "\"loaded-background\""));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void set_global_state(mln_map map, const char* value) {
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_global_state_property(
                     map, MLN_BUFFER_LITERAL("theme"), mln_test_view_of(value),
                     &completion.descriptor, NULL
                   )
  );
}

// Global state starts from the style's defaults, takes updates, returns to the
// default on null, and belongs to the style, so replacing the style drops it.
static void global_state_follows_its_style(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, "has not loaded",
    mln_map_set_global_state_property(
      map, MLN_BUFFER_LITERAL("theme"), MLN_BUFFER_LITERAL("true"),
      &completion.descriptor, NULL
    )
  );

  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sources\":{},\"layers\":[],"
      "\"state\":{\"theme\":{\"default\":\"light\"}}}"
    )
  );
  char state[256];
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    read_text(mln_map_get_global_state, map, state, sizeof(state))
  );
  TEST_ASSERT_EQUAL_STRING("{\"theme\":\"light\"}", state);

  set_global_state(map, "[\"dark\",{\"enabled\":true}]");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    read_text(mln_map_get_global_state, map, state, sizeof(state))
  );
  TEST_ASSERT_EQUAL_STRING("{\"theme\":[\"dark\",{\"enabled\":true}]}", state);

  set_global_state(map, "null");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    read_text(mln_map_get_global_state, map, state, sizeof(state))
  );
  TEST_ASSERT_EQUAL_STRING("{\"theme\":\"light\"}", state);

  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL("{\"version\":8,\"sources\":{},\"layers\":[]}")
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    read_text(mln_map_get_global_state, map, state, sizeof(state))
  );
  TEST_ASSERT_EQUAL_STRING("{}", state);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_map_snapshot read_snapshot(mln_map map) {
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(map, &snapshot, NULL)
  );
  return snapshot;
}

// A free camera with an orientation commits, and the snapshot then reports
// the pose that was set.
static void a_free_camera_orientation_round_trips(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime);
  jump(map, 10.0, 20.0, 4.0);
  const mln_free_camera_options pose = read_snapshot(map).free_camera;
  TEST_ASSERT_BITS_HIGH(
    MLN_FREE_CAMERA_OPTION_POSITION | MLN_FREE_CAMERA_OPTION_ORIENTATION,
    pose.fields
  );
  jump(map, 0.0, 0.0, 2.0);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_free_camera_options(map, &pose, &completion.descriptor, NULL)
  );

  const mln_free_camera_options restored = read_snapshot(map).free_camera;
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, pose.position.x, restored.position.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, pose.position.y, restored.position.y);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, pose.position.z, restored.position.z);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, pose.orientation.x, restored.orientation.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, pose.orientation.y, restored.orientation.y);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, pose.orientation.z, restored.orientation.z);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, pose.orientation.w, restored.orientation.w);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A zero-duration flight settles at once whatever its other animation fields
// say, so the committed camera shows the target.
static void a_flight_with_every_animation_field_commits(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_square_map(runtime);
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
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );

  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_map_get_camera(map, &camera));
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, 6.0, camera.zoom);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(pixel_conversions_agree_and_invert);
  RUN_TEST(the_loaded_style_document_reads_back);
  RUN_TEST(global_state_follows_its_style);
  RUN_TEST(a_free_camera_orientation_round_trips);
  RUN_TEST(a_flight_with_every_animation_field_commits);
}
