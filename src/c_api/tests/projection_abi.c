// Raw C ABI coverage for standalone projection lifecycle, ordered creation,
// synchronous reads and setters, and any-thread handles.

#include <stdbool.h>
#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static void wait_completed(mln_operation operation) {
  bool completed = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_wait(operation, 10000, &completed)
  );
  TEST_ASSERT_TRUE(completed);
  mln_status terminal = MLN_STATUS_INVALID_STATE;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_get_status(operation, &terminal)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, terminal);
}

static mln_map_projection create_projection(mln_map map) {
  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_create_start(map, &operation)
  );
  wait_completed(operation);
  mln_map_projection projection = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_create_take_result(operation, &projection)
  );
  mln_operation_release(operation);
  return projection;
}

static mln_camera_options read_camera(mln_map_projection projection) {
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_get_camera(projection, &camera)
  );
  return camera;
}

static void creation_has_synchronous_preflight_and_close_retires_the_handle(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_projection_create_start(map, NULL)
  );
  mln_operation operation = UINT64_C(99);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_create_start(map, &operation)
  );

  operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_create_start(map, &operation)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_STATE, mln_map_release(map));
  wait_completed(operation);

  mln_map_projection projection = UINT64_C(77);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_create_take_result(operation, &projection)
  );
  projection = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_create_take_result(operation, &projection)
  );
  mln_operation_release(operation);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_projection_close(projection));

  // Every call with the retired handle reports an invalid argument.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_projection_close(projection)
  );
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_get_camera(projection, &camera)
  );
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = 2.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_set_camera(projection, &camera)
  );
  mln_screen_point point = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_pixel_for_lat_lng(
      projection, (mln_lat_lng){.latitude = 0.0, .longitude = 0.0}, &point
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void creation_observes_earlier_map_camera_commands(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  update.camera = mln_camera_options_default();
  update.camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  update.camera.latitude = 12.0;
  update.camera.longitude = 34.0;
  update.camera.zoom = 4.0;
  uint64_t command_id = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_update_camera(map, &update, &command_id)
  );

  // Creation is ordered after the accepted camera command, so the projection
  // copies the committed transform state.
  mln_map_projection projection = create_projection(map);
  const mln_camera_options camera = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 12.0, camera.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 34.0, camera.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 4.0, camera.zoom);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_projection_close(projection));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void setters_apply_before_return_and_conversions_round_trip(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = create_projection(map);

  mln_camera_options too_small = {.size = sizeof(mln_camera_options) - 1};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_get_camera(projection, &too_small)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_projection_get_camera(projection, NULL)
  );

  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.latitude = 20.0;
  camera.longitude = 40.0;
  camera.zoom = 6.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_set_camera(projection, &camera)
  );
  const mln_camera_options committed = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 20.0, committed.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 40.0, committed.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 6.0, committed.zoom);

  // A committed camera changes what later conversions observe: the new center
  // converts back to itself through a pixel round trip.
  mln_screen_point center_pixel = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_projection_pixel_for_lat_lng(
      projection, (mln_lat_lng){.latitude = 20.0, .longitude = 40.0},
      &center_pixel
    )
  );
  mln_lat_lng round_trip = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_projection_lat_lng_for_pixel(projection, center_pixel, &round_trip)
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 20.0, round_trip.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 40.0, round_trip.longitude);

  const mln_lat_lng origin = {.latitude = 0.0, .longitude = 0.0};
  mln_screen_point origin_before_fit = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_projection_pixel_for_lat_lng(projection, origin, &origin_before_fit)
  );

  const mln_lat_lng coordinates[] = {
    {.latitude = 0.0, .longitude = -10.0},
    {.latitude = 0.0, .longitude = 10.0},
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_set_visible_coordinates(
                     projection, coordinates, 2, (mln_edge_insets){0}
                   )
  );
  const mln_camera_options fitted = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 0.0, fitted.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 0.0, fitted.longitude);
  mln_screen_point origin_after_fit = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_projection_pixel_for_lat_lng(projection, origin, &origin_after_fit)
  );
  // The committed fit moved the camera, so the same coordinate lands on a
  // different pixel than it did before the fit.
  TEST_ASSERT_TRUE(origin_after_fit.x != origin_before_fit.x);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_projection_close(projection));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct projection_thread_probe {
  mln_map_projection projection;
  mln_status set_camera_status;
  mln_status get_camera_status;
  double observed_zoom;
  mln_status pixel_status;
  mln_status coordinate_status;
  double round_trip_latitude;
  mln_status close_status;
} projection_thread_probe;

static void projection_foreign_thread(void* argument) {
  projection_thread_probe* probe = argument;
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.latitude = 15.0;
  camera.longitude = 25.0;
  camera.zoom = 3.0;
  probe->set_camera_status =
    mln_map_projection_set_camera(probe->projection, &camera);

  camera = mln_camera_options_default();
  probe->get_camera_status =
    mln_map_projection_get_camera(probe->projection, &camera);
  probe->observed_zoom = camera.zoom;

  mln_screen_point pixel = {0};
  probe->pixel_status = mln_map_projection_pixel_for_lat_lng(
    probe->projection, (mln_lat_lng){.latitude = 15.0, .longitude = 25.0},
    &pixel
  );
  mln_lat_lng coordinate = {0};
  probe->coordinate_status =
    mln_map_projection_lat_lng_for_pixel(probe->projection, pixel, &coordinate);
  probe->round_trip_latitude = coordinate.latitude;

  probe->close_status = mln_map_projection_close(probe->projection);
}

static void projection_handles_are_callable_from_foreign_threads(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  projection_thread_probe probe = {
    .projection = create_projection(map),
    .set_camera_status = MLN_STATUS_INVALID_STATE,
    .get_camera_status = MLN_STATUS_INVALID_STATE,
    .observed_zoom = 0.0,
    .pixel_status = MLN_STATUS_INVALID_STATE,
    .coordinate_status = MLN_STATUS_INVALID_STATE,
    .round_trip_latitude = 0.0,
    .close_status = MLN_STATUS_INVALID_STATE,
  };
  mln_test_thread* thread =
    mln_test_thread_start(projection_foreign_thread, &probe);
  mln_test_thread_join(thread);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.set_camera_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.get_camera_status);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 3.0, probe.observed_zoom);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.pixel_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.coordinate_status);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 15.0, probe.round_trip_latitude);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.close_status);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_projection_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(creation_has_synchronous_preflight_and_close_retires_the_handle);
  RUN_TEST(creation_observes_earlier_map_camera_commands);
  RUN_TEST(setters_apply_before_return_and_conversions_round_trip);
  RUN_TEST(projection_handles_are_callable_from_foreign_threads);
}
