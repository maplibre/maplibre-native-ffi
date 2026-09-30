// Coordinate-to-pixel conversions on a map: one coordinate, a batch, and an
// empty batch, each answered from the committed camera.

#include <stdbool.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static const mln_lat_lng san_francisco = {
  .latitude = 37.7749, .longitude = -122.4194
};
static const mln_lat_lng origin = {.latitude = 0.0, .longitude = 0.0};

static mln_map centered_map(mln_runtime runtime) {
  mln_map map = mln_test_create_map(runtime);
  mln_camera_update update = mln_camera_update_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = san_francisco.latitude;
  update.camera.longitude = san_francisco.longitude;
  update.camera.zoom = 3.0;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  return map;
}

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

static mln_lat_lng coordinate_at(mln_map map, mln_screen_point point) {
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

// The camera's center lands on the viewport's center, and a pixel converts
// back to the coordinate it came from.
static void a_coordinate_converts_to_a_pixel_and_back(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = centered_map(runtime);
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(map, &snapshot, NULL)
  );

  const mln_screen_point center = pixel_for(map, san_francisco);
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, snapshot.logical_extent.width / 2.0, center.x
  );
  TEST_ASSERT_DOUBLE_WITHIN(
    1e-6, snapshot.logical_extent.height / 2.0, center.y
  );
  const mln_screen_point off_center = pixel_for(map, origin);
  const mln_lat_lng round_trip = coordinate_at(map, off_center);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, origin.latitude, round_trip.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, origin.longitude, round_trip.longitude);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A batch answers each coordinate as the single conversion does, in order, and
// an empty batch answers with no points.
static void a_batch_converts_each_coordinate_in_order(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = centered_map(runtime);

  const mln_lat_lng coordinates[] = {san_francisco, origin};
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
  for (size_t index = 0; index < 2; index += 1) {
    const mln_screen_point single = pixel_for(map, coordinates[index]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, single.x, points[index].x);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, single.y, points[index].y);
  }

  mln_test_completion empty = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_pixels_for_lat_lngs(map, NULL, 0, &empty.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&empty));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_completion_value_count(&empty));
  mln_test_completion_destroy(&empty);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A projection's fit validates its coordinates before it moves the camera.
static void a_projection_fit_rejects_an_invalid_coordinate(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = centered_map(runtime);
  mln_test_completion created =
    mln_test_completion_default(sizeof(mln_map_projection));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_create(map, &created.descriptor, NULL)
  );
  mln_map_projection projection = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&created, &projection, sizeof(projection))
  );

  const mln_lat_lng coordinates[] = {
    origin, {.latitude = 91.0, .longitude = 0.0}
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_set_visible_coordinates(
      projection, coordinates, 2, (mln_edge_insets){0}, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_close(projection, NULL)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_coordinate_converts_to_a_pixel_and_back);
  RUN_TEST(a_batch_converts_each_coordinate_in_order);
  RUN_TEST(a_projection_fit_rejects_an_invalid_coordinate);
}
