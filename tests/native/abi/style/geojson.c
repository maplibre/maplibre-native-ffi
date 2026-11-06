// GeoJSON sources, their options, and the data they hold.

#include "support/style.h"
#include "support/test_support.h"

// One point at the map's center, named so a query can tell datasets apart.
#define POINT_COLLECTION(name)                                           \
  "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\"," \
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"             \
  "\"properties\":{\"name\":\"" name "\"}}]}"

#define EMPTY_COLLECTION "{\"type\":\"FeatureCollection\",\"features\":[]}"

// Adds a GeoJSON source named `id` with `options`, or the defaults when null.
static void add_source(
  mln_map map, const char* id, const mln_geo_json_source_options* options
) {
  MLN_TEST_AWAIT_OK(mln_map_add_geo_json_source(
    map, mln_buffer_view_from_cstr(id), options, MLN_BUFFER_LITERAL(""),
    &completion.descriptor, NULL
  ));
}

static void set_data(mln_map map, const char* id, const char* json) {
  MLN_TEST_AWAIT_OK(mln_map_set_geo_json_source_data(
    map, mln_buffer_view_from_cstr(id), mln_buffer_view_from_cstr(json),
    &completion.descriptor, NULL
  ));
}

// Adds a circle layer that draws `source`, so the source's tiles render.
static void draw_source(mln_map map, const char* source) {
  char layer[160];
  snprintf(
    layer, sizeof(layer),
    "{\"id\":\"%s-dots\",\"type\":\"circle\",\"source\":\"%s\"}", source, source
  );
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, mln_buffer_view_from_cstr(layer), MLN_BUFFER_LITERAL(""),
    &completion.descriptor, NULL
  ));
}

typedef struct source_query {
  const mln_test_render_fixture* fixture;
  const char* const* sources;
  size_t source_count;
  // The features each source must hold, and text the first must contain.
  size_t count;
  const char* text;
} source_query;

static bool sources_hold_the_features(void* context) {
  const source_query* query = context;
  for (size_t index = 0; index < query->source_count; index += 1) {
    const mln_test_feature_list list =
      mln_test_style_query_source(query->fixture, query->sources[index], NULL);
    if (
      list.status != MLN_STATUS_OK || list.count != query->count ||
      (query->text != NULL &&
       strstr(list.features[0].feature, query->text) == NULL)
    ) {
      return false;
    }
  }
  return true;
}

static void clustered(mln_geo_json_source_options* options) {
  options->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER;
  options->cluster = true;
}

// Every option field, each set to a value in range.
static void every_option(mln_geo_json_source_options* options) {
  options->fields = MLN_GEO_JSON_SOURCE_OPTION_MIN_ZOOM |
                    MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM |
                    MLN_GEO_JSON_SOURCE_OPTION_TOLERANCE |
                    MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM |
                    MLN_GEO_JSON_SOURCE_OPTION_BUFFER |
                    MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS |
                    MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS |
                    MLN_GEO_JSON_SOURCE_OPTION_LINE_METRICS |
                    MLN_GEO_JSON_SOURCE_OPTION_CLUSTER;
  options->min_zoom = 2.0;
  options->max_zoom = 14.0;
  options->tolerance = 0.5;
  options->cluster_max_zoom = 12.0;
  options->cluster_properties =
    MLN_BUFFER_LITERAL("{\"total\":[\"+\",[\"get\",\"weight\"]]}");
  options->buffer = 64;
  options->cluster_radius = 40;
  options->cluster_min_points = 3;
  options->line_metrics = false;
  options->cluster = true;
}

