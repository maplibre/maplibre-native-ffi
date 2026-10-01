// The coordinates a projection fit refuses: a fit needs at least one point.

#include "support/test_support.h"

// A fit to no coordinates is refused, and leaves the projection's camera where
// it was.
static void a_fit_to_no_coordinates_is_refused(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_map_projection));
  MLN_TEST_OK(mln_map_projection_create(map, &completion.descriptor, NULL));
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_completion_finish_value(
    &completion, &projection, sizeof(projection)
  ));
  mln_camera_options before = mln_camera_options_default();
  MLN_TEST_OK(mln_map_projection_get_camera(projection, &before, NULL));

  const mln_lat_lng unused = {.latitude = 10.0, .longitude = 10.0};
  MLN_TEST_INVALID(mln_map_projection_set_visible_coordinates(
    projection, &unused, 0, (mln_edge_insets){0}, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "coordinate_count must be greater than 0"),
    mln_test_last_error()
  );
  mln_camera_options after = mln_camera_options_default();
  MLN_TEST_OK(mln_map_projection_get_camera(projection, &after, NULL));
  TEST_ASSERT_EQUAL_DOUBLE(before.latitude, after.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(before.longitude, after.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(before.zoom, after.zoom);

  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP { RUN_TEST(a_fit_to_no_coordinates_is_refused); }
