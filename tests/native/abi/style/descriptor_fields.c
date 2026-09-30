// Descriptor fields that no other case sets: every GeoJSON and custom source
// option at once, a raster DEM encoding, the location indicator's top image,
// the placement transition switch, a one-axis text fit, and the stretches of a
// missing image.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static void ignore_tile(void* user_data, mln_canonical_tile_id tile_id) {
  (void)user_data;
  (void)tile_id;
}

static uint32_t read_source_type(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_source_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(
                     map, mln_test_view_of(id), &completion.descriptor, NULL
                   )
  );
  mln_style_source_result result = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&completion, &result, sizeof(result))
  );
  return result.info.type;
}

// Prepared data takes every GeoJSON option field, and a source installs it.
static void every_geojson_option_field_prepares_data(void) {
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields =
    MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM | MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM |
    MLN_GEOJSON_SOURCE_OPTION_TOLERANCE |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM |
    MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE | MLN_GEOJSON_SOURCE_OPTION_BUFFER |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS |
    MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS | MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  options.min_zoom = 1.0;
  options.max_zoom = 16.0;
  options.tolerance = 0.5;
  options.cluster_max_zoom = 12.0;
  options.tile_size = 256;
  options.buffer = 64;
  options.cluster_radius = 40;
  options.cluster_min_points = 3;
  options.line_metrics = true;
  options.cluster = true;
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL("{\"type\":\"FeatureCollection\",\"features\":[]}"),
      &options, &data, MLN_TEST_DIAGNOSTIC
    )
  );

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(
      map, MLN_BUFFER_LITERAL("points"), data, &completion.descriptor, NULL
    )
  );
  mln_geojson_source_data_destroy(data);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_GEOJSON, read_source_type(map, "points")
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A custom source takes every option field, and a null cancel callback.
static void every_custom_source_option_field_adds_a_source(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  mln_custom_geometry_source_options geometry =
    mln_custom_geometry_source_options_default();
  geometry.fields = MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
  geometry.fetch_tile = ignore_tile;
  geometry.min_zoom = 0.0;
  geometry.max_zoom = 2.0;
  geometry.tolerance = 0.375;
  geometry.tile_size = 512;
  geometry.buffer = 64;
  geometry.clip = true;
  geometry.wrap = false;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_custom_geometry_source(
                     map, MLN_BUFFER_LITERAL("geometry"), &geometry,
                     &completion.descriptor, NULL
                   )
  );

  mln_custom_mvt_vector_source_options vector =
    mln_custom_mvt_vector_source_options_default();
  vector.fields = MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM |
                  MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  vector.fetch_tile = ignore_tile;
  vector.min_zoom = 1.0;
  vector.max_zoom = 3.0;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL("vector"), &vector, &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR, read_source_type(map, "geometry")
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR, read_source_type(map, "vector")
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A raster DEM source keeps the Mapbox encoding it was added with.
static void a_raster_dem_source_keeps_its_encoding(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_style_serve(runtime, NULL, 0);
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  static const mln_buffer_view tiles[] = {
    MLN_BUFFER_LITERAL("fixture://dem/{z}/{x}/{y}.png"),
  };
  mln_style_tile_source_options options =
    mln_style_tile_source_options_default();
  options.fields = MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
  options.raster_encoding = MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_raster_dem_source_tiles(
                     map, MLN_BUFFER_LITERAL("dem"), tiles, 1, &options,
                     &completion.descriptor, NULL
                   )
  );
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_source_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_source_info(
      map, MLN_BUFFER_LITERAL("dem"), &completion.descriptor, NULL
    )
  );
  mln_style_source_result result = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&completion, &result, sizeof(result))
  );
  TEST_ASSERT_TRUE(
    (result.info.fields & MLN_STYLE_SOURCE_INFO_RASTER_ENCODING) != 0
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX, result.info.raster_encoding
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The top image is the one image kind the location indicator case leaves out.
static void a_location_indicator_takes_a_top_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_location_indicator_layer(
                     map, MLN_BUFFER_LITERAL("puck"), MLN_BUFFER_LITERAL(""),
                     &completion.descriptor, NULL
                   )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_location_indicator_image_name(
      map, MLN_BUFFER_LITERAL("puck"), MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP,
      MLN_BUFFER_LITERAL("dot"), &completion.descriptor, NULL
    )
  );
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_layer_property(
      map, MLN_BUFFER_LITERAL("puck"), MLN_BUFFER_LITERAL("top-image"),
      &completion.descriptor, NULL
    )
  );
  char value[128];
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_style_finish_text(&completion, value, sizeof(value), NULL)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(value, "\"name\":\"dot\""), value);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Clearing the placement cross-fade is a field of its own, which the
// transition read reports back.
static void the_placement_transition_switch_round_trips(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  mln_style_transition_options options = mln_style_transition_options_default();
  options.fields = MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  options.enable_placement_transitions = false;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_transition_options(
                     map, &options, &completion.descriptor, NULL
                   )
  );
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_transition_options));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_transition_options(map, &completion.descriptor, NULL)
  );
  mln_style_transition_options read = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&completion, &read, sizeof(read))
  );
  TEST_ASSERT_TRUE(
    (read.fields & MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) !=
    0
  );
  TEST_ASSERT_FALSE(read.enable_placement_transitions);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A text fit set on one axis reads back on that axis alone.
static void a_text_fit_on_one_axis_reads_back_alone(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  static const uint8_t pixels[16] = {0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 2;
  image.height = 2;
  image.stride = 8;
  image.pixels = pixels;
  image.byte_length = sizeof(pixels);
  mln_style_image_options options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, MLN_BUFFER_LITERAL("fitted"), &image, &options,
                     &completion.descriptor, NULL
                   )
  );
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_image_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_image_info(
      map, MLN_BUFFER_LITERAL("fitted"), &completion.descriptor, NULL
    )
  );
  mln_style_image_result result = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&completion, &result, sizeof(result))
  );
  TEST_ASSERT_FALSE(result.info.has_text_fit_width);
  TEST_ASSERT_TRUE(result.info.has_text_fit_height);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, result.info.text_fit_height
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Copying the stretches of an image the style lacks succeeds with no value.
static void a_missing_image_has_no_stretches(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  mln_test_completion completion = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_image_stretches(
      map, MLN_BUFFER_LITERAL("missing"), &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&completion));
  TEST_ASSERT_EQUAL_size_t(0, mln_test_completion_value_count(&completion));
  mln_test_completion_destroy(&completion);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(every_geojson_option_field_prepares_data);
  RUN_TEST(every_custom_source_option_field_adds_a_source);
  RUN_TEST(a_raster_dem_source_keeps_its_encoding);
  RUN_TEST(a_location_indicator_takes_a_top_image);
  RUN_TEST(the_placement_transition_switch_round_trips);
  RUN_TEST(a_text_fit_on_one_axis_reads_back_alone);
  RUN_TEST(a_missing_image_has_no_stretches);
}