static void undersized(mln_geo_json_source_options* options) {
  options->size = sizeof(uint32_t);
}
static void cluster_properties_not_json(mln_geo_json_source_options* options) {
  every_option(options);
  options->cluster_properties = MLN_BUFFER_LITERAL("{\"total\":NaN}");
}
// Cluster properties are expressions, so a JSON object of plain values fails
// MapLibre Native's conversion.
static void plain_cluster_properties(mln_geo_json_source_options* options) {
  every_option(options);
  options->cluster_properties = MLN_BUFFER_LITERAL("{\"total\":5}");
}
static void cluster_properties_array(mln_geo_json_source_options* options) {
  every_option(options);
  options->cluster_properties = MLN_BUFFER_LITERAL("[]");
}
static void inverted_zoom_range(mln_geo_json_source_options* options) {
  every_option(options);
  options->min_zoom = 10.0;
  options->max_zoom = 4.0;
}
static void fractional_max_zoom(mln_geo_json_source_options* options) {
  every_option(options);
  options->max_zoom = 2.5;
}
static void negative_tolerance(mln_geo_json_source_options* options) {
  every_option(options);
  options->tolerance = -1.0;
}
static void cluster_zoom_out_of_range(mln_geo_json_source_options* options) {
  every_option(options);
  options->cluster_max_zoom = 300.0;
}
static void cluster_zoom_past_cluster_ids(
  mln_geo_json_source_options* options
) {
  every_option(options);
  options->cluster_max_zoom = 31.0;
}

static const char trailing_bytes[] = EMPTY_COLLECTION "garbage";
static const char embedded_nul[] = EMPTY_COLLECTION "\0garbage";
static const char point_and_collection[] =
  "{\"type\":\"FeatureCollection\",\"features\":["
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[-122.5,37.7]},\"properties\":{}},"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"GeometryCollection\","
  "\"geometries\":[{\"type\":\"Point\",\"coordinates\":[-122.4,37.8]}]},"
  "\"properties\":{}}]}";
static const char bare_point[] =
  "{\"type\":\"Point\",\"coordinates\":[-122.5,37.7]}";
static const char single_feature[] =
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[-122.5,37.7]},\"properties\":{}}";

// Options that adding a source must refuse, and a fragment of the diagnostic.
typedef struct geojson_options_case {
  const char* label;
  void (*options)(mln_geo_json_source_options* options);
  const char* fragment;
} geojson_options_case;

static void max_zoom_past_tiling(mln_geo_json_source_options* options) {
  options->fields = MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM;
  options->max_zoom = 25.0;
}
static void buffer_too_wide(mln_geo_json_source_options* options) {
  options->fields = MLN_GEO_JSON_SOURCE_OPTION_BUFFER;
  options->buffer = 513;
}
static void cluster_radius_too_wide(mln_geo_json_source_options* options) {
  clustered(options);
  options->fields |= MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS;
  options->cluster_radius = 4096;
}
static void unknown_field(mln_geo_json_source_options* options) {
  options->fields = 1U << 31U;
}

static const geojson_options_case geojson_options_cases[] = {
  {"undersized options", undersized, "size is too small"},
  {"an unknown field", unknown_field, "unknown bits"},
  {"cluster properties that are not JSON", cluster_properties_not_json,
   "cluster_properties"},
  {"cluster properties of plain values", plain_cluster_properties,
   "GeoJSON source options"},
  {"cluster properties that are not an object", cluster_properties_array,
   "cluster_properties must contain a JSON object"},
  {"inverted zoom range", inverted_zoom_range,
   "min_zoom must be less than or equal to max_zoom"},
  {"fractional max zoom", fractional_max_zoom, "max_zoom"},
  {"max zoom past the tiling limit", max_zoom_past_tiling,
   "max_zoom must be an integer within [0, 24]"},
  {"negative tolerance", negative_tolerance, "tolerance"},
  {"cluster zoom out of range", cluster_zoom_out_of_range, "cluster_max_zoom"},
  // A cluster ID holds its zoom level plus one in five bits.
  {"cluster zoom past what cluster IDs hold", cluster_zoom_past_cluster_ids,
   "cluster_max_zoom must be an integer within [0, 30]"},
  {"a buffer past 512", buffer_too_wide, "buffer must be at most 512"},
  {"a cluster radius past 4095", cluster_radius_too_wide,
   "cluster_radius must be at most 4095"},
};

