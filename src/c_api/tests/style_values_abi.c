#include <math.h>
#include <string.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

#define VIEW(text) ((mln_buffer_view){.data = (text), .size = sizeof(text) - 1})

static bool source_exists(mln_runtime runtime, mln_map map, const char* id) {
  (void)runtime;
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_source_info));
  const mln_buffer_view view = {.data = id, .size = strlen(id)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_source_info(map, view, &completion.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  const bool found = mln_test_completion_value_count(&completion) == 1;
  mln_test_completion_destroy(&completion);
  return found;
}

static bool image_exists(mln_runtime runtime, mln_map map, const char* id) {
  (void)runtime;
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_image_info));
  const mln_buffer_view view = {.data = id, .size = strlen(id)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_image_info(map, view, &completion.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  const bool found = mln_test_completion_value_count(&completion) == 1;
  mln_test_completion_destroy(&completion);
  return found;
}

#define EXPECT_STYLE_COMMAND(terminal_status, expression)            \
  do {                                                               \
    mln_test_completion completion = mln_test_completion_default(0); \
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, (expression));              \
    TEST_ASSERT_EQUAL_INT(                                           \
      (terminal_status), mln_test_completion_finish(&completion)     \
    );                                                               \
    mln_test_completion_destroy(&completion);                        \
  } while (false)

#define EXPECT_STYLE_COMMAND_FAILED(terminal_status, fragment, expression) \
  do {                                                                     \
    mln_test_completion completion = mln_test_completion_default(0);       \
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, (expression));                    \
    TEST_ASSERT_EQUAL_INT(                                                 \
      (terminal_status), mln_test_completion_finish(&completion)           \
    );                                                                     \
    TEST_ASSERT_EQUAL_UINT32(                                              \
      MLN_COMMAND_DISPOSITION_FAILED,                                      \
      mln_test_completion_disposition(&completion)                         \
    );                                                                     \
    TEST_ASSERT_NOT_NULL(                                                  \
      strstr(mln_test_completion_diagnostic(&completion), (fragment))      \
    );                                                                     \
    mln_test_completion_destroy(&completion);                              \
  } while (false)

static void style_command_deep_copies_and_ordered_read_observes_it(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  char id[] = "owned-source";
  char json[] =
    "{\"type\":\"geojson\",\"data\":{\"type\":\"FeatureCollection\","
    "\"features\":[]}}";
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_source_json(
                     map, (mln_buffer_view){.data = id, .size = strlen(id)},
                     (mln_buffer_view){.data = json, .size = strlen(json)},
                     &completion.descriptor
                   )
  );
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
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_style_source_json(map, id, json, &completion.descriptor)
  );
  mln_test_completion duplicate = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_style_source_json(map, id, json, &duplicate.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_completion_finish(&duplicate)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_FAILED, mln_test_completion_disposition(&duplicate)
  );
  mln_test_completion_destroy(&duplicate);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void image_source_coordinates_are_borrowed_by_the_completion(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_lat_lng coordinates[4] = {
    {.latitude = 1.0, .longitude = 2.0},
    {.latitude = 1.0, .longitude = 3.0},
    {.latitude = 0.0, .longitude = 3.0},
    {.latitude = 0.0, .longitude = 2.0},
  };
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_image_source_url(
      map, VIEW("probe-image"), coordinates, 4,
      VIEW("https://example.invalid/image.png"), &completion.descriptor
    )
  );

  mln_test_completion completion =
    mln_test_completion_default(sizeof(coordinates));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_image_source_coordinates(
                     map, VIEW("probe-image"), &completion.descriptor
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  TEST_ASSERT_EQUAL_size_t(4, mln_test_completion_value_count(&completion));
  mln_lat_lng copied[4] = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&completion, copied, sizeof(copied))
  );
  TEST_ASSERT_EQUAL_MEMORY(coordinates, copied, sizeof(coordinates));
  mln_test_completion_destroy(&completion);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct stretch_probe {
  atomic_bool done;
  mln_status status;
  mln_image_stretch x;
  mln_image_stretch y;
  size_t x_count;
  size_t y_count;
} stretch_probe;

