#include <math.h>
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

static bool image_exists(mln_runtime runtime, mln_map map, const char* id) {
  mln_operation operation = MLN_HANDLE_NULL;
  const mln_buffer_view view = {.data = id, .size = strlen(id)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_image_info_start(map, view, &operation)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  mln_style_image_info info = {.size = sizeof(mln_style_image_info)};
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_image_info_take_result(operation, &info, &found)
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

// Drains the queue until it empties and asserts the COMMAND_FINISHED event for
// command_id carries the expected disposition, code, generation contract, and,
// when message_fragment is non-NULL, a diagnostic containing it.
static void expect_command_finished(
  mln_runtime runtime, uint64_t command_id, uint32_t disposition,
  mln_status code, const char* message_fragment
) {
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  bool found = false;
  for (;;) {
    mln_test_event_batch batch = mln_test_event_batch_default();
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_test_drain_events(runtime, &batch)
    );
    if (batch.event_count == 0) {
      break;
    }
    for (size_t index = 0; index < batch.event_count; index += 1) {
      const mln_runtime_event* event =
        (const mln_runtime_event*)((const char*)batch.events +
                                   (index * batch.event_size));
      if (
        event->type != MLN_RUNTIME_EVENT_COMMAND_FINISHED ||
        event->payload.command_finished.command_id != command_id
      ) {
        continue;
      }
      found = true;
      TEST_ASSERT_EQUAL_UINT32(
        disposition, event->payload.command_finished.disposition
      );
      TEST_ASSERT_EQUAL_INT(code, event->code);
      if (disposition == MLN_COMMAND_DISPOSITION_COMMITTED) {
        TEST_ASSERT_NOT_EQUAL(0, event->payload.command_finished.generation);
      } else {
        TEST_ASSERT_EQUAL_UINT64(0, event->payload.command_finished.generation);
      }
      if (message_fragment != NULL) {
        char message[256] = {0};
        size_t copied = event->message_size;
        if (copied > sizeof(message) - 1) {
          copied = sizeof(message) - 1;
        }
        memcpy(message, batch.messages + event->message_offset, copied);
        TEST_ASSERT_NOT_NULL(strstr(message, message_fragment));
      }
    }
  }
  TEST_ASSERT_TRUE(found);
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
    MLN_STATUS_OK, mln_runtime_drain_events(runtime, &batch)
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
  mln_style_source_info info = {.size = sizeof(mln_style_source_info)};
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_style_source_info_take_result(operation, &info, &found)
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

static void remove_commands_commit_and_report_missing_ids(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view json = VIEW(
    "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\","
    "\"features\":[]}}"
  );
  uint64_t command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_style_source_json(map, VIEW("doomed-source"), json, &command)
  );

  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_remove_style_source(map, VIEW("doomed-source"), &command)
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );
  TEST_ASSERT_FALSE(source_exists(runtime, map, "doomed-source"));

  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_remove_style_source(map, VIEW("doomed-source"), &command)
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_FAILED, MLN_STATUS_NOT_FOUND,
    "doomed-source"
  );

  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_remove_style_layer(map, VIEW("missing-layer"), &command)
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_FAILED, MLN_STATUS_NOT_FOUND,
    "missing-layer"
  );

  const uint8_t pixel[4] = {0, 0, 0, 0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = sizeof(pixel);
  mln_style_image_options options = mln_style_image_options_default();
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, VIEW("doomed-image"), &image, &options, &command
                   )
  );

  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_remove_style_image(map, VIEW("doomed-image"), &command)
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );
  TEST_ASSERT_FALSE(image_exists(runtime, map, "doomed-image"));

  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_remove_style_image(map, VIEW("doomed-image"), &command)
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_FAILED, MLN_STATUS_NOT_FOUND,
    "doomed-image"
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Takes one layer-info result for id after a runtime barrier, asserting the
// operation itself succeeds, and returns whether the layer was found.
static bool take_layer_info(
  mln_runtime runtime, mln_map map, const char* id, mln_style_layer_info* info
) {
  mln_operation operation = MLN_HANDLE_NULL;
  const mln_buffer_view view = {.data = id, .size = strlen(id)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_layer_info_start(map, view, &operation)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  memset(info, 0, sizeof(*info));
  info->size = sizeof(*info);
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_layer_info_take_result(operation, info, &found)
  );
  mln_operation_release(operation);
  return found;
}