static void adding_a_source_validates_its_options(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  for (size_t index = 0;
       index < sizeof(geojson_options_cases) / sizeof(geojson_options_cases[0]);
       index += 1) {
    const geojson_options_case* row = &geojson_options_cases[index];
    mln_geo_json_source_options options = {
      .size = sizeof(mln_geo_json_source_options)
    };
    row->options(&options);
    MLN_TEST_EXPECT_COMMAND_REJECTED(
      row->fragment,
      mln_map_add_geo_json_source(
        map, MLN_BUFFER_LITERAL("rejected"), &options, MLN_BUFFER_LITERAL(""),
        &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
  }

  // Every limit is inclusive.
  mln_geo_json_source_options options = {
    .size = sizeof(mln_geo_json_source_options)
  };
  every_option(&options);
  options.fields |= MLN_GEO_JSON_SOURCE_OPTION_MAX_ZOOM;
  options.max_zoom = 24.0;
  options.buffer = 512;
  options.cluster_radius = 4095;
  options.cluster_max_zoom = 30.0;
  add_source(map, "limits", &options);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The engine counts cluster points in a size_t, so a 32-bit target rejects a
// count past 32 bits instead of truncating it.
#if SIZE_MAX <= UINT32_MAX
static void cluster_min_points_must_fit_a_32_bit_target(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_styled_map(runtime);
  mln_geo_json_source_options options = {
    .size = sizeof(mln_geo_json_source_options)
  };
  options.fields = MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
  options.cluster_min_points = UINT32_MAX;
  add_source(map, "fits", &options);

  options.cluster_min_points = (uint64_t)UINT32_MAX + 1U;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "cluster_min_points must be at most 4294967295",
    mln_map_add_geo_json_source(
      map, MLN_BUFFER_LITERAL("too-many"), &options, MLN_BUFFER_LITERAL(""),
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

// Data that a source must refuse, and fragments of the diagnostic.
typedef struct geojson_data_case {
  const char* label;
  const char* source;
  const char* data;
  // Bytes of `data` to pass, or 0 for its length.
  size_t size;
  const char* fragments[3];
} geojson_data_case;

// Supercluster reads every feature geometry as a point, and MapLibre Native
// clusters feature collections only, so a clustered source refuses anything
// else up front, naming the feature and the constraint.
static const geojson_data_case geojson_data_cases[] = {
  {"bytes after the document", "plain", trailing_bytes, 0, {"not valid JSON"}},
  {"an embedded NUL", "plain", embedded_nul, sizeof(embedded_nul) - 1, {NULL}},
  {"a clustered line string",
   "clustered",
   "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
   "\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]},"
   "\"properties\":{}}]}",
   0,
   {"line string"}},
  {"a clustered geometry collection",
   "clustered",
   point_and_collection,
   0,
   {"point geometry on every feature", "feature 1", "geometry collection"}},
  {"a clustered bare geometry",
   "clustered",
   bare_point,
   0,
   {"requires a feature collection", "a bare geometry"}},
  {"a clustered single feature",
   "clustered",
   single_feature,
   0,
   {"a single feature"}},
};

// A source checks new data against its own options, and keeps the data it
// holds when it refuses new data.
static void a_source_validates_data_with_its_options(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  add_source(map, "plain", NULL);
  mln_geo_json_source_options options = {
    .size = sizeof(mln_geo_json_source_options)
  };
  clustered(&options);
  add_source(map, "clustered", &options);
  set_data(map, "plain", POINT_COLLECTION("kept"));
  set_data(map, "clustered", POINT_COLLECTION("kept"));

  for (size_t index = 0;
       index < sizeof(geojson_data_cases) / sizeof(geojson_data_cases[0]);
       index += 1) {
    const geojson_data_case* row = &geojson_data_cases[index];
    const size_t size = row->size == 0 ? strlen(row->data) : row->size;
    mln_test_completion completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_OK,
      mln_map_set_geo_json_source_data(
        map, mln_buffer_view_from_cstr(row->source),
        mln_test_buffer_view(row->data, size), &completion.descriptor, NULL
      ),
      row->label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT, mln_test_completion_finish(&completion),
      row->label
    );
    for (size_t fragment = 0; fragment < 3; fragment += 1) {
      if (row->fragments[fragment] != NULL) {
        TEST_ASSERT_NOT_NULL_MESSAGE(
          strstr(
            mln_test_completion_diagnostic(&completion),
            row->fragments[fragment]
          ),
          row->label
        );
      }
    }
    mln_test_completion_destroy(&completion);
  }

  // The clustering constraints belong to clustering alone, so the same data
  // tiles fine without it, and an empty collection carries nothing to cluster.
  set_data(map, "plain", point_and_collection);
  set_data(map, "plain", bare_point);
  set_data(map, "plain", single_feature);
  set_data(map, "clustered", EMPTY_COLLECTION);
  set_data(map, "clustered", POINT_COLLECTION("kept"));

  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "data", mln_map_set_geo_json_source_data(
              map, MLN_BUFFER_LITERAL("plain"), MLN_BUFFER_LITERAL(""),
              &completion.descriptor, MLN_TEST_DIAGNOSTIC
            )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_NOT_FOUND, "does not exist",
    mln_map_set_geo_json_source_data(
      map, MLN_BUFFER_LITERAL("missing"), MLN_BUFFER_LITERAL(EMPTY_COLLECTION),
      &completion.descriptor, NULL
    )
  );
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("raster"),
    MLN_BUFFER_LITERAL(
      "{\"type\":\"raster\",\"tiles\":[\"fixture://{z}/{x}/{y}.png\"]}"
    ),
    &completion.descriptor, NULL
  ));
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "is not a GeoJSON source",
    mln_map_set_geo_json_source_data(
      map, MLN_BUFFER_LITERAL("raster"), MLN_BUFFER_LITERAL(EMPTY_COLLECTION),
      &completion.descriptor, NULL
    )
  );

  mln_test_render_fixture fixture = {0};
  draw_source(map, "plain");
  draw_source(map, "clustered");
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  static const char* const sources[] = {"clustered"};
  source_query query = {
    .fixture = &fixture,
    .sources = sources,
    .source_count = 1,
    .count = 1,
    .text = "\"name\":\"kept\"",
  };
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, sources_hold_the_features, &query, "the kept feature"
  ));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Data set on one source after another shows the newest data, whichever