static void copy_stretches(
  void* user_data, const mln_completion_result* result
) {
  stretch_probe* probe = user_data;
  probe->status = result->status;
  if (result->value_count == 1) {
    const mln_style_image_stretches_result* stretches = result->value;
    probe->x_count = stretches->stretch_x_count;
    probe->y_count = stretches->stretch_y_count;
    if (probe->x_count != 0) probe->x = stretches->stretch_x[0];
    if (probe->y_count != 0) probe->y = stretches->stretch_y[0];
  }
  atomic_store(&probe->done, true);
}

static void style_image_stretches_are_borrowed_by_the_completion(void) {
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
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_style_image(
      map, VIEW("probe-stretches"), &image, &options, &completion.descriptor
    )
  );

  stretch_probe probe = {.status = MLN_STATUS_INVALID_STATE};
  atomic_init(&probe.done, false);
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = copy_stretches,
    .user_data = &probe,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_stretches(
                     map, VIEW("probe-stretches"), &completion
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
  TEST_ASSERT_TRUE(atomic_load(&probe.done));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.status);
  TEST_ASSERT_EQUAL_size_t(1, probe.x_count);
  TEST_ASSERT_EQUAL_size_t(1, probe.y_count);
  TEST_ASSERT_EQUAL_MEMORY(&stretch_x, &probe.x, sizeof(stretch_x));
  TEST_ASSERT_EQUAL_MEMORY(&stretch_y, &probe.y, sizeof(stretch_y));
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
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_source_json(
                     map, VIEW("doomed-source"), json, &completion.descriptor
                   )
  );

  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_remove_style_source(
                     map, VIEW("doomed-source"), &completion.descriptor
                   )
  );
  TEST_ASSERT_FALSE(source_exists(runtime, map, "doomed-source"));

  mln_test_completion missing = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_remove_style_source(map, VIEW("doomed-source"), &missing.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND, mln_test_completion_finish(&missing)
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_test_completion_diagnostic(&missing), "doomed-source")
  );
  mln_test_completion_destroy(&missing);

  missing = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_remove_style_layer(map, VIEW("missing-layer"), &missing.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND, mln_test_completion_finish(&missing)
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_test_completion_diagnostic(&missing), "missing-layer")
  );
  mln_test_completion_destroy(&missing);

  const uint8_t pixel[4] = {0, 0, 0, 0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = sizeof(pixel);
  mln_style_image_options options = mln_style_image_options_default();
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_style_image(
      map, VIEW("doomed-image"), &image, &options, &completion.descriptor
    )
  );

  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_remove_style_image(
                     map, VIEW("doomed-image"), &completion.descriptor
                   )
  );
  TEST_ASSERT_FALSE(image_exists(runtime, map, "doomed-image"));

  missing = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_remove_style_image(map, VIEW("doomed-image"), &missing.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND, mln_test_completion_finish(&missing)
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_test_completion_diagnostic(&missing), "doomed-image")
  );
  mln_test_completion_destroy(&missing);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Copies one layer-info result from its completion.