static void layer_info_reports_scalars_and_sizes_the_source_id_copy(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_style_layer_info info = {.size = sizeof(mln_style_layer_info)};
  TEST_ASSERT_FALSE(take_layer_info(runtime, map, "missing-layer", &info));

  const mln_buffer_view json = VIEW(
    "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\","
    "\"features\":[]}}"
  );
  uint64_t command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_style_source_json(map, VIEW("info-source"), json, &command)
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_style_layer_json(
                     map,
                     VIEW(
                       "{\"id\":\"info-layer\",\"type\":\"circle\",\"source\":"
                       "\"info-source\"}"
                     ),
                     VIEW(""), &command
                   )
  );
  TEST_ASSERT_TRUE(take_layer_info(runtime, map, "info-layer", &info));
  TEST_ASSERT_EQUAL_size_t(strlen("circle"), info.type.size);
  TEST_ASSERT_EQUAL_MEMORY("circle", info.type.data, info.type.size);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_LAYER_VISIBILITY_VISIBLE, info.visibility);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_LAYER_INFO_SOURCE_ID, info.fields);
  TEST_ASSERT_EQUAL_size_t(0, info.source_layer_size);
  // Unbounded zooms report the documented infinities before bounds are set.
  TEST_ASSERT_EQUAL_DOUBLE(-INFINITY, info.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(INFINITY, info.max_zoom);

  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_layer_min_zoom(map, VIEW("info-layer"), 3.0, &command)
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_layer_max_zoom(map, VIEW("info-layer"), 12.0, &command)
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_layer_visibility(
      map, VIEW("info-layer"), MLN_STYLE_LAYER_VISIBILITY_NONE, &command
    )
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_layer_source_layer(
                     map, VIEW("info-layer"), VIEW("roads"), &command
                   )
  );
  TEST_ASSERT_TRUE(take_layer_info(runtime, map, "info-layer", &info));
  TEST_ASSERT_EQUAL_DOUBLE(3.0, info.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(12.0, info.max_zoom);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_LAYER_VISIBILITY_NONE, info.visibility);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_LAYER_INFO_SOURCE_ID | MLN_STYLE_LAYER_INFO_SOURCE_LAYER,
    info.fields
  );
  TEST_ASSERT_EQUAL_size_t(strlen("info-source"), info.source_id_size);
  TEST_ASSERT_EQUAL_size_t(strlen("roads"), info.source_layer_size);

  // The reported source ID length sizes the copy operation's result exactly.
  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_layer_source_id_start(map, VIEW("info-layer"), &operation)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  mln_buffer source_id = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_layer_source_id_take_result(operation, &source_id)
  );
  mln_buffer_view copied = {0};
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_buffer_get(source_id, &copied));
  TEST_ASSERT_EQUAL_size_t(info.source_id_size, copied.size);
  TEST_ASSERT_EQUAL_MEMORY("info-source", copied.data, copied.size);
  mln_buffer_destroy(source_id);
  mln_operation_release(operation);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void an_in_use_source_removal_fails_and_leaves_the_source(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view json = VIEW(
    "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\","
    "\"features\":[]}}"
  );
  uint64_t command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_style_source_json(map, VIEW("in-use"), json, &command)
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_style_layer_json(
      map, VIEW("{\"id\":\"user\",\"type\":\"circle\",\"source\":\"in-use\"}"),
      VIEW(""), &command
    )
  );

  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_remove_style_source(map, VIEW("in-use"), &command)
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_FAILED, MLN_STATUS_INVALID_STATE,
    "used by a layer"
  );
  TEST_ASSERT_TRUE(source_exists(runtime, map, "in-use"));

  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_remove_style_layer(map, VIEW("user"), &command)
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );

  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_remove_style_source(map, VIEW("in-use"), &command)
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );
  TEST_ASSERT_FALSE(source_exists(runtime, map, "in-use"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Prepared GeoJSON data creation is synchronous and touches no runtime or
// map, so validation reports through status and the thread diagnostic.
static void geojson_source_data_create_rejects_unsafe_raw_values(void) {
  static const char trailing_geojson[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}garbage";
  mln_geojson_source_data trailing_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(trailing_geojson), NULL, &trailing_data
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, trailing_data);

  static const char nul_geojson[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}\0garbage";
  mln_geojson_source_data nul_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      (mln_buffer_view){.data = nul_geojson, .size = sizeof(nul_geojson) - 1},
      NULL, &nul_data
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, nul_data);

  // A populated output handle is rejected rather than silently overwritten.
  static const char empty_collection[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}";
  mln_geojson_source_data populated = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(empty_collection), NULL, &populated
                   )
  );
  mln_geojson_source_data reused = populated;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(empty_collection), NULL, &reused
    )
  );
  mln_geojson_source_data_destroy(populated);

  // Unsafe raw options reject data preparation up front.
  mln_geojson_source_options short_size = mln_geojson_source_options_default();
  short_size.size = sizeof(uint32_t);
  mln_geojson_source_data short_size_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(empty_collection), &short_size, &short_size_data
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, short_size_data);
}

