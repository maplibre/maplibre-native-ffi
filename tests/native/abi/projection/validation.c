// A standalone projection validates what its setters take, and a rejected
// call leaves the projection's camera as it was. The conversions' rejections
// are in projection.c.

#include <math.h>
#include <string.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static mln_map_projection create_projection(mln_map map) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_map_projection));
  MLN_TEST_OK(mln_map_projection_create(map, &completion.descriptor, NULL));
  mln_map_projection projection = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_test_completion_finish_value(
    &completion, &projection, sizeof(projection)
  ));
  return projection;
}

static double projection_zoom(mln_map_projection projection) {
  mln_camera_options camera = mln_camera_options_default();
  MLN_TEST_OK(mln_map_projection_get_camera(projection, &camera, NULL));
  return camera.zoom;
}

static void projection_setters_reject_invalid_values(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = create_projection(map);
  const double zoom = projection_zoom(projection);

  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = NAN;
  MLN_TEST_INVALID(
    mln_map_projection_set_camera(projection, &camera, MLN_TEST_DIAGNOSTIC)
  );

  const mln_lat_lng past_the_pole[2] = {
    {.latitude = 91.0, .longitude = 0.0},
    {.latitude = 0.0, .longitude = 10.0},
  };
  const mln_edge_insets no_padding = {0};
  MLN_TEST_INVALID(mln_map_projection_set_visible_coordinates(
    projection, past_the_pole, 2, no_padding, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "latitude"));
  const mln_lat_lng corners[2] = {
    {.latitude = -10.0, .longitude = -10.0},
    {.latitude = 10.0, .longitude = 10.0},
  };
  const mln_edge_insets negative_padding = {.top = -1.0};
  MLN_TEST_INVALID(mln_map_projection_set_visible_coordinates(
    projection, corners, 2, negative_padding, MLN_TEST_DIAGNOSTIC
  ));

  TEST_ASSERT_EQUAL_DOUBLE(zoom, projection_zoom(projection));

  MLN_TEST_OK(mln_map_projection_close(projection, NULL));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP { RUN_TEST(projection_setters_reject_invalid_values); }
