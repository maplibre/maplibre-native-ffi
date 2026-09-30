// Ordered conversions between coordinates and pixels, one at a time and in
// batches, answered from the committed camera. The invalid inputs these
// conversions reject are in camera.c.

#include <stddef.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static mln_screen_point pixel_for(mln_map map, mln_lat_lng coordinate) {
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

static mln_lat_lng coordinate_for(mln_map map, mln_screen_point point) {
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

// The camera's center lands on the middle of the map's extent and a point to
// the north east lands up and to the right of it. A batch converts each
// coordinate as the single call does, and both forms convert back to the
// coordinates they came from.
static void conversions_agree_singly_and_in_batches_and_invert(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 200;
  options.initial_extent.height = 100;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = 0.0;
  update.camera.longitude = 0.0;
  update.camera.zoom = 2.0;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );

  const mln_lat_lng coordinates[2] = {
    {.latitude = 0.0, .longitude = 0.0},
    {.latitude = 10.0, .longitude = 20.0},
  };
  const mln_screen_point center = pixel_for(map, coordinates[0]);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 100.0, center.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 50.0, center.y);
  const mln_screen_point north_east = pixel_for(map, coordinates[1]);
  TEST_ASSERT_GREATER_THAN_DOUBLE(center.x, north_east.x);
  TEST_ASSERT_LESS_THAN_DOUBLE(center.y, north_east.y);

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
  const mln_screen_point singles[2] = {center, north_east};
  for (size_t index = 0; index < 2; index += 1) {
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, singles[index].x, points[index].x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, singles[index].y, points[index].y);
    const mln_lat_lng back = coordinate_for(map, points[index]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, coordinates[index].latitude, back.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(
      1e-6, coordinates[index].longitude, back.longitude
    );
  }

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
  for (size_t index = 0; index < 2; index += 1) {
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

MLN_TEST_GROUP {
  RUN_TEST(conversions_agree_singly_and_in_batches_and_invert);
  RUN_TEST(empty_batches_convert_to_empty_results);
}
