#include <string.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

#define VIEW(text) ((mln_buffer_view){.data = (text), .size = sizeof(text) - 1})

static bool source_exists(mln_runtime runtime, mln_map map, const char* id) {
  mln_operation operation = MLN_HANDLE_NULL;
  const mln_buffer_view view = {.data = id, .size = strlen(id)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info_start(map, view, &operation)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  mln_style_source_info info = {.size = sizeof(mln_style_source_info)};
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_source_info_take_result(operation, &info, &found)
  );
  mln_operation_release(operation);
  return found;
}

static const mln_runtime_event* event_at(
  const mln_runtime_event_batch_view* view, size_t index
) {
  return (const mln_runtime_event*)((const char*)view->events +
                                    (index * view->event_size));
}

static void style_command_deep_copies_and_ordered_read_observes_it(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  char id[] = "owned-source";
  char json[] =
    "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\","
    "\"features\":[]}}";
  uint64_t command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_style_source_json(
      map, (mln_buffer_view){.data = id, .size = strlen(id)},
      (mln_buffer_view){.data = json, .size = strlen(json)}, &command
    )
  );
  TEST_ASSERT_NOT_EQUAL(0, command);
  memset(id, 'x', strlen(id));
  memset(json, ' ', strlen(json));
  TEST_ASSERT_TRUE(source_exists(runtime, map, "owned-source"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void duplicate_id_is_an_async_failed_terminal_event(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view id = VIEW("duplicate");
  const mln_buffer_view json = VIEW(
    "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\","
    "\"features\":[]}}"
  );
  uint64_t first = 0;
  uint64_t duplicate = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_style_source_json(map, id, json, &first)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_style_source_json(map, id, json, &duplicate)
  );
  TEST_ASSERT_TRUE(duplicate > first);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  mln_event_batch batch = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, 0, &batch)
  );
  mln_runtime_event_batch_view view = {
    .size = sizeof(mln_runtime_event_batch_view)
  };
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_event_batch_get(batch, &view));
  bool saw_failure = false;
  for (size_t index = 0; index < view.event_count; ++index) {
    const mln_runtime_event* event = event_at(&view, index);
    if (
      event->type == MLN_RUNTIME_EVENT_COMMAND_FINISHED &&
      event->payload.command_finished.command_id == duplicate
    ) {
      saw_failure = true;
      TEST_ASSERT_EQUAL_UINT32(
        MLN_COMMAND_DISPOSITION_FAILED,
        event->payload.command_finished.disposition
      );
      TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_ARGUMENT, event->code);
      TEST_ASSERT_EQUAL_UINT64(0, event->payload.command_finished.generation);
    }
  }
  TEST_ASSERT_TRUE(saw_failure);
  mln_event_batch_release(batch);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void typed_take_transfers_and_discard_retains_no_result(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_list_style_source_ids_start(map, &operation)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  mln_style_id_list list = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_list_style_source_ids_take_result(operation, &list)
  );
  TEST_ASSERT_NOT_EQUAL(MLN_HANDLE_NULL, list);
  mln_style_id_list_destroy(list);
  mln_operation_release(operation);

  operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_list_style_source_ids_start(map, &operation)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_operation_discard_result(operation));
  list = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_map_list_style_source_ids_take_result(operation, &list)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, list);
  mln_operation_release(operation);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void wrong_typed_take_does_not_consume_result(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_list_style_source_ids_start(map, &operation)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  bool removed = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_remove_style_source_take_result(operation, &removed)
  );
  mln_style_id_list list = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_list_style_source_ids_take_result(operation, &list)
  );
  mln_style_id_list_destroy(list);
  mln_operation_release(operation);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void coordinate_size_probe_preserves_typed_result(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_lat_lng coordinates[4] = {
    {.latitude = 1.0, .longitude = 2.0},
    {.latitude = 1.0, .longitude = 3.0},
    {.latitude = 0.0, .longitude = 3.0},
    {.latitude = 0.0, .longitude = 2.0},
  };
  uint64_t command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_image_source_url(
                     map, VIEW("probe-image"), coordinates, 4,
                     VIEW("https://example.invalid/image.png"), &command
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));

  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_image_source_coordinates_start(
                     map, VIEW("probe-image"), &operation
                   )
  );
  bool completed = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_wait(operation, -1, &completed)
  );
  TEST_ASSERT_TRUE(completed);
  size_t count = 0;
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_image_source_coordinates_take_result(
                     operation, NULL, 0, &count, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(4, count);
  mln_lat_lng copied[4] = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_image_source_coordinates_take_result(
                     operation, copied, 4, &count, &found
                   )
  );
  TEST_ASSERT_EQUAL_MEMORY(coordinates, copied, sizeof(coordinates));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_map_get_image_source_coordinates_take_result(
                                operation, copied, 4, &count, &found
                              )
  );
  mln_operation_release(operation);

  operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_image_source_coordinates_start(
                     map, VIEW("probe-image"), &operation
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_image_source_coordinates_take_result(
      operation, NULL, 4, &count, &found
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_image_source_coordinates_take_result(
                     operation, copied, 4, &count, &found
                   )
  );
  mln_operation_release(operation);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void stretch_size_probe_preserves_typed_result(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const uint8_t pixel[4] = {0, 0, 0, 0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = sizeof(pixel);
  const mln_image_stretch stretch_x = {.from = 0.0f, .to = 1.0f};
  const mln_image_stretch stretch_y = {.from = 0.0f, .to = 1.0f};
  mln_style_image_options options = mln_style_image_options_default();
  options.fields =
    MLN_STYLE_IMAGE_OPTION_STRETCH_X | MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
  options.stretch_x = &stretch_x;
  options.stretch_x_count = 1;
  options.stretch_y = &stretch_y;
  options.stretch_y_count = 1;
  uint64_t command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, VIEW("probe-stretches"), &image, &options, &command
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));

  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_stretches_start(
                     map, VIEW("probe-stretches"), &operation
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  size_t x_count = 0;
  size_t y_count = 0;
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_stretches_take_result(
                     operation, NULL, 0, &x_count, NULL, 0, &y_count, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(1, x_count);
  TEST_ASSERT_EQUAL_size_t(1, y_count);
  mln_image_stretch copied_x = {0};
  mln_image_stretch copied_y = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_image_stretches_take_result(
      operation, &copied_x, 1, &x_count, &copied_y, 1, &y_count, &found
    )
  );
  TEST_ASSERT_EQUAL_MEMORY(&stretch_x, &copied_x, sizeof(stretch_x));
  TEST_ASSERT_EQUAL_MEMORY(&stretch_y, &copied_y, sizeof(stretch_y));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_map_copy_style_image_stretches_take_result(
      operation, &copied_x, 1, &x_count, &copied_y, 1, &y_count, &found
    )
  );
  mln_operation_release(operation);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_style_values_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(style_command_deep_copies_and_ordered_read_observes_it);
  RUN_TEST(duplicate_id_is_an_async_failed_terminal_event);
  RUN_TEST(typed_take_transfers_and_discard_retains_no_result);
  RUN_TEST(wrong_typed_take_does_not_consume_result);
  RUN_TEST(coordinate_size_probe_preserves_typed_result);
  RUN_TEST(stretch_size_probe_preserves_typed_result);
}