static bool take_layer_info(
  mln_runtime runtime, mln_map map, const char* id, mln_style_layer_info* info
) {
  (void)runtime;
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_layer_result));
  const mln_buffer_view view = {.data = id, .size = strlen(id)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_layer_info(map, view, &completion.descriptor)
  );
  memset(info, 0, sizeof(*info));
  info->size = sizeof(*info);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  const bool found = mln_test_completion_value_count(&completion) == 1;
  if (found) {
    mln_style_layer_result result = {0};
    TEST_ASSERT_TRUE(
      mln_test_completion_copy_value(&completion, &result, sizeof(result))
    );
    *info = result.info;
  }
  mln_test_completion_destroy(&completion);
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
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_source_json(
                     map, VIEW("info-source"), json, &completion.descriptor
                   )
  );
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_layer_json(
                     map,
                     VIEW(
                       "{\"id\":\"info-layer\",\"type\":\"circle\",\"source\":"
                       "\"info-source\"}"
                     ),
                     VIEW(""), &completion.descriptor
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

  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_set_layer_min_zoom(
                     map, VIEW("info-layer"), 3.0, &completion.descriptor
                   )
  );
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_set_layer_max_zoom(
                     map, VIEW("info-layer"), 12.0, &completion.descriptor
                   )
  );
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_set_layer_visibility(
                     map, VIEW("info-layer"), MLN_STYLE_LAYER_VISIBILITY_NONE,
                     &completion.descriptor
                   )
  );
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_layer_source_layer(
      map, VIEW("info-layer"), VIEW("roads"), &completion.descriptor
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
  mln_test_completion source_completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_layer_source_id(
                     map, VIEW("info-layer"), &source_completion.descriptor
                   )
  );
  mln_buffer_view copied = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_completion_finish(&source_completion)
  );
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&source_completion, &copied, sizeof(copied))
  );
  TEST_ASSERT_EQUAL_size_t(info.source_id_size, copied.size);
  TEST_ASSERT_EQUAL_MEMORY("info-source", copied.data, copied.size);
  mln_test_completion_destroy(&source_completion);
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
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_source_json(
                     map, VIEW("in-use"), json, &completion.descriptor
                   )
  );
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_style_layer_json(
      map, VIEW("{\"id\":\"user\",\"type\":\"circle\",\"source\":\"in-use\"}"),
      VIEW(""), &completion.descriptor
    )
  );

  EXPECT_STYLE_COMMAND_FAILED(
    MLN_STATUS_INVALID_STATE, "used by a layer",
    mln_map_remove_style_source(map, VIEW("in-use"), &completion.descriptor)
  );
  TEST_ASSERT_TRUE(source_exists(runtime, map, "in-use"));

  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_remove_style_layer(map, VIEW("user"), &completion.descriptor)
  );

  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_remove_style_source(map, VIEW("in-use"), &completion.descriptor)
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

  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_add_geojson_source_data(
                     map, VIEW("plain"), plain_data, &completion.descriptor
                   )
  );
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(
      map, VIEW("clustered"), clustered_data, &completion.descriptor
    )
  );

  // Data prepared under different options tiles inconsistently with the
  // source, so the mismatch fails the command and names the source.
  EXPECT_STYLE_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "do not match",
    mln_map_set_geojson_source_data(
      map, VIEW("clustered"), plain_data, &completion.descriptor
    )
  );

  // One prepared handle installs on any number of matching sources.
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_data(
      map, VIEW("clustered"), clustered_data, &completion.descriptor
    )
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
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(
      map, VIEW("aggregated"), aggregated_data, &completion.descriptor
    )
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
  EXPECT_STYLE_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "do not match",
    mln_map_set_geojson_source_data(
      map, VIEW("aggregated"), reaggregated_data, &completion.descriptor
    )
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
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_data(
      map, VIEW("aggregated"), reformatted_data, &completion.descriptor
    )
  );
  mln_geojson_source_data_destroy(reformatted_data);
  mln_geojson_source_data_destroy(aggregated_data);

  // The submit-time lease keeps the prepared index alive, so the handle may
  // be destroyed as soon as the install command is submitted.
  mln_test_completion install = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_geojson_source_data(
                     map, VIEW("plain"), plain_data, &install.descriptor
                   )
  );
  mln_geojson_source_data_destroy(plain_data);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&install));
  mln_test_completion_destroy(&install);

  // The runtime override takes a live GeoJSON source alone.
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_set_geojson_source_synchronous_tiling(
                     map, VIEW("plain"), true, &completion.descriptor
                   )
  );
  EXPECT_STYLE_COMMAND(
    MLN_STATUS_OK, mln_map_set_geojson_source_synchronous_tiling(
                     map, VIEW("plain"), false, &completion.descriptor
                   )
  );
  EXPECT_STYLE_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "source does not exist",
    mln_map_set_geojson_source_synchronous_tiling(
      map, VIEW("missing"), true, &completion.descriptor
    )
  );

  // A released handle rejects new installs at submit, a second destroy is a
  // no-op, and installed sources keep their own reference.
  mln_geojson_source_data_destroy(plain_data);
  mln_geojson_source_data_destroy(MLN_HANDLE_NULL);
  mln_completion rejected = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(map, VIEW("plain"), plain_data, &rejected)
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
  RUN_TEST(remove_commands_commit_and_report_missing_ids);
  RUN_TEST(an_in_use_source_removal_fails_and_leaves_the_source);
  RUN_TEST(geojson_source_data_create_rejects_unsafe_raw_values);
  RUN_TEST(clustered_geojson_data_reports_non_point_geometry);
  RUN_TEST(clustered_geojson_data_requires_a_feature_collection);
  RUN_TEST(prepared_geojson_data_installs_and_checks_options);
  RUN_TEST(layer_info_reports_scalars_and_sizes_the_source_id_copy);
  RUN_TEST(image_source_coordinates_are_borrowed_by_the_completion);
  RUN_TEST(style_image_stretches_are_borrowed_by_the_completion);
}
