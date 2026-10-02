// Style commands that refuse their input or their target: empty IDs, URLs, and
// tile lists, images without pixels, invalid light and style JSON, and sources
// whose ID is taken or whose kind does not take the command.

#include "support/style.h"
#include "support/test_support.h"

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

static void ignore_tile(void* user_data, mln_canonical_tile_id tile_id) {
  (void)user_data;
  (void)tile_id;
}

static const mln_lat_lng corners[4] = {{1, 1}, {1, 2}, {0, 2}, {0, 1}};

static void style_commands_reject_invalid_inputs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_style_serve(runtime, NULL, 0);
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  const mln_buffer_view unnamed = MLN_BUFFER_LITERAL("");
  const mln_buffer_view png = MLN_BUFFER_LITERAL("fixture://image.png");
  const char* empty_source_id = "source_id must not be empty";

  mln_custom_geometry_source_options geometry =
    mln_custom_geometry_source_options_default();
  geometry.fetch_tile = ignore_tile;
  EXPECT_REFUSED(
    empty_source_id,
    mln_map_add_custom_geometry_source(
      map, unnamed, &geometry, &completion.descriptor, &diagnostic
    )
  );
  mln_custom_mvt_vector_source_options mvt =
    mln_custom_mvt_vector_source_options_default();
  mvt.fetch_tile = ignore_tile;
  EXPECT_REFUSED(
    empty_source_id, mln_map_add_custom_mvt_vector_source(
                       map, unnamed, &mvt, &completion.descriptor, &diagnostic
                     )
  );
  EXPECT_REFUSED(
    empty_source_id,
    mln_map_add_geojson_source_url(
      map, unnamed, MLN_BUFFER_LITERAL("fixture://points.geojson"), NULL,
      &completion.descriptor, &diagnostic
    )
  );
  EXPECT_REFUSED(
    empty_source_id, mln_map_get_style_source_info(
                       map, unnamed, &completion.descriptor, &diagnostic
                     )
  );
  EXPECT_REFUSED(
    "layer_id must not be empty",
    mln_map_get_style_layer_info(
      map, unnamed, &completion.descriptor, &diagnostic
    )
  );
  static const mln_buffer_view no_tiles[1] = {{0}};
  EXPECT_REFUSED(
    "tile_count must be greater than 0",
    mln_map_add_vector_source_tiles(
      map, MLN_BUFFER_LITERAL("empty"), no_tiles, 0, NULL,
      &completion.descriptor, &diagnostic
    )
  );
  EXPECT_REFUSED(
    "tiles must not be null", mln_map_add_vector_source_tiles(
                                map, MLN_BUFFER_LITERAL("missing"), NULL, 1,
                                NULL, &completion.descriptor, &diagnostic
                              )
  );
  EXPECT_REFUSED(
    empty_source_id,
    mln_map_add_image_source_url(
      map, unnamed, corners, 4, png, &completion.descriptor, &diagnostic
    )
  );
  EXPECT_REFUSED(
    "url must not be empty", mln_map_add_image_source_url(
                               map, MLN_BUFFER_LITERAL("image"), corners, 4,
                               unnamed, &completion.descriptor, &diagnostic
                             )
  );
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

  mln_geojson_source_data data = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_geojson_source_data_create(
    MLN_BUFFER_LITERAL("{\"type\":\"FeatureCollection\",\"features\":[]}"),
    NULL, &data, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "source_id must not be empty",
    mln_map_add_geojson_source_data(
      map, MLN_BUFFER_LITERAL(""), data, &completion.descriptor, NULL
    )
  );
  mln_geojson_source_data_destroy(data);

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
  RUN_TEST(style_commands_reject_invalid_inputs);
  RUN_TEST(source_commands_fail_on_a_taken_or_wrong_target);
}
