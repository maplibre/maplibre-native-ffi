// Raw C ABI coverage for standalone projection lifecycle, ordered reads,
// commands, input ownership, and any-thread handles.

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

static void close_projection(mln_map_projection projection) {
  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_close_start(projection, &operation)
  );
  wait_completed(operation);
  mln_operation_release(operation);
}

static mln_camera_options read_camera(mln_map_projection projection) {
  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_get_camera_start(projection, &operation)
  );
  wait_completed(operation);
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_get_camera_take_result(operation, &camera)
  );
  mln_operation_release(operation);
  return camera;
}

static void creation_and_close_have_synchronous_preflight(void) {
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
  mln_operation map_close = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_map_close_start(map, &map_close)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, map_close);
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

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_close_start(projection, NULL)
  );
  mln_operation close = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_close_start(projection, &close)
  );
  mln_camera_options camera = mln_camera_options_default();
  uint64_t command_id = 0;
  // Close may retire the handle before this thread reaches the next call.
  const mln_status closing_command_status =
    mln_map_projection_set_camera(projection, &camera, &command_id);
  TEST_ASSERT_TRUE(
    closing_command_status == MLN_STATUS_INVALID_STATE ||
    closing_command_status == MLN_STATUS_INVALID_ARGUMENT
  );
  wait_completed(close);
  mln_operation_release(close);

  close = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_close_start(projection, &close)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void commands_copy_inputs_and_preserve_runtime_order(void) {
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_runtime_set_event_mask(runtime, MLN_RUNTIME_EVENT_MASK_COMMAND_FINISHED)
  );
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = create_projection(map);

  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.latitude = 12.0;
  camera.longitude = 34.0;
  camera.zoom = 4.0;
  uint64_t first_id = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_set_camera(projection, &camera, &first_id)
  );
  camera.latitude = -70.0;
  camera.longitude = -150.0;
  camera.zoom = 1.0;

  camera.latitude = 20.0;
  camera.longitude = 40.0;
  camera.zoom = 6.0;
  uint64_t second_id = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_projection_set_camera(projection, &camera, &second_id)
  );
  camera.latitude = 70.0;
  camera.longitude = 150.0;
  camera.zoom = 2.0;
  TEST_ASSERT_GREATER_THAN_UINT64(first_id, second_id);

  const mln_camera_options result = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 20.0, result.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 40.0, result.longitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 6.0, result.zoom);

  mln_lat_lng coordinates[] = {
    {.latitude = 0.0, .longitude = -10.0},
    {.latitude = 0.0, .longitude = 10.0},
  };
  uint64_t third_id = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_set_visible_coordinates(
                     projection, coordinates, 2, (mln_edge_insets){0}, &third_id
                   )
  );
  coordinates[0] = (mln_lat_lng){.latitude = 70.0, .longitude = 150.0};
  coordinates[1] = (mln_lat_lng){.latitude = 75.0, .longitude = 160.0};
  TEST_ASSERT_GREATER_THAN_UINT64(second_id, third_id);
  const mln_camera_options fitted = read_camera(projection);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 0.0, fitted.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, 0.0, fitted.longitude);

  mln_test_event_batch batch = mln_test_event_batch_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_drain_events(runtime, 0, &batch)
  );
  bool saw_first = false;
  bool saw_second_after_first = false;
  bool saw_third_after_second = false;
  for (size_t index = 0; index < batch.event_count; index += 1) {
    const mln_runtime_event* event = &batch.events[index];
    if (
      event->type != MLN_RUNTIME_EVENT_COMMAND_FINISHED ||
      event->source_type != MLN_RUNTIME_EVENT_SOURCE_PROJECTION ||
      event->source != projection
    ) {
      continue;
    }
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, event->code);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_COMMAND_DISPOSITION_COMMITTED,
      event->payload.command_finished.disposition
    );
    if (event->payload.command_finished.command_id == first_id) {
      saw_first = true;
    }
    if (event->payload.command_finished.command_id == second_id && saw_first) {
      saw_second_after_first = true;
    }
    if (
      event->payload.command_finished.command_id == third_id &&
      saw_second_after_first
    ) {
      saw_third_after_second = true;
    }
  }
  TEST_ASSERT_TRUE(saw_first);
  TEST_ASSERT_TRUE(saw_second_after_first);
  TEST_ASSERT_TRUE(saw_third_after_second);

  close_projection(projection);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void typed_reads_preserve_results_after_failed_transfers(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_map_projection projection = create_projection(map);

  mln_operation camera_operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_projection_get_camera_start(projection, &camera_operation)
  );
  wait_completed(camera_operation);
  mln_screen_point wrong_type = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_pixel_for_lat_lng_take_result(
      camera_operation, &wrong_type
    )
  );
  mln_camera_options too_small = {.size = sizeof(mln_camera_options) - 1};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_projection_get_camera_take_result(camera_operation, &too_small)
  );
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_projection_get_camera_take_result(camera_operation, &camera)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_map_projection_get_camera_take_result(camera_operation, &camera)
  );
  mln_operation_release(camera_operation);

  const mln_lat_lng input = {.latitude = 0.0, .longitude = 0.0};
  mln_operation pixel_operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_pixel_for_lat_lng_start(
                     projection, input, &pixel_operation
                   )
  );
  wait_completed(pixel_operation);
  mln_screen_point pixel = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_projection_pixel_for_lat_lng_take_result(pixel_operation, &pixel)
  );
  mln_operation_release(pixel_operation);

  mln_operation coordinate_operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_lat_lng_for_pixel_start(
                     projection, pixel, &coordinate_operation
                   )
  );
  wait_completed(coordinate_operation);
  mln_lat_lng coordinate = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_projection_lat_lng_for_pixel_take_result(
                     coordinate_operation, &coordinate
                   )
  );
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, input.latitude, coordinate.latitude);
  TEST_ASSERT_DOUBLE_WITHIN(1e-7, input.longitude, coordinate.longitude);
  mln_operation_release(coordinate_operation);

  close_projection(projection);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct projection_thread_probe {
  mln_map_projection projection;
  mln_status command_status;
  mln_status read_start_status;
  mln_status read_wait_status;
  mln_status read_take_status;
  mln_status close_start_status;
  mln_status close_wait_status;
} projection_thread_probe;

