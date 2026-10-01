// Prepared GeoJSON options: the clustering fields decide which points a
// rendered source holds. The validation of each field is in source_options.c.

#include "support/style.h"
#include "support/test_support.h"

// Three points close enough to cluster at zoom 0 under a 60-pixel radius.
static const char nearby_points[] =
  "{\"type\":\"FeatureCollection\",\"features\":["
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}},"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0.001,0.001]},\"properties\":{}},"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0.002,0.002]},\"properties\":{}}]}";

// Sets every field to a value other than its default.
static mln_geojson_source_options every_field(void) {
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields =
    MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM | MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM |
    MLN_GEOJSON_SOURCE_OPTION_TOLERANCE |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM |
    MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE | MLN_GEOJSON_SOURCE_OPTION_BUFFER |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS |
    MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS | MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  options.min_zoom = 0.0;
  options.max_zoom = 20.0;
  options.tolerance = 0.5;
  options.cluster_max_zoom = 20.0;
  options.tile_size = 256;
  options.buffer = 64;
  options.cluster_radius = 60;
  options.cluster_min_points = 2;
  options.line_metrics = true;
  options.cluster = true;
  return options;
}

typedef struct cluster_query {
  const mln_test_render_fixture* fixture;
  size_t expected_features;
  const char* expected_text;
  mln_test_feature_list last;
} cluster_query;

static bool source_holds_the_expected_features(void* context) {
  cluster_query* query = context;
  query->last = mln_test_style_query_source(query->fixture, "points", NULL);
  if (
    query->last.status != MLN_STATUS_OK ||
    query->last.count != query->expected_features
  ) {
    return false;
  }
  return query->expected_text == NULL ||
         strstr(query->last.features[0].feature, query->expected_text) != NULL;
}

// Renders `points` with `options` and waits until the source holds the
// expected features.
static void render_points(
  const mln_geojson_source_options* options, size_t expected_features,
  const char* expected_text
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map,
    MLN_BUFFER_LITERAL("{\"version\":8,\"sources\":{},\"layers\":[]}")
  );
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_geojson_source_data_create(
    MLN_BUFFER_LITERAL(nearby_points), options, &data, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_data(
    map, MLN_BUFFER_LITERAL("points"), data, &completion.descriptor, NULL
  ));
  mln_geojson_source_data_destroy(data);
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map,
    MLN_BUFFER_LITERAL(
      "{\"id\":\"dots\",\"type\":\"circle\",\"source\":\"points\"}"
    ),
    MLN_BUFFER_LITERAL(""), &completion.descriptor, NULL
  ));

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  cluster_query query = {
    .fixture = &fixture,
    .expected_features = expected_features,
    .expected_text = expected_text,
  };
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, source_holds_the_expected_features, &query, "clustered source"
  ));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void clustering_fields_decide_which_points_the_source_holds(void) {
  // Within the radius, two points are enough for one cluster of all three.
  const mln_geojson_source_options clustered = every_field();
  render_points(&clustered, 1, "\"point_count\":3");

  // Requiring four points leaves the three unclustered.
  mln_geojson_source_options sparse = every_field();
  sparse.cluster_min_points = 4;
  render_points(&sparse, 3, NULL);
}

MLN_TEST_GROUP {
  RUN_TEST(clustering_fields_decide_which_points_the_source_holds);
}
