// GeoJSON sources: preparing data and validating its options, installing one
// prepared handle on many sources, matching data to a source's options, the
// clustering fields, and URL data served through a resource provider.

#include "support/style.h"
#include "support/test_support.h"

// One point at the map's center, named so a query can tell datasets apart.
#define POINT_COLLECTION(name)                                           \
  "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\"," \
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"             \
  "\"properties\":{\"name\":\"" name "\"}}]}"

#define EMPTY_COLLECTION "{\"type\":\"FeatureCollection\",\"features\":[]}"

static mln_geojson_source_data prepare(
  const char* json, const mln_geojson_source_options* options
) {
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_geojson_source_data_create(
    mln_test_view_of(json), options, &data, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, data);
  return data;
}

// Adds a circle layer that draws `source`, so the source's tiles render.
static void draw_source(mln_map map, const char* source) {
  char layer[160];
  snprintf(
    layer, sizeof(layer),
    "{\"id\":\"%s-dots\",\"type\":\"circle\",\"source\":\"%s\"}", source, source
  );
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, mln_test_view_of(layer), MLN_BUFFER_LITERAL(""),
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

static void defaults(mln_geojson_source_options* options) { (void)options; }

static void clustered(mln_geojson_source_options* options) {
  options->fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  options->cluster = true;
}

// Every option field, each set to a value in range.
static void every_option(mln_geojson_source_options* options) {
  options->fields =
    MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM | MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM |
    MLN_GEOJSON_SOURCE_OPTION_TOLERANCE |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES |
    MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE | MLN_GEOJSON_SOURCE_OPTION_BUFFER |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS |
    MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS | MLN_GEOJSON_SOURCE_OPTION_CLUSTER |
    MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
  options->min_zoom = 2.0;
  options->max_zoom = 14.0;
  options->tolerance = 0.5;
  options->cluster_max_zoom = 12.0;
  options->cluster_properties =
    MLN_BUFFER_LITERAL("{\"total\":[\"+\",[\"get\",\"weight\"]]}");
  options->tile_size = 256;
  options->buffer = 64;
  options->cluster_radius = 40;
  options->cluster_min_points = 3;
  options->line_metrics = false;
  options->cluster = true;
  options->synchronous_tiling = true;
}

static void undersized(mln_geojson_source_options* options) {
  options->size = sizeof(uint32_t);
}
static void empty_cluster_properties(mln_geojson_source_options* options) {
  every_option(options);
  options->cluster_properties = MLN_BUFFER_LITERAL("");
}
static void cluster_properties_not_json(mln_geojson_source_options* options) {
  every_option(options);
  options->cluster_properties = MLN_BUFFER_LITERAL("{\"total\":NaN}");
}
// Cluster properties are expressions, so a JSON object of plain values fails
// MapLibre Native's conversion.
static void plain_cluster_properties(mln_geojson_source_options* options) {
  every_option(options);
  options->cluster_properties = MLN_BUFFER_LITERAL("{\"total\":5}");
}
static void cluster_properties_array(mln_geojson_source_options* options) {
  every_option(options);
  options->cluster_properties = MLN_BUFFER_LITERAL("[]");
}
static void inverted_zoom_range(mln_geojson_source_options* options) {
  every_option(options);
  options->min_zoom = 10.0;
  options->max_zoom = 4.0;
}
static void fractional_max_zoom(mln_geojson_source_options* options) {
  every_option(options);
  options->max_zoom = 2.5;
}
static void negative_tolerance(mln_geojson_source_options* options) {
  every_option(options);
  options->tolerance = -1.0;
}
static void cluster_zoom_out_of_range(mln_geojson_source_options* options) {
  every_option(options);
  options->cluster_max_zoom = 300.0;
}
static void fractional_cluster_zoom(mln_geojson_source_options* options) {
  every_option(options);
  options->cluster_max_zoom = 1.5;
}
static void empty_tiles(mln_geojson_source_options* options) {
  every_option(options);
  options->tile_size = 0;
}
static void oversized_buffer(mln_geojson_source_options* options) {
  every_option(options);
  options->buffer = 70000;
}
static void oversized_cluster_radius(mln_geojson_source_options* options) {
  every_option(options);
  options->cluster_radius = 70000;
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

// One document and options that preparation must refuse, and the fragments its
// diagnostic must contain.
typedef struct geojson_data_case {
  const char* label;
  const char* data;
  // Bytes of `data` to pass, or 0 for its length.
  size_t size;
  // Edits default options, or null to pass null options.
  void (*options)(mln_geojson_source_options* options);
  const char* fragments[3];
} geojson_data_case;

static const geojson_data_case geojson_data_cases[] = {
  {"an unknown geometry type",
   "{\"type\":\"Unsupported\",\"coordinates\":[]}",
   0,
   defaults,
   {"is invalid"}},
  {"bytes after the document", trailing_bytes, 0, NULL, {NULL}},
  {"an embedded NUL", embedded_nul, sizeof(embedded_nul) - 1, NULL, {NULL}},
  {"undersized options", EMPTY_COLLECTION, 0, undersized, {NULL}},
  {"empty cluster properties",
   POINT_COLLECTION("a"),
   0,
   empty_cluster_properties,
   {"must not be empty"}},
  {"cluster properties that are not JSON",
   POINT_COLLECTION("a"),
   0,
   cluster_properties_not_json,
   {"cluster_properties"}},
  {"cluster properties of plain values",
   EMPTY_COLLECTION,
   0,
   plain_cluster_properties,
   {"GeoJSON source options"}},
  {"cluster properties that are not an object",
   POINT_COLLECTION("a"),
   0,
   cluster_properties_array,
   {"cluster_properties must contain a JSON object"}},
  {"inverted zoom range",
   POINT_COLLECTION("a"),
   0,
   inverted_zoom_range,
   {"min_zoom must be less than or equal to max_zoom"}},
  {"fractional max zoom",
   POINT_COLLECTION("a"),
   0,
   fractional_max_zoom,
   {"max_zoom"}},
  {"negative tolerance",
   POINT_COLLECTION("a"),
   0,
   negative_tolerance,
   {"tolerance"}},
  {"cluster zoom out of range",
   POINT_COLLECTION("a"),
   0,
   cluster_zoom_out_of_range,
   {"cluster_max_zoom"}},
  {"fractional cluster zoom",
   POINT_COLLECTION("a"),
   0,
   fractional_cluster_zoom,
   {"cluster_max_zoom"}},
  {"empty tiles", POINT_COLLECTION("a"), 0, empty_tiles, {"tile_size"}},
  {"oversized buffer", POINT_COLLECTION("a"), 0, oversized_buffer, {"buffer"}},
  {"oversized cluster radius",
   POINT_COLLECTION("a"),
   0,
   oversized_cluster_radius,
   {"cluster_radius"}},
  // Supercluster reads every feature geometry as a point, and MapLibre Native
  // clusters feature collections only, so clustering refuses anything else up
  // front, naming the feature and the constraint.
  {"a clustered line string",
   "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
   "\"geometry\":{\"type\":\"LineString\",\"coordinates\":[[0,0],[1,1]]},"
   "\"properties\":{}}]}",
   0,
   clustered,
   {"line string"}},
  {"a clustered geometry collection",
   point_and_collection,
   0,
   clustered,
   {"point geometry on every feature", "feature 1", "geometry collection"}},
  {"a clustered bare geometry",
   bare_point,
   0,
   clustered,
   {"requires a feature collection", "a bare geometry"}},
  {"a clustered single feature",
   single_feature,
   0,
   clustered,
   {"a single feature"}},
};

// Preparation is synchronous and touches no runtime or map, so validation
// reports through status and the call's diagnostic, and leaves the output
// handle null.
static void geojson_data_preparation_validates_documents_and_options(void) {
  for (size_t index = 0;
       index < sizeof(geojson_data_cases) / sizeof(geojson_data_cases[0]);
       index += 1) {
    const geojson_data_case* row = &geojson_data_cases[index];
    mln_geojson_source_options options = mln_geojson_source_options_default();
    if (row->options != NULL) {
      row->options(&options);
    }
    const size_t size = row->size == 0 ? strlen(row->data) : row->size;
    mln_geojson_source_data data = MLN_HANDLE_NULL;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_geojson_source_data_create(
        mln_test_buffer_view(row->data, size),
        row->options == NULL ? NULL : &options, &data, MLN_TEST_DIAGNOSTIC
      ),
      row->label
    );
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(MLN_HANDLE_NULL, data, row->label);
    for (size_t fragment = 0; fragment < 3; fragment += 1) {
      if (row->fragments[fragment] != NULL) {
        TEST_ASSERT_NOT_NULL_MESSAGE(
          strstr(mln_test_last_error(), row->fragments[fragment]), row->label
        );
      }
    }
  }

  // A populated output handle is rejected rather than silently overwritten.
  mln_geojson_source_data populated = prepare(EMPTY_COLLECTION, NULL);
  mln_geojson_source_data reused = populated;
  MLN_TEST_INVALID(mln_geojson_source_data_create(
    MLN_BUFFER_LITERAL(EMPTY_COLLECTION), NULL, &reused, NULL
  ));
  mln_geojson_source_data_destroy(populated);

  // Every field in range prepares. The clustering constraints belong to
  // clustering alone, so the same data tiles fine without it, and an empty
  // collection carries nothing to cluster, so a later update supplies them.
  mln_geojson_source_options options = mln_geojson_source_options_default();
  every_option(&options);
  mln_geojson_source_data_destroy(prepare(POINT_COLLECTION("a"), &options));
  mln_geojson_source_data_destroy(prepare(point_and_collection, NULL));
  mln_geojson_source_data_destroy(prepare(bare_point, NULL));
  options = mln_geojson_source_options_default();
  clustered(&options);
  mln_geojson_source_data_destroy(prepare(EMPTY_COLLECTION, &options));
}

// Data prepared with every field installs on a source, and a URL source takes
// the same options. A URL source checks the same fields: the numeric ones when
// the call submits, and the cluster properties in the command, which parses
// them.
static void geojson_sources_take_and_validate_their_options(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_style_serve(runtime, NULL, 0);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  const mln_buffer_view url = MLN_BUFFER_LITERAL("fixture://points.geojson");

  mln_geojson_source_options options = mln_geojson_source_options_default();
  every_option(&options);
  mln_geojson_source_data data = prepare(POINT_COLLECTION("a"), &options);
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_data(
    map, MLN_BUFFER_LITERAL("prepared"), data, &completion.descriptor, NULL
  ));
  mln_geojson_source_data_destroy(data);
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_url(
    map, MLN_BUFFER_LITERAL("points"), url, &options, &completion.descriptor,
    NULL
  ));

  options = mln_geojson_source_options_default();
  inverted_zoom_range(&options);
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "min_zoom must be less than or equal to max_zoom",
    mln_map_add_geojson_source_url(
      map, MLN_BUFFER_LITERAL("inverted"), url, &options,
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  options = mln_geojson_source_options_default();
  cluster_properties_array(&options);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT,
    "cluster_properties must contain a JSON object",
    mln_map_add_geojson_source_url(
      map, MLN_BUFFER_LITERAL("array-properties"), url, &options,
      &completion.descriptor, NULL
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The submit-time lease keeps the prepared index alive, so one handle serves
// any number of sources, each of which keeps its own reference after the host
// destroys the handle. A destroyed handle rejects new installs at submission.
static void one_prepared_handle_serves_many_sources_and_outlives_itself(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  static const char* const sources[] = {"first", "second", "third"};
  mln_geojson_source_data data = prepare(POINT_COLLECTION("shared"), NULL);
  for (size_t index = 0; index < 3; index += 1) {
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_OK(mln_map_add_geojson_source_data(
      map, mln_test_view_of(sources[index]), data, &completion.descriptor, NULL
    ));
    // The last install is still pending when the handle goes away.
    if (index == 2) {
      mln_geojson_source_data_destroy(data);
    }
    MLN_TEST_OK(mln_test_completion_settle(&completion));
    draw_source(map, sources[index]);
  }

  // A second destroy is a no-op, and the dead handle installs nowhere.
  mln_geojson_source_data_destroy(data);
  mln_geojson_source_data_destroy(MLN_HANDLE_NULL);
  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID_STATE(mln_map_set_geojson_source_data(
    map, MLN_BUFFER_LITERAL("first"), data, &rejected, NULL
  ));
  MLN_TEST_INVALID_STATE(mln_map_add_geojson_source_data(
    map, MLN_BUFFER_LITERAL("fourth"), data, &rejected, NULL
  ));

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  source_query query = {
    .fixture = &fixture,
    .sources = sources,
    .source_count = 3,
    .count = 1,
    .text = "\"name\":\"shared\"",
  };
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, sources_hold_the_features, &query, "every source's feature"
  ));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// One cluster_properties variant and whether data prepared with it matches a
// source added with `{"total":["+",["get","rank"]],"top":["max",...]}`.
typedef struct cluster_match_case {
  const char* label;
  const char* cluster_properties;
  mln_status expected;
} cluster_match_case;

// One option field that data can carry and a source compares, with a change
// that moves it off its default.
typedef struct geojson_field_case {
  uint32_t field;
  void (*change)(mln_geojson_source_options* options);
} geojson_field_case;

static void change_cluster_max_zoom(mln_geojson_source_options* options) {
  options->cluster_max_zoom = 10.0;
}
static void change_cluster_radius(mln_geojson_source_options* options) {
  options->cluster_radius = 120;
}
static void change_cluster_min_points(mln_geojson_source_options* options) {
  options->cluster_min_points = 5;
}
static void change_buffer(mln_geojson_source_options* options) {
  options->buffer = 64;
}
static void change_synchronous_tiling(mln_geojson_source_options* options) {
  options->synchronous_tiling = true;
}

// MapLibre Native fixes a source's options when it is added, so installed data
// must carry equal options. Cluster aggregations compare as parsed
// expressions, not as JSON text.
static void prepared_data_must_match_the_source_options(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  static const char points[] = POINT_COLLECTION("clustered");
  const mln_buffer_view id = MLN_BUFFER_LITERAL("clustered");

  mln_geojson_source_options source = mln_geojson_source_options_default();
  clustered(&source);
  source.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  source.cluster_properties = MLN_BUFFER_LITERAL(
    "{\"total\":[\"+\",[\"get\",\"rank\"]],\"top\":[\"max\",[\"get\","
    "\"rank\"]]}"
  );
  mln_geojson_source_data data = prepare(points, &source);
  MLN_TEST_AWAIT_OK(
    mln_map_add_geojson_source_data(map, id, data, &completion.descriptor, NULL)
  );
  mln_geojson_source_data_destroy(data);

  // Data prepared without clustering tiles inconsistently with the source.
  data = prepare(points, NULL);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "do not match",
    mln_map_set_geojson_source_data(map, id, data, &completion.descriptor, NULL)
  );
  mln_geojson_source_data_destroy(data);

  static const cluster_match_case cases[] = {
    {"the same JSON", NULL, MLN_STATUS_OK},
    {"reformatted whitespace",
     " { \"total\" : [ \"+\" , [ \"get\" , \"rank\" ] ] , \"top\" : [ "
     "\"max\" , [ \"get\" , \"rank\" ] ] } ",
     MLN_STATUS_OK},
    {"members in the other order",
     "{\"top\":[\"max\",[\"get\",\"rank\"]],\"total\":[\"+\",[\"get\","
     "\"rank\"]]}",
     MLN_STATUS_OK},
    {"one operator swapped",
     "{\"total\":[\"max\",[\"get\",\"rank\"]],\"top\":[\"max\",[\"get\","
     "\"rank\"]]}",
     MLN_STATUS_INVALID_ARGUMENT},
    {"one input swapped",
     "{\"total\":[\"+\",[\"get\",\"score\"]],\"top\":[\"max\",[\"get\","
     "\"rank\"]]}",
     MLN_STATUS_INVALID_ARGUMENT},
    {"one member renamed",
     "{\"sum\":[\"+\",[\"get\",\"rank\"]],\"top\":[\"max\",[\"get\","
     "\"rank\"]]}",
     MLN_STATUS_INVALID_ARGUMENT},
    {"one member dropped", "{\"total\":[\"+\",[\"get\",\"rank\"]]}",
     MLN_STATUS_INVALID_ARGUMENT},
  };
  for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]); index += 1) {
    mln_geojson_source_options variant = source;
    if (cases[index].cluster_properties != NULL) {
      variant.cluster_properties =
        mln_test_view_of(cases[index].cluster_properties);
    }
    data = prepare(points, &variant);
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_OK(mln_map_set_geojson_source_data(
      map, id, data, &completion.descriptor, NULL
    ));
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      cases[index].expected, mln_test_completion_settle(&completion),
      cases[index].label
    );
    mln_geojson_source_data_destroy(data);
  }

  // Every other option is compared too, so data that differs from the source
  // in any one field does not match it.
  static const geojson_field_case fields[] = {
    {MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM, change_cluster_max_zoom},
    {MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS, change_cluster_radius},
    {MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS, change_cluster_min_points},
    {MLN_GEOJSON_SOURCE_OPTION_BUFFER, change_buffer},
    {MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING, change_synchronous_tiling},
  };
  for (size_t index = 0; index < sizeof(fields) / sizeof(fields[0]);
       index += 1) {
    mln_geojson_source_options variant = source;
    variant.fields |= fields[index].field;
    fields[index].change(&variant);
    data = prepare(points, &variant);
    MLN_TEST_EXPECT_COMMAND_FAILED(
      MLN_STATUS_INVALID_ARGUMENT, "do not match",
      mln_map_set_geojson_source_data(
        map, id, data, &completion.descriptor, NULL
      )
    );
    mln_geojson_source_data_destroy(data);
  }

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Three points close enough to cluster at zoom 0 under a 60-pixel radius.
static const char nearby_points[] =
  "{\"type\":\"FeatureCollection\",\"features\":["
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0,0]},\"properties\":{}},"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0.001,0.001]},\"properties\":{}},"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
  "\"coordinates\":[0.002,0.002]},\"properties\":{}}]}";