// earlier data the source skipped.
static void a_source_shows_the_newest_data(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  add_source(map, "points", NULL);
  draw_source(map, "points");

  mln_test_completion first = mln_test_completion_default(0);
  mln_test_completion middle = mln_test_completion_default(0);
  mln_test_completion last = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_set_geo_json_source_data(
    map, MLN_BUFFER_LITERAL("points"),
    MLN_BUFFER_LITERAL(POINT_COLLECTION("first")), &first.descriptor, NULL
  ));
  MLN_TEST_OK(mln_map_set_geo_json_source_data(
    map, MLN_BUFFER_LITERAL("points"),
    MLN_BUFFER_LITERAL(POINT_COLLECTION("middle")), &middle.descriptor, NULL
  ));
  MLN_TEST_OK(mln_map_set_geo_json_source_data(
    map, MLN_BUFFER_LITERAL("points"),
    MLN_BUFFER_LITERAL(POINT_COLLECTION("last")), &last.descriptor, NULL
  ));
  // The first data has nothing to wait behind, and nothing replaces the last.
  // The middle data shows unless the last replaced it while it waited.
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&first));
  const mln_status middle_status = mln_test_completion_settle(&middle);
  TEST_ASSERT_TRUE_MESSAGE(
    middle_status == MLN_STATUS_OK || middle_status == MLN_STATUS_CANCELLED,
    "the middle data shows or is replaced"
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_settle(&last));

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  static const char* const sources[] = {"points"};
  source_query query = {
    .fixture = &fixture,
    .sources = sources,
    .source_count = 1,
    .count = 1,
    .text = "\"name\":\"last\"",
  };
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, sources_hold_the_features, &query, "the last feature"
  ));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Renders the points in a source with `options` and waits until the source
