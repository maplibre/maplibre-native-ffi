// Map geometry conversions that succeed, single and batched, including the
// empty batch; the camera options that only a successful command converts; and
// the ordered copy of the parsed style document.

#include <math.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static void jump_to_origin(mln_map map) {
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = 0.0;
  update.camera.longitude = 0.0;
  update.camera.zoom = 2.0;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

// The map's center lands on the middle of its extent, a point to the north
// east lands up and to the right of it, and the batch converts back to the
// coordinates it came from.
static void a_map_converts_lat_lngs_to_pixels_singly_and_in_batches(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 200;
  options.initial_extent.height = 100;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  jump_to_origin(map);

  mln_test_completion single =
    mln_test_completion_default(sizeof(mln_screen_point));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_pixel_for_lat_lng(
                     map, (mln_lat_lng){.latitude = 0.0, .longitude = 0.0},
                     &single.descriptor, NULL
                   )
  );
  mln_screen_point center = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&single, &center, sizeof(center))
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 100.0, center.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 50.0, center.y);

  const mln_lat_lng coordinates[2] = {
    {.latitude = 0.0, .longitude = 0.0},
    {.latitude = 10.0, .longitude = 20.0},
  };
  mln_test_completion batch =
    mln_test_completion_default(2 * sizeof(mln_screen_point));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_pixels_for_lat_lngs(map, coordinates, 2, &batch.descriptor, NULL)
  );
  mln_screen_point points[2] = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&batch, points, sizeof(points))
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, center.x, points[0].x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, center.y, points[0].y);
  TEST_ASSERT_GREATER_THAN_DOUBLE(center.x, points[1].x);
  TEST_ASSERT_LESS_THAN_DOUBLE(center.y, points[1].y);

  mln_test_completion back =
    mln_test_completion_default(2 * sizeof(mln_lat_lng));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_lat_lngs_for_pixels(map, points, 2, &back.descriptor, NULL)
  );
  mln_lat_lng round_trip[2] = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&back, round_trip, sizeof(round_trip))
  );
  for (size_t index = 0; index < 2; ++index) {
    TEST_ASSERT_DOUBLE_WITHIN(
      1e-6, coordinates[index].latitude, round_trip[index].latitude
    );
    TEST_ASSERT_DOUBLE_WITHIN(
      1e-6, coordinates[index].longitude, round_trip[index].longitude
    );
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An empty batch is valid input in both directions and converts to an empty
// result.
static void empty_batches_convert_to_empty_results(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_test_completion pixels = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_pixels_for_lat_lngs(map, NULL, 0, &pixels.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&pixels));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_completion_value_count(&pixels));
  mln_test_completion_destroy(&pixels);

  mln_test_completion coordinates = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_lat_lngs_for_pixels(map, NULL, 0, &coordinates.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&coordinates)
  );
  TEST_ASSERT_EQUAL_size_t(0, mln_test_completion_value_count(&coordinates));
  mln_test_completion_destroy(&coordinates);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A creation descriptor whose extent has no area or no finite scale factor is
// rejected before any map exists.
static void map_creation_rejects_a_degenerate_extent(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 0;
  mln_map map = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_test_map_create_status(runtime, &options, &map)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "initial extent"));
  options = mln_map_options_default();
  options.initial_extent.scale_factor = INFINITY;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_test_map_create_status(runtime, &options, &map)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, map);
  mln_test_destroy_runtime(runtime);
}

// An ease that carries its own easing curve converts it with the rest of the
// animation, and a zero duration lands the camera on its target at commit.
static void an_ease_with_a_custom_easing_lands_on_its_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = 6.0;
  update.animation.fields =
    MLN_ANIMATION_OPTION_DURATION | MLN_ANIMATION_OPTION_EASING;
  update.animation.duration_ms = 0.0;
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

// A free camera orientation is a quaternion the map applies with the
// position; an identity orientation looks straight down, so the pitch stays
// zero.
static void a_free_camera_orientation_commits_with_its_position(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_free_camera_options options = mln_free_camera_options_default();
  options.fields =
    MLN_FREE_CAMERA_OPTION_POSITION | MLN_FREE_CAMERA_OPTION_ORIENTATION;
  options.position = (mln_vec3){.x = 0.5, .y = 0.5, .z = 0.25};
  options.orientation =
    (mln_quaternion){.x = 0.0, .y = 0.0, .z = 0.0, .w = 1.0};
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_free_camera_options(map, &options, &completion.descriptor, NULL)
  );
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_map_get_camera(map, &camera));
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, camera.pitch);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The parsed style document reads back byte for byte, so a host can reload it
// unchanged.
static void the_loaded_style_document_reads_back_unchanged(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_test_completion copy = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_loaded_style_json(map, &copy.descriptor, NULL)
  );
  char document[1024] = {0};
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_style_finish_text(&copy, document, sizeof(document), &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(
    mln_test_background_style_json.size, strlen(document)
  );
  TEST_ASSERT_EQUAL_MEMORY(
    mln_test_background_style_json.data, document,
    mln_test_background_style_json.size
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_map_converts_lat_lngs_to_pixels_singly_and_in_batches);
  RUN_TEST(empty_batches_convert_to_empty_results);
  RUN_TEST(map_creation_rejects_a_degenerate_extent);
  RUN_TEST(an_ease_with_a_custom_easing_lands_on_its_target);
  RUN_TEST(a_free_camera_orientation_commits_with_its_position);
  RUN_TEST(the_loaded_style_document_reads_back_unchanged);
}
