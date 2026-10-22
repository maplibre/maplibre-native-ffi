#include "support/test_support.h"

static mln_screen_point pixel_for(
  mln_map_projection projection, mln_lat_lng coordinate
) {
  mln_screen_point point = {0};
  MLN_TEST_OK(
    mln_map_projection_pixel_for_lat_lng(projection, coordinate, &point, NULL)
  );
  return point;
}

static mln_lat_lng coordinate_for(
  mln_map_projection projection, mln_screen_point point
) {
  mln_lat_lng coordinate = {0};
  MLN_TEST_OK(
    mln_map_projection_lat_lng_for_pixel(projection, point, &coordinate, NULL)
  );
  return coordinate;
}

// The camera's center lands on the middle of the map's extent and a point to
// the north east lands up and to the right of it. A batch converts each
// coordinate as the single call does, in input order, and both forms convert
// back to the coordinates they came from.
static void conversions_agree_singly_and_in_batches_and_invert(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.initial_extent.width = 200;
  options.initial_extent.height = 100;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.center.latitude = 0.0;
  update.camera.center.longitude = 0.0;
  update.camera.zoom = 2.0;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );

  mln_map_projection projection = mln_test_map_create_projection(map);

  const mln_lat_lng coordinates[2] = {
    {.latitude = 0.0, .longitude = 0.0},
    {.latitude = 10.0, .longitude = 20.0},
  };
  const mln_screen_point center = pixel_for(projection, coordinates[0]);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 100.0, center.x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, 50.0, center.y);
  const mln_screen_point north_east = pixel_for(projection, coordinates[1]);
  TEST_ASSERT_GREATER_THAN_DOUBLE(center.x, north_east.x);
  TEST_ASSERT_LESS_THAN_DOUBLE(center.y, north_east.y);

  mln_screen_point points[2] = {0};
  MLN_TEST_OK(mln_map_projection_pixels_for_lat_lngs(
    projection, coordinates, 2, points, NULL
  ));
  const mln_screen_point singles[2] = {center, north_east};
  for (size_t index = 0; index < 2; index += 1) {
    TEST_ASSERT_EQUAL_DOUBLE(singles[index].x, points[index].x);
    TEST_ASSERT_EQUAL_DOUBLE(singles[index].y, points[index].y);
    const mln_lat_lng back = coordinate_for(projection, singles[index]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, coordinates[index].latitude, back.latitude);
    TEST_ASSERT_DOUBLE_WITHIN(
      1e-6, coordinates[index].longitude, back.longitude
    );
  }

  mln_lat_lng round_trip[2] = {0};
  MLN_TEST_OK(mln_map_projection_lat_lngs_for_pixels(
    projection, points, 2, round_trip, NULL
  ));
  for (size_t index = 0; index < 2; index += 1) {
    TEST_ASSERT_DOUBLE_WITHIN(
      1e-6, coordinates[index].latitude, round_trip[index].latitude
    );
    TEST_ASSERT_DOUBLE_WITHIN(
      1e-6, coordinates[index].longitude, round_trip[index].longitude
    );
  }

  MLN_TEST_OK(mln_map_projection_release(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An empty batch is valid in every direction, with null arrays, and converts
// to an empty result.
static void empty_batches_convert_to_empty_results(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = mln_test_map_create_projection(map);

  MLN_TEST_OK(
    mln_map_projection_pixels_for_lat_lngs(projection, NULL, 0, NULL, NULL)
  );
  MLN_TEST_OK(
    mln_map_projection_lat_lngs_for_pixels(projection, NULL, 0, NULL, NULL)
  );
  MLN_TEST_OK(mln_map_projection_lat_lngs_for_pixels_unwrapped(
    projection, NULL, 0, NULL, NULL
  ));

  MLN_TEST_OK(mln_map_projection_release(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(conversions_agree_singly_and_in_batches_and_invert);
  RUN_TEST(empty_batches_convert_to_empty_results);
}
