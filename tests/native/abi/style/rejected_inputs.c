// Inputs the style functions reject that no other case sends: invalid GeoJSON
// data and cluster properties, empty source, layer, and image IDs, an empty
// image source URL, an empty or missing tile list, images without pixels,
// invalid light JSON, and style JSON with an embedded NUL. Some are refused at
// submission and some by the command, so each row reads the status from
// whichever stage refused it.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

typedef struct geojson_case {
  const char* label;
  const char* data;
  // A cluster_properties value to set, or null to set none.
  const char* cluster_properties;
  bool cluster;
  const char* fragment;
} geojson_case;

static void geojson_data_rejects_invalid_documents_and_properties(void) {
  static const char points[] =
    "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
    "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"
    "\"properties\":{\"rank\":1}}]}";
  static const geojson_case cases[] = {
    {"an unknown geometry type",
     "{\"type\":\"Unsupported\",\"coordinates\":[]}", NULL, false,
     "is invalid"},
    {"a clustered line string",
     "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
     "\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]},"
     "\"properties\":{}}]}",
     NULL, true, "line string"},
    {"empty cluster properties", points, "", true, "must not be empty"},
    {"cluster properties that are not JSON", points, "{\"total\":NaN}", true,
     "cluster_properties"},
  };
  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index += 1) {
    const geojson_case* row = &cases[index];
    mln_geojson_source_options options = mln_geojson_source_options_default();
    if (row->cluster) {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
      options.cluster = true;
    }
    if (row->cluster_properties != NULL) {
      options.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
      options.cluster_properties = mln_test_view_of(row->cluster_properties);
    }
    mln_geojson_source_data data = MLN_HANDLE_NULL;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_geojson_source_data_create(
        mln_test_view_of(row->data), &options, &data, MLN_TEST_DIAGNOSTIC
      ),
      row->label
    );
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(MLN_HANDLE_NULL, data, row->label);
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(mln_test_last_error(), row->fragment), row->label
    );
  }
}

static void ignore_tile(void* user_data, mln_canonical_tile_id tile_id) {
  (void)user_data;
  (void)tile_id;
}

static const mln_lat_lng image_corners[] = {
  {.latitude = 1, .longitude = 1},
  {.latitude = 1, .longitude = 2},
  {.latitude = 0, .longitude = 2},
  {.latitude = 0, .longitude = 1},
};

static mln_premultiplied_rgba8_image image_without_pixels(void) {
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.byte_length = 4;
  return image;
}

static mln_status add_unnamed_custom_geometry_source(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  mln_custom_geometry_source_options options =
    mln_custom_geometry_source_options_default();
  options.fetch_tile = ignore_tile;
  return mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL(""), &options, completion, diagnostic
  );
}

static mln_status add_unnamed_custom_mvt_source(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  mln_custom_mvt_vector_source_options options =
    mln_custom_mvt_vector_source_options_default();
  options.fetch_tile = ignore_tile;
  return mln_map_add_custom_mvt_vector_source(
    map, MLN_BUFFER_LITERAL(""), &options, completion, diagnostic
  );
}

static mln_status add_unnamed_geojson_url_source(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  return mln_map_add_geojson_source_url(
    map, MLN_BUFFER_LITERAL(""), MLN_BUFFER_LITERAL("fixture://points.geojson"),
    NULL, completion, diagnostic
  );
}

static mln_status read_unnamed_source(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  return mln_map_get_style_source_info(
    map, MLN_BUFFER_LITERAL(""), completion, diagnostic
  );
}

static mln_status read_unnamed_layer(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  return mln_map_get_style_layer_info(
    map, MLN_BUFFER_LITERAL(""), completion, diagnostic
  );
}

static mln_status add_vector_source_without_tiles(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  static const mln_buffer_view no_tiles[1] = {{0}};
  return mln_map_add_vector_source_tiles(
    map, MLN_BUFFER_LITERAL("empty"), no_tiles, 0, NULL, completion, diagnostic
  );
}

static mln_status add_vector_source_with_missing_tiles(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  return mln_map_add_vector_source_tiles(
    map, MLN_BUFFER_LITERAL("missing"), NULL, 1, NULL, completion, diagnostic
  );
}

