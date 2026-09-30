// Optional descriptor fields that only a successful style command converts:
// every GeoJSON, custom geometry, and custom MVT option, a raster-DEM encoding,
// an image's text fit, the placement transition flag, the location indicator's
// images, a layer inserted before another, and global state.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static const char point_collection[] =
  "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"
  "\"properties\":{\"rank\":1}}]}";

static mln_geojson_source_options every_geojson_option(void) {
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
  options.buffer = 32;
  options.cluster_radius = 40;
  options.cluster_min_points = 3;
  options.line_metrics = true;
  options.cluster = true;
  return options;
}

static mln_geojson_source_data prepare(
  const mln_geojson_source_options* options
) {
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_geojson_source_data_create(
      mln_test_view_of(point_collection), options, &data, MLN_TEST_DIAGNOSTIC
    )
  );
  return data;
}

// Data prepared with every option installs on a source, and a later set
// accepts only data prepared with the same options, so each option reached
// the prepared tiler. A URL source takes the same options.
static void every_geojson_option_reaches_the_prepared_tiler(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  const mln_geojson_source_options options = every_geojson_option();
  mln_geojson_source_data data = prepare(&options);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(
      map, MLN_BUFFER_LITERAL("points"), data, &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_data(
      map, MLN_BUFFER_LITERAL("points"), data, &completion.descriptor, NULL
    )
  );
  mln_geojson_source_data_destroy(data);

  mln_geojson_source_options wider = every_geojson_option();
  wider.buffer = 64;
  mln_geojson_source_data mismatched = prepare(&wider);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "do not match",
    mln_map_set_geojson_source_data(
      map, MLN_BUFFER_LITERAL("points"), mismatched, &completion.descriptor,
      NULL
    )
  );
  mln_geojson_source_data_destroy(mismatched);

  static mln_test_style_route routes[] = {
    {.url = "fixture://points.geojson", .body = point_collection},
  };
  mln_test_style_serve(runtime, routes, 1);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_geojson_source_url(
                     map, MLN_BUFFER_LITERAL("remote"),
                     MLN_BUFFER_LITERAL("fixture://points.geojson"), &options,
                     &completion.descriptor, NULL
                   )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Cluster properties that name no known aggregation fail to convert when the
// data is prepared, before any map sees them.
static void unconvertible_cluster_properties_fail_preparation(void) {
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER |
                   MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  options.cluster = true;
  options.cluster_properties =
    MLN_BUFFER_LITERAL("{\"total\":[\"no-such-operator\",[\"get\",\"rank\"]]}");
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      mln_test_view_of(point_collection), &options, &data, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "GeoJSON source options"),
    mln_test_last_error()
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, data);
}

static void fetch_nothing(void* user_data, mln_canonical_tile_id tile_id) {
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

// Custom sources take every optional field, a custom MVT source validates its
// maximum zoom, and each reports its own source type.
static void custom_sources_take_every_optional_field(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  mln_custom_geometry_source_options geometry =
    mln_custom_geometry_source_options_default();
  geometry.fetch_tile = fetch_nothing;
  geometry.fields = MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP |
                    MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
  geometry.min_zoom = 0.0;
  geometry.max_zoom = 14.0;
  geometry.tolerance = 0.5;
  geometry.tile_size = 256;
  geometry.buffer = 64;
  geometry.clip = true;
  geometry.wrap = true;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_custom_geometry_source(
                     map, MLN_BUFFER_LITERAL("geometry"), &geometry,
                     &completion.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_VECTOR, read_source_type(map, "geometry")
  );

  mln_custom_mvt_vector_source_options mvt =
    mln_custom_mvt_vector_source_options_default();
  mvt.fetch_tile = fetch_nothing;
  mvt.fields = MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM |
               MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  mvt.min_zoom = 0.0;
  mvt.max_zoom = 33.0;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "max_zoom", mln_map_add_custom_mvt_vector_source(
                  map, MLN_BUFFER_LITERAL("mvt"), &mvt, &completion.descriptor,
                  MLN_TEST_DIAGNOSTIC
                )
  );
  mvt.max_zoom = 14.0;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL("mvt"), &mvt, &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_SOURCE_TYPE_CUSTOM_MVT_VECTOR, read_source_type(map, "mvt")
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A raster-DEM URL source carries an explicit encoding to MapLibre Native.
static void a_raster_dem_url_source_takes_an_explicit_encoding(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  static mln_test_style_route routes[] = {
    {.url = "fixture://dem.json",
     .body = "{\"tilejson\":\"2.2.0\",\"tiles\":[\"fixture://dem/{z}/{x}/"
             "{y}.png\"]}"},
  };
  mln_test_style_serve(runtime, routes, 1);
  mln_style_tile_source_options options =
    mln_style_tile_source_options_default();
  options.fields = MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
  options.raster_encoding = MLN_STYLE_RASTER_DEM_ENCODING_MAPBOX;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_raster_dem_source_url(
      map, MLN_BUFFER_LITERAL("dem"), MLN_BUFFER_LITERAL("fixture://dem.json"),
      &options, &completion.descriptor, NULL
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_style_image_info read_image_info(mln_map map, const char* id) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(mln_style_image_result));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_image_info(
                     map, mln_test_view_of(id), &completion.descriptor, NULL
                   )
  );
  mln_style_image_result result = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&completion, &result, sizeof(result))
  );
  return result.info;
}

