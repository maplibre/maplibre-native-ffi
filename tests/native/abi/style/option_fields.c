// Option fields that a host marks present: each one reaches its command,
// where it is validated and applied, and the image and global-state paths that
// read them back.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static const char clustered_points[] =
  "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"
  "\"properties\":{}}]}";

static mln_geojson_source_data prepare(
  const mln_geojson_source_options* options
) {
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_geojson_source_data_create(
      mln_test_view_of(clustered_points), options, &data, MLN_TEST_DIAGNOSTIC
    )
  );
  return data;
}

typedef struct geojson_field_case {
  const char* label;
  uint32_t field;
  void (*change)(mln_geojson_source_options* options);
} geojson_field_case;

static void change_cluster_radius(mln_geojson_source_options* options) {
  options->cluster_radius = 80;
}

static void change_cluster_max_zoom(mln_geojson_source_options* options) {
  options->cluster_max_zoom = 12;
}

static void change_cluster_min_points(mln_geojson_source_options* options) {
  options->cluster_min_points = 5;
}

static void change_buffer(mln_geojson_source_options* options) {
  options->buffer = 64;
}

// A source keeps the options its first data carried, so data that differs in
// any one present field does not match it, and identical data does.
static void each_present_geojson_field_takes_part_in_the_match(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER |
                   MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS |
                   MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM |
                   MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS |
                   MLN_GEOJSON_SOURCE_OPTION_BUFFER;
  options.cluster = true;
  options.cluster_radius = 40;
  options.cluster_max_zoom = 10;
  options.cluster_min_points = 3;
  options.buffer = 0;
  mln_geojson_source_data added = prepare(&options);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(
      map, MLN_BUFFER_LITERAL("points"), added, &completion.descriptor, NULL
    )
  );
  mln_geojson_source_data_destroy(added);

  static const geojson_field_case cases[] = {
    {"cluster_radius", MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS,
     change_cluster_radius},
    {"cluster_max_zoom", MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM,
     change_cluster_max_zoom},
    {"cluster_min_points", MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS,
     change_cluster_min_points},
    {"buffer", MLN_GEOJSON_SOURCE_OPTION_BUFFER, change_buffer},
  };
  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index += 1) {
    mln_geojson_source_options variant = options;
    cases[index].change(&variant);
    mln_geojson_source_data data = prepare(&variant);
    mln_test_completion completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_map_set_geojson_source_data(
        map, MLN_BUFFER_LITERAL("points"), data, &completion.descriptor, NULL
      )
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT, mln_test_completion_settle(&completion),
      cases[index].label
    );
    mln_geojson_source_data_destroy(data);
  }

  mln_geojson_source_data matching = prepare(&options);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_data(
      map, MLN_BUFFER_LITERAL("points"), matching, &completion.descriptor, NULL
    )
  );
  mln_geojson_source_data_destroy(matching);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void ignore_tile(void* user_data, mln_canonical_tile_id tile) {
  (void)user_data;
  (void)tile;
}

static mln_custom_geometry_source_options every_geometry_field(void) {
  mln_custom_geometry_source_options options =
    mln_custom_geometry_source_options_default();
  options.fields = MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
  options.min_zoom = 1;
  options.max_zoom = 14;
  options.tolerance = 0.5;
  options.tile_size = 256;
  options.buffer = 64;
  options.clip = true;
  options.wrap = true;
  // A source may leave cancellation out.
  options.fetch_tile = ignore_tile;
  options.cancel_tile = NULL;
  return options;
}

static void set_negative_tolerance(
  mln_custom_geometry_source_options* options
) {
  options->tolerance = -1;
}

static void set_zero_tile_size(mln_custom_geometry_source_options* options) {
  options->tile_size = 0;
}

static void set_oversized_buffer(mln_custom_geometry_source_options* options) {
  options->buffer = 70000;
}

typedef struct geometry_field_case {
  const char* fragment;
  void (*change)(mln_custom_geometry_source_options* options);
} geometry_field_case;