static mln_status add_unnamed_image_source(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  return mln_map_add_image_source_url(
    map, MLN_BUFFER_LITERAL(""), image_corners, 4,
    MLN_BUFFER_LITERAL("fixture://image.png"), completion, diagnostic
  );
}

static mln_status add_image_source_without_a_url(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  return mln_map_add_image_source_url(
    map, MLN_BUFFER_LITERAL("image"), image_corners, 4, MLN_BUFFER_LITERAL(""),
    completion, diagnostic
  );
}

static mln_status set_image_without_pixels(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  const mln_premultiplied_rgba8_image image = image_without_pixels();
  return mln_map_set_style_image(
    map, MLN_BUFFER_LITERAL("marker"), &image, NULL, completion, diagnostic
  );
}

static mln_status add_image_source_without_pixels(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  const mln_premultiplied_rgba8_image image = image_without_pixels();
  return mln_map_add_image_source_image(
    map, MLN_BUFFER_LITERAL("image"), image_corners, 4, &image, completion,
    diagnostic
  );
}

static mln_status set_light_to_invalid_json(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  return mln_map_set_style_light_property(
    map, MLN_BUFFER_LITERAL("intensity"), MLN_BUFFER_LITERAL("{"), completion,
    diagnostic
  );
}

static mln_status set_style_json_with_a_nul(
  mln_map map, const mln_completion* completion, mln_diagnostic* diagnostic
) {
  static const char json[] = "{\0}";
  return mln_map_set_style_json(
    map, (mln_buffer_view){.data = json, .size = sizeof(json) - 1}, completion,
    diagnostic
  );
}

typedef struct command_case {
  const char* label;
  mln_status (*submit)(mln_map, const mln_completion*, mln_diagnostic*);
  const char* fragment;
} command_case;

static void style_commands_reject_invalid_inputs(void) {
  static const command_case cases[] = {
    {"an unnamed custom geometry source", add_unnamed_custom_geometry_source,
     "source_id must not be empty"},
    {"an unnamed custom MVT source", add_unnamed_custom_mvt_source,
     "source_id must not be empty"},
    {"an unnamed GeoJSON URL source", add_unnamed_geojson_url_source,
     "source_id must not be empty"},
    {"a read of an unnamed source", read_unnamed_source,
     "source_id must not be empty"},
    {"a read of an unnamed layer", read_unnamed_layer,
     "layer_id must not be empty"},
    {"a vector source without tiles", add_vector_source_without_tiles,
     "tile_count must be greater than 0"},
    {"a vector source whose tile list is missing",
     add_vector_source_with_missing_tiles, "tiles must not be null"},
    {"an unnamed image source", add_unnamed_image_source,
     "source_id must not be empty"},
    {"an image source without a URL", add_image_source_without_a_url,
     "url must not be empty"},
    {"an image without pixels", set_image_without_pixels,
     "pixels must not be null"},
    {"an image source without pixels", add_image_source_without_pixels,
     "pixels must not be null"},
    {"light JSON that does not parse", set_light_to_invalid_json,
     "style light property"},
    // Run last: a style that fails to load leaves the map without its
    // style.
    {"style JSON with an embedded NUL", set_style_json_with_a_nul,
     "must not contain embedded NUL"},
  };
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_style_serve(runtime, NULL, 0);
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL(
      "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background\","
      "\"type\":\"background\"}]}"
    )
  );
  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index += 1) {
    const command_case* row = &cases[index];
    mln_test_completion completion = mln_test_completion_default(0);
    mln_diagnostic diagnostic = {.size = sizeof(mln_diagnostic)};
    mln_status status = row->submit(map, &completion.descriptor, &diagnostic);
    const char* message = diagnostic.message;
    if (status == MLN_STATUS_OK) {
      status = mln_test_completion_finish(&completion);
      message = mln_test_completion_diagnostic(&completion);
    } else {
      mln_test_completion_reject(&completion);
    }
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT, status, row->label
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(message, row->fragment), row->label);
    mln_test_completion_destroy(&completion);
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(geojson_data_rejects_invalid_documents_and_properties);
  RUN_TEST(style_commands_reject_invalid_inputs);
}