static void projection_foreign_thread(void* argument) {
  projection_thread_probe* probe = argument;
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_ZOOM;
  camera.zoom = 3.0;
  uint64_t command_id = 0;
  probe->command_status =
    mln_map_projection_set_camera(probe->projection, &camera, &command_id);

  mln_operation read = MLN_HANDLE_NULL;
  probe->read_start_status =
    mln_map_projection_get_camera_start(probe->projection, &read);
  bool completed = false;
  probe->read_wait_status = mln_operation_wait(read, 10000, &completed);
  camera = mln_camera_options_default();
  probe->read_take_status =
    completed ? mln_map_projection_get_camera_take_result(read, &camera)
              : MLN_STATUS_INVALID_STATE;
  mln_operation_release(read);

  mln_operation close = MLN_HANDLE_NULL;
  probe->close_start_status =
    mln_map_projection_close_start(probe->projection, &close);
  completed = false;
  probe->close_wait_status = mln_operation_wait(close, 10000, &completed);
  if (!completed) {
    probe->close_wait_status = MLN_STATUS_INVALID_STATE;
  }
  mln_operation_release(close);
}

static void projection_handles_are_callable_from_foreign_threads(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  projection_thread_probe probe = {
    .projection = create_projection(map),
    .command_status = MLN_STATUS_INVALID_STATE,
    .read_start_status = MLN_STATUS_INVALID_STATE,
    .read_wait_status = MLN_STATUS_INVALID_STATE,
    .read_take_status = MLN_STATUS_INVALID_STATE,
    .close_start_status = MLN_STATUS_INVALID_STATE,
    .close_wait_status = MLN_STATUS_INVALID_STATE,
  };
  mln_test_thread* thread =
    mln_test_thread_start(projection_foreign_thread, &probe);
  mln_test_thread_join(thread);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.command_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.read_start_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.read_wait_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.read_take_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.close_start_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.close_wait_status);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_projection_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(creation_and_close_have_synchronous_preflight);
  RUN_TEST(commands_copy_inputs_and_preserve_runtime_order);
  RUN_TEST(typed_reads_preserve_results_after_failed_transfers);
  RUN_TEST(projection_handles_are_callable_from_foreign_threads);
}