// Supercluster reads every feature geometry as a point, so data preparation
// rejects other geometry up front and names the feature and constraint.
static void clustered_geojson_data_reports_non_point_geometry(void) {
  static const char data[] =
    "{\"type\":\"FeatureCollection\",\"features\":["
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[-122.5,37.7]},\"properties\":{}},"
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"GeometryCollection\","
    "\"geometries\":[{\"type\":\"Point\",\"coordinates\":[-122.4,37.8]}]},"
    "\"properties\":{}}]}";

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;
  mln_geojson_source_data prepared = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(data), &clustered, &prepared
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, prepared);

  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "point geometry on every feature"));
  TEST_ASSERT_NOT_NULL(strstr(message, "feature 1"));
  TEST_ASSERT_NOT_NULL(strstr(message, "geometry collection"));

  // The constraint belongs to clustering alone, so the same data tiles fine
  // without it.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_geojson_source_data_create(MLN_BUFFER_LITERAL(data), NULL, &prepared)
  );
  mln_geojson_source_data_destroy(prepared);
}

static void clustered_geojson_data_requires_a_feature_collection(void) {
  static const char bare_geometry[] =
    "{\"type\":\"Point\",\"coordinates\":[-122.5,37.7]}";
  static const char single_feature[] =
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[-122.5,37.7]},\"properties\":{}}";

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;

  // MapLibre Native clusters feature collections only, so both of these would
  // tile unclustered rather than honouring the requested cluster option.
  mln_geojson_source_data prepared = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(bare_geometry), &clustered, &prepared
    )
  );
  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "requires a feature collection"));
  TEST_ASSERT_NOT_NULL(strstr(message, "a bare geometry"));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(single_feature), &clustered, &prepared
    )
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_thread_last_error_message(), "a single feature")
  );

  // The constraint belongs to clustering alone, so the same data tiles fine
  // without it.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(bare_geometry), NULL, &prepared
                   )
  );
  mln_geojson_source_data_destroy(prepared);
  prepared = MLN_HANDLE_NULL;

  // An empty feature collection carries nothing to cluster, so it stays
  // accepted and a later update supplies the features to cluster.
  static const char empty[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}";
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(empty), &clustered, &prepared
                   )
  );
  mln_geojson_source_data_destroy(prepared);
}