// holds `count` features, the first of which contains `text` when it is
// non-null.
static void render_points(
  const char* points, const mln_geo_json_source_options* options, size_t count,
  const char* text
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  add_source(map, "points", options);
  set_data(map, "points", points);
  draw_source(map, "points");

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  static const char* const sources[] = {"points"};
  source_query query = {
    .fixture = &fixture,
    .sources = sources,
    .source_count = 1,
    .count = count,
    .text = text,
  };
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, sources_hold_the_features, &query, "clustered source"
  ));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Two points 11.25 degrees apart lie 16 pixels apart at zoom 0, because GeoJSON
// sources cluster on a fixed 512-pixel tile: a 15-pixel radius leaves them
// apart and a 17-pixel one joins them, unless the cluster needs more points
// than there are.
static void clustering_fields_decide_which_points_the_source_holds(void) {
  static const char points[] =
    "{\"type\":\"FeatureCollection\",\"features\":["
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[0,0]},\"properties\":{}},"
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[11.25,0]},\"properties\":{}}]}";
  mln_geo_json_source_options options = {
    .size = sizeof(mln_geo_json_source_options)
  };
  options.fields = MLN_GEO_JSON_SOURCE_OPTION_CLUSTER |
                   MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_RADIUS |
                   MLN_GEO_JSON_SOURCE_OPTION_CLUSTER_MIN_POINTS;
  options.cluster = true;
  options.cluster_min_points = 2;
  options.cluster_radius = 15;
  render_points(points, &options, 2, NULL);
  options.cluster_radius = 17;
  render_points(points, &options, 1, "\"point_count\":2");
  options.cluster_min_points = 3;
  render_points(points, &options, 2, NULL);
}

static void a_url_source_loads_through_the_resource_provider(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  // Static, because a failing case leaves the provider running until the
  // harness reclaims the runtime.
  static mln_test_style_route routes[] = {
    {.url = "fixture://first.geojson", .body = POINT_COLLECTION("first")},
    {.url = "fixture://second.geojson", .body = POINT_COLLECTION("second")},
  };
  mln_test_style_serve(runtime, routes, 2);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("points"),
    MLN_BUFFER_LITERAL(
      "{\"type\":\"geojson\",\"data\":\"fixture://first.geojson\"}"
    ),
    &completion.descriptor, NULL
  ));
  draw_source(map, "points");

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  static const char* const sources[] = {"points"};
  source_query query = {
    .fixture = &fixture,
    .sources = sources,
    .source_count = 1,
    .count = 1,
    .text = "\"name\":\"first\"",
  };
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, sources_hold_the_features, &query, "the first URL's feature"
  ));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&routes[0].requests));

  MLN_TEST_AWAIT_OK(mln_map_set_geo_json_source_url(
    map, MLN_BUFFER_LITERAL("points"),
    MLN_BUFFER_LITERAL("fixture://second.geojson"), &completion.descriptor, NULL
  ));
  query.text = "\"name\":\"second\"";
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, sources_hold_the_features, &query, "the second URL's feature"
  ));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&routes[1].requests));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(adding_a_source_validates_its_options);
#if SIZE_MAX <= UINT32_MAX
  RUN_TEST(cluster_min_points_must_fit_a_32_bit_target);
#endif
  RUN_TEST(a_source_validates_data_with_its_options);
  RUN_TEST(a_source_shows_the_newest_data);
  RUN_TEST(clustering_fields_decide_which_points_the_source_holds);
  RUN_TEST(a_url_source_loads_through_the_resource_provider);
}