// A text fit on one axis reads back on that axis alone, and a stretch
// interval with no width is rejected before the image reaches the style.
static void style_image_text_fit_round_trips_per_axis(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  const uint8_t pixels[4 * 4 * 4] = {0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 4;
  image.height = 4;
  image.stride = 16;
  image.pixels = pixels;
  image.byte_length = sizeof(pixels);

  mln_style_image_options options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_style_image(
                     map, MLN_BUFFER_LITERAL("fit"), &image, &options,
                     &completion.descriptor, NULL
                   )
  );
  const mln_style_image_info info = read_image_info(map, "fit");
  TEST_ASSERT_FALSE(info.has_text_fit_width);
  TEST_ASSERT_TRUE(info.has_text_fit_height);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, info.text_fit_height
  );

  const mln_image_stretch empty = {.from = 2.0f, .to = 2.0f};
  mln_style_image_options stretched = mln_style_image_options_default();
  stretched.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  stretched.stretch_x = &empty;
  stretched.stretch_x_count = 1;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "positive width", mln_map_set_style_image(
                        map, MLN_BUFFER_LITERAL("stretched"), &image,
                        &stretched, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                      )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The placement transition flag reads back as the command set it.
static void the_placement_transition_flag_round_trips(void) {
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
  mln_test_completion read =
    mln_test_completion_default(sizeof(mln_style_transition_options));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_transition_options(map, &read.descriptor, NULL)
  );
  mln_style_transition_options value = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&read, &value, sizeof(value))
  );
  TEST_ASSERT_TRUE(
    (value.fields & MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) !=
    0
  );
  TEST_ASSERT_FALSE(value.enable_placement_transitions);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void read_text(
  mln_test_completion* completion, char* out, size_t capacity
) {
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_style_finish_text(completion, out, capacity, &found)
  );
  TEST_ASSERT_TRUE(found);
}

static void read_layer_property(
  mln_map map, const char* layer, const char* name, char* out, size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_property(
                     map, mln_test_view_of(layer), mln_test_view_of(name),
                     &completion.descriptor, NULL
                   )
  );
  read_text(&completion, out, capacity);
}

// The location indicator names each of its three images by kind.
static void location_indicator_images_are_named_by_kind(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_location_indicator_layer(
                     map, MLN_BUFFER_LITERAL("puck"), MLN_BUFFER_LITERAL(""),
                     &completion.descriptor, NULL
                   )
  );
  static const struct {
    uint32_t kind;
    const char* image;
    const char* property;
  } images[] = {
    {MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP, "dot", "top-image"},
    {MLN_LOCATION_INDICATOR_IMAGE_KIND_SHADOW, "halo", "shadow-image"},
  };
  for (size_t index = 0; index < sizeof(images) / sizeof(images[0]);
       index += 1) {
    MLN_TEST_AWAIT_COMMAND(
      MLN_STATUS_OK,
      mln_map_set_location_indicator_image_name(
        map, MLN_BUFFER_LITERAL("puck"), images[index].kind,
        mln_test_view_of(images[index].image), &completion.descriptor, NULL
      )
    );
    char value[128] = {0};
    read_layer_property(
      map, "puck", images[index].property, value, sizeof(value)
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(value, images[index].image), value);
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A layer added before another lands directly below it in style order.
static void a_layer_added_before_another_lands_below_it(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_style_layer_json(
      map, MLN_BUFFER_LITERAL("{\"id\":\"top\",\"type\":\"background\"}"),
      MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_add_style_layer_json(
      map, MLN_BUFFER_LITERAL("{\"id\":\"under\",\"type\":\"background\"}"),
      MLN_BUFFER_LITERAL("top"), &completion.descriptor, NULL
    )
  );
  const mln_test_style_list layers = mln_test_style_list_layer_ids(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, layers.status);
  TEST_ASSERT_EQUAL_size_t(2, layers.count);
  TEST_ASSERT_EQUAL_STRING("under", layers.entries[0].id);
  TEST_ASSERT_EQUAL_STRING("top", layers.entries[1].id);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A property set on the loaded style's global state reads back in the state
// object, beside the style's own defaults.
static void global_state_properties_read_back(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"state\":{\"mode\":{\"default\":\"day\"}},"
      "\"sources\":{},\"layers\":[]}"
    )
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_set_global_state_property(
      map, MLN_BUFFER_LITERAL("theme"), MLN_BUFFER_LITERAL("\"dark\""),
      &completion.descriptor, NULL
    )
  );
  mln_test_completion completion = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_global_state(map, &completion.descriptor, NULL)
  );
  char state[256] = {0};
  read_text(&completion, state, sizeof(state));
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(state, "\"theme\":\"dark\""), state);
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(state, "\"mode\":\"day\""), state);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(every_geojson_option_reaches_the_prepared_tiler);
  RUN_TEST(unconvertible_cluster_properties_fail_preparation);
  RUN_TEST(custom_sources_take_every_optional_field);
  RUN_TEST(a_raster_dem_url_source_takes_an_explicit_encoding);
  RUN_TEST(style_image_text_fit_round_trips_per_axis);
  RUN_TEST(the_placement_transition_flag_round_trips);
  RUN_TEST(location_indicator_images_are_named_by_kind);
  RUN_TEST(a_layer_added_before_another_lands_below_it);
  RUN_TEST(global_state_properties_read_back);
}