// Prepared data installs on any number of sources through commands; an update
// requires data whose baked-in options match the source's, and released
// handles reject installs while installed sources keep their own reference.
static void prepared_geojson_data_installs_and_checks_options(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  static const char points[] =
    "{\"type\":\"FeatureCollection\",\"features\":["
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[-122.5,37.7]},\"properties\":{}}]}";

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;

  mln_geojson_source_data plain_data = MLN_HANDLE_NULL;
  mln_geojson_source_data clustered_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(points), NULL, &plain_data
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(points), &clustered, &clustered_data
                   )
  );

  uint64_t command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, VIEW("plain"), plain_data, &command)
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_geojson_source_data(
                     map, VIEW("clustered"), clustered_data, &command
                   )
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );

  // Data prepared under different options tiles inconsistently with the
  // source, so the mismatch fails the command and names the source.
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_geojson_source_data(
                     map, VIEW("clustered"), plain_data, &command
                   )
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_FAILED,
    MLN_STATUS_INVALID_ARGUMENT, "do not match"
  );

  // One prepared handle installs on any number of matching sources.
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_geojson_source_data(
                     map, VIEW("clustered"), clustered_data, &command
                   )
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );

  // Cluster aggregations are part of the options match: a different
  // expression is rejected, while equivalent JSON with different formatting
  // compares equal by parsed expression.
  mln_geojson_source_options aggregated = clustered;
  aggregated.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  aggregated.cluster_properties =
    MLN_BUFFER_LITERAL("{\"total\":[\"+\",[\"get\",\"rank\"]]}");
  mln_geojson_source_data aggregated_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(points), &aggregated, &aggregated_data
                   )
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_geojson_source_data(
                     map, VIEW("aggregated"), aggregated_data, &command
                   )
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );
  mln_geojson_source_options reaggregated = aggregated;
  reaggregated.cluster_properties =
    MLN_BUFFER_LITERAL("{\"total\":[\"max\",[\"get\",\"rank\"]]}");
  mln_geojson_source_data reaggregated_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(points), &reaggregated, &reaggregated_data
    )
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_geojson_source_data(
                     map, VIEW("aggregated"), reaggregated_data, &command
                   )
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_FAILED,
    MLN_STATUS_INVALID_ARGUMENT, "do not match"
  );
  mln_geojson_source_data_destroy(reaggregated_data);
  mln_geojson_source_options reformatted = aggregated;
  reformatted.cluster_properties =
    MLN_BUFFER_LITERAL(" { \"total\" : [\"+\", [\"get\", \"rank\"]] } ");
  mln_geojson_source_data reformatted_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(points), &reformatted, &reformatted_data
                   )
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_geojson_source_data(
                     map, VIEW("aggregated"), reformatted_data, &command
                   )
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );
  mln_geojson_source_data_destroy(reformatted_data);
  mln_geojson_source_data_destroy(aggregated_data);

  // The submit-time lease keeps the prepared index alive, so the handle may
  // be destroyed as soon as the install command is submitted.
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_data(map, VIEW("plain"), plain_data, &command)
  );
  mln_geojson_source_data_destroy(plain_data);
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );

  // The runtime override takes a live GeoJSON source alone.
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_geojson_source_synchronous_tiling(
                     map, VIEW("plain"), true, &command
                   )
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_geojson_source_synchronous_tiling(
                     map, VIEW("plain"), false, &command
                   )
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_COMMITTED, MLN_STATUS_OK, NULL
  );
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_geojson_source_synchronous_tiling(
                     map, VIEW("missing"), true, &command
                   )
  );
  expect_command_finished(
    runtime, command, MLN_COMMAND_DISPOSITION_FAILED,
    MLN_STATUS_INVALID_ARGUMENT, "source does not exist"
  );

  // A released handle rejects new installs at submit, a second destroy is a
  // no-op, and installed sources keep their own reference.
  mln_geojson_source_data_destroy(plain_data);
  mln_geojson_source_data_destroy(MLN_HANDLE_NULL);
  command = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(map, VIEW("plain"), plain_data, &command)
  );
  TEST_ASSERT_TRUE(source_exists(runtime, map, "plain"));
  mln_geojson_source_data_destroy(clustered_data);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_style_values_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(style_command_deep_copies_and_ordered_read_observes_it);
  RUN_TEST(duplicate_id_is_an_async_failed_terminal_event);
  RUN_TEST(typed_take_transfers_and_discard_retains_no_result);
  RUN_TEST(wrong_typed_take_does_not_consume_result);
  RUN_TEST(remove_commands_commit_and_report_missing_ids);
  RUN_TEST(an_in_use_source_removal_fails_and_leaves_the_source);
  RUN_TEST(geojson_source_data_create_rejects_unsafe_raw_values);
  RUN_TEST(clustered_geojson_data_reports_non_point_geometry);
  RUN_TEST(clustered_geojson_data_requires_a_feature_collection);
  RUN_TEST(prepared_geojson_data_installs_and_checks_options);
  RUN_TEST(layer_info_reports_scalars_and_sizes_the_source_id_copy);
  RUN_TEST(coordinate_size_probe_preserves_typed_result);
  RUN_TEST(stretch_size_probe_preserves_typed_result);
}