// Renders the nearby points with `options` and waits until the source holds
// `count` features, the first of which contains `text` when it is non-null.
static void render_nearby_points(
  const mln_geojson_source_options* options, size_t count, const char* text
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  mln_geojson_source_data data = prepare(nearby_points, options);
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_data(
    map, MLN_BUFFER_LITERAL("points"), data, &completion.descriptor, NULL
  ));
  mln_geojson_source_data_destroy(data);
  draw_source(map, "points");

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
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

// The clustering fields decide which points a rendered source holds: within
// the radius, two points are enough for one cluster of all three, and
// requiring four leaves the three unclustered.
static void clustering_fields_decide_which_points_the_source_holds(void) {
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
  render_nearby_points(&options, 1, "\"point_count\":3");
  options.cluster_min_points = 4;
  render_nearby_points(&options, 3, NULL);
}

// A URL source loads its data through the runtime's resource provider when it
// is added, and again from each URL it is set to.
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
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_url(
    map, MLN_BUFFER_LITERAL("points"),
    MLN_BUFFER_LITERAL("fixture://first.geojson"), NULL, &completion.descriptor,
    NULL
  ));
  draw_source(map, "points");

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
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

  MLN_TEST_AWAIT_OK(mln_map_set_geojson_source_url(
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
  RUN_TEST(geojson_data_preparation_validates_documents_and_options);
  RUN_TEST(geojson_sources_take_and_validate_their_options);
  RUN_TEST(one_prepared_handle_serves_many_sources_and_outlives_itself);
  RUN_TEST(prepared_data_must_match_the_source_options);
  RUN_TEST(clustering_fields_decide_which_points_the_source_holds);
  RUN_TEST(a_url_source_loads_through_the_resource_provider);
}
