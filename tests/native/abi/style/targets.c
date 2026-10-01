// Style commands whose target is missing, of the wrong kind, or already
// taken: each fails at commit with the reason. The reads of a missing target
// that find nothing are in images.c and sources.c.

#include <stdbool.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static const mln_lat_lng corners[4] = {
  {.latitude = 1.0, .longitude = 0.0},
  {.latitude = 1.0, .longitude = 1.0},
  {.latitude = 0.0, .longitude = 1.0},
  {.latitude = 0.0, .longitude = 0.0},
};

static void fetch_nothing(void* user_data, mln_canonical_tile_id tile_id) {
  (void)user_data;
  (void)tile_id;
}

static void add_json_source(mln_map map, const char* id) {
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, mln_test_view_of(id),
    MLN_BUFFER_LITERAL(MLN_TEST_EMPTY_GEOJSON_SOURCE), &completion.descriptor,
    NULL
  ));
}

static void source_commands_fail_on_a_taken_missing_or_wrong_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  static mln_test_style_route routes[] = {
    {.url = "fixture://taken.geojson", .body = "{}"},
  };
  mln_test_style_serve(runtime, routes, 1);
  add_json_source(map, "taken");

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
      map, MLN_BUFFER_LITERAL("taken"),
      MLN_BUFFER_LITERAL("fixture://taken.geojson"), NULL,
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "source already exists",
    mln_map_add_image_source_url(
      map, MLN_BUFFER_LITERAL("taken"), corners, 4,
      MLN_BUFFER_LITERAL("fixture://image.png"), &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not an image source",
    mln_map_set_image_source_url(
      map, MLN_BUFFER_LITERAL("taken"),
      MLN_BUFFER_LITERAL("fixture://image.png"), &completion.descriptor, NULL
    )
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
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Tile data goes only to a tile inside its zoom level's grid, and only to a
// custom source of the matching kind that exists.
static void custom_tile_data_needs_a_real_tile_and_source(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  mln_custom_geometry_source_options options =
    mln_custom_geometry_source_options_default();
  options.fetch_tile = fetch_nothing;
  MLN_TEST_AWAIT_OK(mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL("geometry"), &options, &completion.descriptor, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "within zoom bounds",
    mln_map_set_custom_geometry_source_tile_data(
      map, MLN_BUFFER_LITERAL("geometry"),
      (mln_canonical_tile_id){.z = 1, .x = 2, .y = 0},
      MLN_BUFFER_LITERAL("{\"type\":\"FeatureCollection\",\"features\":[]}"),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "does not exist",
    mln_map_set_custom_mvt_vector_source_tile_data(
      map, MLN_BUFFER_LITERAL("missing"),
      (mln_canonical_tile_id){.z = 0, .x = 0, .y = 0}, MLN_BUFFER_LITERAL(""),
      &completion.descriptor, NULL
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(source_commands_fail_on_a_taken_missing_or_wrong_target);
  RUN_TEST(custom_tile_data_needs_a_real_tile_and_source);
}