// A custom geometry source takes every present field, validating each one
// against the value the host set rather than the default.
static void a_custom_geometry_source_validates_and_applies_every_field(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  static const geometry_field_case rejected[] = {
    {"tolerance", set_negative_tolerance},
    {"tile_size", set_zero_tile_size},
    {"buffer", set_oversized_buffer},
  };
  for (size_t index = 0; index < sizeof(rejected) / sizeof(rejected[0]);
       index += 1) {
    mln_custom_geometry_source_options options = every_geometry_field();
    rejected[index].change(&options);
    MLN_TEST_EXPECT_COMMAND_REJECTED(
      rejected[index].fragment, mln_map_add_custom_geometry_source(
                                  map, MLN_BUFFER_LITERAL("custom"), &options,
                                  &completion.descriptor, MLN_TEST_DIAGNOSTIC
                                )
    );
  }

  const mln_custom_geometry_source_options options = every_geometry_field();
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_custom_geometry_source(
      map, MLN_BUFFER_LITERAL("custom"), &options, &completion.descriptor, NULL
    )
  );
  mln_test_completion info =
    mln_test_completion_default(sizeof(mln_style_source_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(
                     map, MLN_BUFFER_LITERAL("custom"), &info.descriptor, NULL
                   )
  );
  mln_style_source_result source = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&info, &source, sizeof(source))
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR, source.info.type
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void ignore_mvt_tile(void* user_data, mln_canonical_tile_id tile) {
  (void)user_data;
  (void)tile;
}

// A custom MVT source validates both zoom bounds the host marks present, and
// their order once applied.
static void a_custom_mvt_source_validates_its_present_zoom_bounds(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  mln_custom_mvt_vector_source_options options =
    mln_custom_mvt_vector_source_options_default();
  options.fields = MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM |
                   MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  options.fetch_tile = ignore_mvt_tile;
  options.min_zoom = 2;
  options.max_zoom = 33;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "max_zoom", mln_map_add_custom_mvt_vector_source(
                  map, MLN_BUFFER_LITERAL("mvt"), &options,
                  &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  options.max_zoom = 1;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "min_zoom", mln_map_add_custom_mvt_vector_source(
                  map, MLN_BUFFER_LITERAL("mvt"), &options,
                  &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  options.max_zoom = 12;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL("mvt"), &options, &completion.descriptor, NULL
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_premultiplied_rgba8_image one_pixel_image(const uint8_t* pixel) {
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = 4;
  return image;
}

// Copies the stretch intervals of image `id`, and reports whether it exists.
static bool image_has_stretches(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_image_stretches_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_stretches(
                     map, mln_test_view_of(id), &completion.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  const bool found = mln_test_completion_value_count(&completion) == 1;
  mln_test_completion_destroy(&completion);
  return found;
}

// A present text fit reaches the image, a stretch interval with no width is
// rejected before the command runs, and a missing image has no stretches.
static void style_image_options_apply_text_fit_and_reject_empty_stretches(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  const uint8_t pixel[4] = {0, 0, 0, 0};
  const mln_premultiplied_rgba8_image image = one_pixel_image(pixel);

  mln_style_image_options options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, MLN_BUFFER_LITERAL("fit"), &image, &options,
                     &completion.descriptor, NULL
                   )
  );
  mln_test_completion info =
    mln_test_completion_default(sizeof(mln_style_image_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_image_info(
                     map, MLN_BUFFER_LITERAL("fit"), &info.descriptor, NULL
                   )
  );
  mln_style_image_result result = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&info, &result, sizeof(result))
  );
  TEST_ASSERT_TRUE(result.info.has_text_fit_height);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, result.info.text_fit_height
  );
  TEST_ASSERT_FALSE(result.info.has_text_fit_width);

  const mln_image_stretch empty = {.from = 1.0f, .to = 1.0f};
  mln_style_image_options stretched = mln_style_image_options_default();
  stretched.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  stretched.stretch_x = &empty;
  stretched.stretch_x_count = 1;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "positive width", mln_map_set_style_image(
                        map, MLN_BUFFER_LITERAL("empty-stretch"), &image,
                        &stretched, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                      )
  );

  TEST_ASSERT_TRUE(image_has_stretches(map, "fit"));
  TEST_ASSERT_FALSE(image_has_stretches(map, "missing"));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A global-state property set on the loaded style reads back in the style's
// global state.
static void a_global_state_property_reads_back_in_the_global_state(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_global_state_property(
                     map, MLN_BUFFER_LITERAL("level"), MLN_BUFFER_LITERAL("3"),
                     &completion.descriptor, NULL
                   )
  );
  mln_test_completion state = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_global_state(map, &state.descriptor, NULL)
  );
  char json[256];
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_style_finish_text(&state, json, sizeof(json), &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(json, "\"level\":3"), json);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(each_present_geojson_field_takes_part_in_the_match);
  RUN_TEST(a_custom_geometry_source_validates_and_applies_every_field);
  RUN_TEST(a_custom_mvt_source_validates_its_present_zoom_bounds);
  RUN_TEST(style_image_options_apply_text_fit_and_reject_empty_stretches);
  RUN_TEST(a_global_state_property_reads_back_in_the_global_state);
}
