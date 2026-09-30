// Ordered conversions from coordinates to screen points, one at a time and in
// batches, the loaded style document, and a projection fit that rejects an
// invalid coordinate.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
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

// A coordinate's screen point converts back to the coordinate, and a batch
// converts each coordinate as a single conversion does.
static void coordinates_convert_to_screen_points(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_lat_lng coordinates[] = {
    {.latitude = 10.0, .longitude = 20.0},
    {.latitude = -5.0, .longitude = -30.0},
  };
  const mln_screen_point first = pixel_for(map, coordinates[0]);
  const mln_lat_lng back = coordinate_for(map, first);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, coordinates[0].latitude, back.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-6, coordinates[0].longitude, back.longitude);

  mln_test_completion batch =
    mln_test_completion_default(2 * sizeof(mln_screen_point));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_pixels_for_lat_lngs(map, coordinates, 2, &batch.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&batch));
  TEST_ASSERT_EQUAL_size_t(2, mln_test_completion_value_count(&batch));
  mln_screen_point points[2] = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&batch, points, sizeof(points))
  );
  mln_test_completion_destroy(&batch);
  const mln_screen_point second = pixel_for(map, coordinates[1]);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, first.x, points[0].x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, first.y, points[0].y);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, second.x, points[1].x);
  TEST_ASSERT_DOUBLE_WITHIN(1e-9, second.y, points[1].y);

  // An empty batch converts nothing and still completes.
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

// The loaded document is the style JSON the map parsed.
static void the_loaded_style_document_is_the_parsed_json(void) {
  static const char style[] =
    "{\"version\":8,\"name\":\"loaded\",\"sources\":{},\"layers\":[]}";
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, MLN_BUFFER_LITERAL(style));
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_loaded_style_json(map, &completion.descriptor, NULL)
  );
  char document[128];
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_style_finish_text(&completion, document, sizeof(document), NULL)
  );
  TEST_ASSERT_EQUAL_STRING(style, document);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A fit to visible coordinates rejects a latitude out of range.
static void a_projection_fit_rejects_an_invalid_coordinate(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_map_projection));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_create(map, &completion.descriptor, NULL)
  );
  mln_map_projection projection = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish_value(
                     &completion, &projection, sizeof(projection)
                   )
  );
  const mln_lat_lng coordinates[] = {
    {.latitude = 0.0, .longitude = 0.0},
    {.latitude = 91.0, .longitude = 0.0},
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_set_visible_coordinates(
      projection, coordinates, 2, (mln_edge_insets){0}, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "latitude"));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_close(projection, NULL)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(coordinates_convert_to_screen_points);
  RUN_TEST(the_loaded_style_document_is_the_parsed_json);
  RUN_TEST(a_projection_fit_rejects_an_invalid_coordinate);
}
