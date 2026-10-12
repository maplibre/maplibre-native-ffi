// Style commands that refuse their input or their target: empty IDs, URLs,
// JSON, and tile lists, images without pixels, invalid light and style JSON,
// and sources whose ID is taken or whose kind does not take the command.

#include "support/style.h"
#include "support/test_support.h"

// Expects `expression`, which submits with `completion.descriptor`, to refuse
// the empty view `name` before it returns. The diagnostic names the view, and
// the completion stays with the caller without its callback running.
#define EXPECT_EMPTY_REJECTED(name, expression) \
  MLN_TEST_EXPECT_COMMAND_REJECTED(name " must not be empty", expression)

static void ignore_tile(void* user_data, mln_canonical_tile_id tile_id) {
  (void)user_data;
  (void)tile_id;
}

static const mln_lat_lng corners[4] = {{1, 1}, {1, 2}, {0, 2}, {0, 1}};

// Every ID, URL, JSON, and tile URL input that must name at least one byte is
// refused at submission when empty, so no empty view reaches the map worker.
static void empty_views_are_rejected_at_submission(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view empty = MLN_BUFFER_LITERAL("");
  const mln_buffer_view id = MLN_BUFFER_LITERAL("id");
  const mln_buffer_view json = MLN_BUFFER_LITERAL("{}");
  const mln_buffer_view url = MLN_BUFFER_LITERAL("fixture://data");
  const mln_canonical_tile_id tile = {.z = 0, .x = 0, .y = 0};
  const mln_lat_lng_bounds bounds = {
    .southwest = {.latitude = 0, .longitude = 0},
    .northeast = {.latitude = 1, .longitude = 1},
  };
  const uint8_t pixel[4] = {0, 0, 0, 0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixel;
  image.byte_length = sizeof(pixel);
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_geojson_source_data_create(
    MLN_BUFFER_LITERAL("{\"type\":\"FeatureCollection\",\"features\":[]}"),
    NULL, &data, MLN_TEST_DIAGNOSTIC
  ));
  mln_custom_geometry_source_options geometry =
    mln_custom_geometry_source_options_default();
  geometry.fetch_tile = ignore_tile;
  mln_custom_mvt_vector_source_options mvt =
    mln_custom_mvt_vector_source_options_default();
  mvt.fetch_tile = ignore_tile;
  const mln_buffer_view tiles[] = {url};
  const mln_buffer_view empty_tiles[] = {url, empty};

  // Sources.
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_add_style_source_json(
                   map, empty, json, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "source_json", mln_map_add_style_source_json(
                     map, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                   )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_remove_style_source(
                   map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_get_style_source(
                   map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_set_style_source_volatile(
                   map, empty, true, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );

  // GeoJSON sources.
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_geojson_source_url(
      map, empty, url, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "url", mln_map_add_geojson_source_url(
             map, id, empty, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
           )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_add_geojson_source_data(
                   map, empty, data, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_set_geojson_source_url(
                   map, empty, url, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "url", mln_map_set_geojson_source_url(
             map, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
           )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_set_geojson_source_data(
                   map, empty, data, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_set_geojson_source_synchronous_tiling(
                   map, empty, true, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );

  // Tile sources.
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_vector_source_url(
      map, empty, url, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "url", mln_map_add_vector_source_url(
             map, id, empty, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
           )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_raster_source_url(
      map, empty, url, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "url", mln_map_add_raster_source_url(
             map, id, empty, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
           )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_raster_dem_source_url(
      map, empty, url, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "url", mln_map_add_raster_dem_source_url(
             map, id, empty, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
           )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_vector_source_tiles(
      map, empty, tiles, 1, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "tile URL",
    mln_map_add_vector_source_tiles(
      map, id, empty_tiles, 2, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "tile_count must be greater than 0",
    mln_map_add_vector_source_tiles(
      map, id, tiles, 0, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "tiles must not be null",
    mln_map_add_vector_source_tiles(
      map, id, NULL, 1, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_raster_source_tiles(
      map, empty, tiles, 1, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "tile URL",
    mln_map_add_raster_source_tiles(
      map, id, empty_tiles, 2, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_raster_dem_source_tiles(
      map, empty, tiles, 1, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "tile URL",
    mln_map_add_raster_dem_source_tiles(
      map, id, empty_tiles, 2, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );

  // Custom sources. A rejected add never takes the callbacks' user_data;
  // custom_sources.c shows that the caller keeps it.
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_custom_geometry_source(
      map, empty, &geometry, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_set_custom_geometry_source_tile_data(
      map, empty, tile, json, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "data", mln_map_set_custom_geometry_source_tile_data(
              map, id, tile, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
            )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_invalidate_custom_geometry_source_tile(
                   map, empty, tile, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_invalidate_custom_geometry_source_region(
      map, empty, bounds, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_add_custom_mvt_vector_source(
                   map, empty, &mvt, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_set_custom_mvt_vector_source_tile_data(
      map, empty, tile, json, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_set_custom_mvt_vector_source_tile_error(
      map, empty, tile, json, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_invalidate_custom_mvt_vector_source_tile(
                   map, empty, tile, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );

  // Images and image sources.
  EXPECT_EMPTY_REJECTED(
    "image_id",
    mln_map_set_style_image(
      map, empty, &image, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "image_id", mln_map_remove_style_image(
                  map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "image_id", mln_map_get_style_image(
                  map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_image_source_url(
      map, empty, corners, 4, url, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "url",
    mln_map_add_image_source_url(
      map, id, corners, 4, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_add_image_source_image(
                   map, empty, corners, 4, &image, &completion.descriptor,
                   MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_set_image_source_url(
                   map, empty, url, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "url", mln_map_set_image_source_url(
             map, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
           )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_set_image_source_image(
      map, empty, &image, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_set_image_source_coordinates(
      map, empty, corners, 4, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_get_image_source_coordinates(
                   map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );

  // Layers. An empty before_layer_id means the top of the stack, so it is
  // accepted.
  EXPECT_EMPTY_REJECTED(
    "layer_id",
    mln_map_add_hillshade_layer(
      map, empty, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_hillshade_layer(
      map, id, empty, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id",
    mln_map_add_color_relief_layer(
      map, empty, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id",
    mln_map_add_color_relief_layer(
      map, id, empty, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_add_location_indicator_layer(
                  map, empty, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id",
    mln_map_set_location_indicator_location(
      map, empty, corners[0], 0, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_set_location_indicator_bearing(
                  map, empty, 0, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_set_location_indicator_accuracy_radius(
                  map, empty, 0, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_set_location_indicator_image_name(
                  map, empty, MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP, id,
                  &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "image_id", mln_map_set_location_indicator_image_name(
                  map, id, MLN_LOCATION_INDICATOR_IMAGE_KIND_TOP, empty,
                  &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_json",
    mln_map_add_style_layer_json(
      map, empty, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_remove_style_layer(
                  map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_get_style_layer(
                  map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_move_style_layer(
                  map, empty, id, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_get_style_layer_json(
                  map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id",
    mln_map_set_style_layer_property(
      map, empty, id, json, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "property_name",
    mln_map_set_style_layer_property(
      map, id, empty, json, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "value", mln_map_set_style_layer_property(
               map, id, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
             )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_get_style_layer_property(
                  map, empty, id, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "property_name",
    mln_map_get_style_layer_property(
      map, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_set_style_layer_filter(
                  map, empty, NULL, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "filter", mln_map_set_style_layer_filter(
                map, id, &empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
              )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_get_style_layer_filter(
                  map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_set_style_layer_source_layer(
                  map, empty, id, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_set_style_layer_source_id(
                  map, empty, id, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "source_id", mln_map_set_style_layer_source_id(
                   map, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                 )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_set_style_layer_min_zoom(
                  map, empty, 0, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_set_style_layer_max_zoom(
                  map, empty, 0, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );
  EXPECT_EMPTY_REJECTED(
    "layer_id", mln_map_set_style_layer_visibility(
                  map, empty, MLN_STYLE_LAYER_VISIBILITY_NONE,
                  &completion.descriptor, MLN_TEST_DIAGNOSTIC
                )
  );

  // Style-wide values.
  EXPECT_EMPTY_REJECTED(
    "value", mln_map_set_global_state_property(
               map, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
             )
  );
  EXPECT_EMPTY_REJECTED(
    "light_json", mln_map_set_style_light_json(
                    map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                  )
  );
  EXPECT_EMPTY_REJECTED(
    "property_name",
    mln_map_set_style_light_property(
      map, empty, json, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  EXPECT_EMPTY_REJECTED(
    "value", mln_map_set_style_light_property(
               map, id, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
             )
  );
  EXPECT_EMPTY_REJECTED(
    "property_name", mln_map_get_style_light_property(
                       map, empty, &completion.descriptor, MLN_TEST_DIAGNOSTIC
                     )
  );

  mln_geojson_source_data_destroy(data);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Some inputs are refused at submission and some by the command, so this reads
// the status and diagnostic from whichever stage refused `expression`, which
// submits with `completion.descriptor` and `&diagnostic`.
#define EXPECT_REFUSED(fragment, expression)                                \
  do {                                                                      \
    mln_test_completion completion = mln_test_completion_default(0);        \
    mln_diagnostic diagnostic = {.size = sizeof(mln_diagnostic)};           \
    mln_status status = (expression);                                       \
    const char* message = diagnostic.message;                               \
    if (status == MLN_STATUS_OK) {                                          \
      status = mln_test_completion_finish(&completion);                     \
      message = mln_test_completion_diagnostic(&completion);                \
    } else {                                                                \
      mln_test_completion_reject(&completion);                              \
    }                                                                       \
    TEST_ASSERT_EQUAL_INT_MESSAGE(                                          \
      MLN_STATUS_INVALID_ARGUMENT, status, #expression                      \
    );                                                                      \
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(message, (fragment)), #expression); \
    mln_test_completion_destroy(&completion);                               \
  } while (false)

static void style_commands_reject_invalid_inputs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_style_serve(runtime, NULL, 0);
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_premultiplied_rgba8_image no_pixels =
    mln_premultiplied_rgba8_image_default();
  no_pixels.width = 1;
  no_pixels.height = 1;
  no_pixels.stride = 4;
  no_pixels.byte_length = 4;
  EXPECT_REFUSED(
    "pixels must not be null", mln_map_set_style_image(
                                 map, MLN_BUFFER_LITERAL("marker"), &no_pixels,
                                 NULL, &completion.descriptor, &diagnostic
                               )
  );
  EXPECT_REFUSED(
    "pixels must not be null", mln_map_add_image_source_image(
                                 map, MLN_BUFFER_LITERAL("image"), corners, 4,
                                 &no_pixels, &completion.descriptor, &diagnostic
                               )
  );
  EXPECT_REFUSED(
    "style light property",
    mln_map_set_style_light_property(
      map, MLN_BUFFER_LITERAL("intensity"), MLN_BUFFER_LITERAL("{"),
      &completion.descriptor, &diagnostic
    )
  );
  // Last: a style that fails to load leaves the map without its style.
  static const char nul_json[] = "{\0}";
  EXPECT_REFUSED(
    "must not contain embedded NUL",
    mln_map_set_style_json(
      map, mln_test_buffer_view(nul_json, sizeof(nul_json) - 1),
      &completion.descriptor, &diagnostic
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A command whose target is taken or of the wrong kind fails at commit with
// the reason.
static void source_commands_fail_on_a_taken_or_wrong_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  static mln_test_style_route routes[] = {
    {.url = "fixture://taken.geojson", .body = "{}"},
  };
  mln_test_style_serve(runtime, routes, 1);
  const mln_buffer_view taken = MLN_BUFFER_LITERAL("taken");
  const mln_buffer_view png = MLN_BUFFER_LITERAL("fixture://image.png");
  const mln_buffer_view geojson_url =
    MLN_BUFFER_LITERAL("fixture://taken.geojson");
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, taken, MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE),
    &completion.descriptor, NULL
  ));

  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "style source",
    mln_map_add_style_source_json(
      map, MLN_BUFFER_LITERAL("unknown-type"),
      MLN_BUFFER_LITERAL("{\"type\":\"no-such-source-type\"}"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "source already exists",
    mln_map_add_geojson_source_url(
      map, taken, geojson_url, NULL, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "source already exists",
    mln_map_add_image_source_url(
      map, taken, corners, 4, png, &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not an image source",
    mln_map_set_image_source_url(map, taken, png, &completion.descriptor, NULL)
  );

  // A URL update and the synchronous tiling override belong to GeoJSON
  // sources alone. internal/geojson_tiling.cpp shows that the override
  // reaches the next frame.
  const mln_buffer_view image = MLN_BUFFER_LITERAL("image");
  MLN_TEST_AWAIT_OK(mln_map_add_image_source_url(
    map, image, corners, 4, png, &completion.descriptor, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not a GeoJSON source",
    mln_map_set_geojson_source_url(
      map, image, geojson_url, &completion.descriptor, NULL
    )
  );
  const mln_buffer_view tiles[] = {
    MLN_BUFFER_LITERAL("fixture://tiles/{z}/{x}/{y}.mvt"),
  };
  const mln_buffer_view vector = MLN_BUFFER_LITERAL("vector");
  MLN_TEST_AWAIT_OK(mln_map_add_vector_source_tiles(
    map, vector, tiles, 1, NULL, &completion.descriptor, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not a GeoJSON source",
    mln_map_set_geojson_source_synchronous_tiling(
      map, vector, true, &completion.descriptor, NULL
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(empty_views_are_rejected_at_submission);
  RUN_TEST(style_commands_reject_invalid_inputs);
  RUN_TEST(source_commands_fail_on_a_taken_or_wrong_target);
}
