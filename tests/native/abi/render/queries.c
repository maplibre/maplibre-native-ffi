// Feature queries against a render session's latest frame: rendered queries
// by point, box, and line string with layer and filter options, source
// queries by source layer, and the supercluster feature extensions.
//
// The map is 64 x 64 at zoom 0 around (0, 0), so a longitude of 10 degrees
// lies about 14 pixels from the center.

#include <math.h>

#include "support/style.h"
#include "support/test_support.h"

// Two points, "west" at x 18 and "east" at x 46 on the center row. The
// "circles" layer draws both and the "halos" layer draws only the east one.
static const char points_style_json[] =
  "{\"version\":8,\"sources\":{\"points\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"FeatureCollection\",\"features\":["
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"west\",\"kind\":\"a\"},"
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[-10,0]}},"
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"east\",\"kind\":\"b\"},"
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[10,0]}}]}}},"
  "\"layers\":[{\"id\":\"circles\",\"type\":\"circle\",\"source\":\"points\","
  "\"paint\":{\"circle-radius\":4}},{\"id\":\"halos\",\"type\":\"circle\","
  "\"source\":\"points\",\"filter\":[\"==\",[\"get\",\"kind\"],\"b\"],"
  "\"paint\":{\"circle-radius\":4}}]}";

typedef struct rendered_count {
  const mln_test_render_fixture* fixture;
  size_t count;
} rendered_count;

static bool renders_count(void* context) {
  const rendered_count* wait = context;
  const mln_test_feature_list list =
    mln_test_style_query_rendered(wait->fixture);
  return list.status == MLN_STATUS_OK && list.count == wait->count;
}

// Loads `style`, attaches a render fixture, and renders until the whole
// viewport holds `rendered` features.
static void render_style(
  mln_runtime runtime, mln_map map, mln_buffer_view style, size_t rendered,
  mln_test_render_fixture* fixture
) {
  mln_test_load_style_and_wait(runtime, map, style);
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, fixture));
  rendered_count wait = {.fixture = fixture, .count = rendered};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    fixture, renders_count, &wait, "the style's features to render"
  ));
}

// Whether the queried feature's properties name `name`.
static bool feature_named(const mln_test_feature* feature, const char* name) {
  char property[64];
  (void)snprintf(property, sizeof(property), "\"name\":\"%s\"", name);
  return strstr(feature->feature, property) != NULL;
}

// Names the points a list holds, in the order west, east, as a bit mask.
enum { WEST = 1U << 0U, EAST = 1U << 1U };

static unsigned int named_points(const mln_test_feature_list* list) {
  unsigned int found = 0;
  for (size_t index = 0; index < list->count; index += 1) {
    found |= feature_named(&list->features[index], "west") ? WEST : 0U;
    found |= feature_named(&list->features[index], "east") ? EAST : 0U;
  }
  return found;
}

static const mln_screen_point crossing_line[] = {
  {.x = 4, .y = 32}, {.x = 60, .y = 32}
};
static const mln_screen_point missing_line[] = {
  {.x = 4, .y = 4}, {.x = 60, .y = 8}
};

typedef struct rendered_row {
  const char* label;
  mln_rendered_query_geometry geometry;
  const char* layer;
  const char* filter;
  size_t count;
  unsigned int points;
} rendered_row;

static void rendered_queries_select_by_geometry_layer_and_filter(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  // Three hits in all: both circles, and the east halo.
  render_style(
    runtime, map, MLN_BUFFER_LITERAL(points_style_json), 3, &fixture
  );

  const rendered_row rows[] = {
    {"a point on the east circle hits it in both layers",
     mln_rendered_query_geometry_point((mln_screen_point){.x = 46, .y = 32}),
     NULL, NULL, 2, EAST},
    {"a point between the circles misses",
     mln_rendered_query_geometry_point((mln_screen_point){.x = 32, .y = 32}),
     NULL, NULL, 0, 0},
    {"a layer ID narrows a hit to that layer",
     mln_rendered_query_geometry_point((mln_screen_point){.x = 46, .y = 32}),
     "circles", NULL, 1, EAST},
    {"a layer whose filter drops the feature misses it",
     mln_rendered_query_geometry_point((mln_screen_point){.x = 18, .y = 32}),
     "halos", NULL, 0, 0},
    {"a filter keeps the matching feature",
     mln_rendered_query_geometry_box(
       (mln_screen_box){.min = {.x = 0, .y = 0}, .max = {.x = 64, .y = 64}}
     ),
     "circles", "[\"==\",[\"get\",\"kind\"],\"a\"]", 1, WEST},
    {"a box past every edge is clipped to the viewport",
     mln_rendered_query_geometry_box((mln_screen_box){
       .min = {.x = -4096, .y = -4096}, .max = {.x = 4096, .y = 4096}
     }),
     "circles", NULL, 2, WEST | EAST},
    {"a box with swapped corners is normalized",
     mln_rendered_query_geometry_box(
       (mln_screen_box){.min = {.x = 64, .y = 40}, .max = {.x = 36, .y = 24}}
     ),
     "circles", NULL, 1, EAST},
    {"a box beyond the viewport finds nothing",
     mln_rendered_query_geometry_box(
       (mln_screen_box){.min = {.x = 100, .y = 0}, .max = {.x = 200, .y = 64}}
     ),
     NULL, NULL, 0, 0},
    {"a line string through both circles hits both",
     mln_rendered_query_geometry_line_string(crossing_line, 2), "circles", NULL,
     2, WEST | EAST},
    {"a line string clear of the circles misses",
     mln_rendered_query_geometry_line_string(missing_line, 2), NULL, NULL, 0,
     0},
  };
  for (size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); index += 1) {
    const rendered_row* row = &rows[index];
    mln_rendered_feature_query_options options =
      mln_rendered_feature_query_options_default();
    const mln_buffer_view layer =
      row->layer == NULL ? (mln_buffer_view){0} : mln_test_view_of(row->layer);
    if (row->layer != NULL) {
      options.fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
      options.layer_ids = &layer;
      options.layer_id_count = 1;
    }
    if (row->filter != NULL) {
      options.fields |= MLN_RENDERED_FEATURE_QUERY_OPTION_FILTER;
      options.filter = mln_test_view_of(row->filter);
    }
    const mln_test_feature_list list =
      mln_test_style_query_rendered_with(&fixture, &row->geometry, &options);
    MLN_TEST_OK_MESSAGE(list.status, row->label);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(row->count, list.count, row->label);
    TEST_ASSERT_EQUAL_UINT_MESSAGE(
      row->points, named_points(&list), row->label
    );
    for (size_t hit = 0; hit < list.count; hit += 1) {
      TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "points", list.features[hit].source_id, row->label
      );
    }
  }

  // A source query with no options reads every feature the source holds.
  const mln_test_feature_list source =
    mln_test_style_query_source_with(&fixture, "points", NULL);
  MLN_TEST_OK(source.status);
  TEST_ASSERT_EQUAL_size_t(2, source.count);
  TEST_ASSERT_EQUAL_UINT(WEST | EAST, named_points(&source));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A rendered or source query, submitted with one of its inputs broken.
typedef struct query_call {
  mln_rendered_query_geometry geometry;
  mln_rendered_feature_query_options rendered;
  mln_source_feature_query_options source;
  mln_buffer_view source_id;
  bool source_query;
} query_call;

static const mln_buffer_view no_bytes_layer = {.data = NULL, .size = 1};
static const mln_buffer_view unparsable_filter = MLN_BUFFER_LITERAL("[\"==\"");

static void undersized_geometry(void* call) {
  ((query_call*)call)->geometry.size = sizeof(mln_rendered_query_geometry) - 1;
}
static void unknown_geometry_type(void* call) {
  ((query_call*)call)->geometry.type = 99;
}
static void non_finite_point(void* call) {
  ((query_call*)call)->geometry.data.point.x = (double)INFINITY;
}
static void empty_line_string(void* call) {
  ((query_call*)call)->geometry =
    mln_rendered_query_geometry_line_string(crossing_line, 0);
}
static void null_line_string_points(void* call) {
  ((query_call*)call)->geometry =
    mln_rendered_query_geometry_line_string(NULL, 2);
}
static void undersized_rendered_options(void* call) {
  ((query_call*)call)->rendered.size =
    sizeof(mln_rendered_feature_query_options) - 1;
}
static void unknown_rendered_field(void* call) {
  ((query_call*)call)->rendered.fields = UINT32_C(1) << 31;
}
static void null_layer_ids(void* call) {
  query_call* query = call;
  query->rendered.fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
  query->rendered.layer_id_count = 1;
}
static void layer_id_without_bytes(void* call) {
  query_call* query = call;
  query->rendered.fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
  query->rendered.layer_ids = &no_bytes_layer;
  query->rendered.layer_id_count = 1;
}
static void unparsable_rendered_filter(void* call) {
  query_call* query = call;
  query->rendered.fields = MLN_RENDERED_FEATURE_QUERY_OPTION_FILTER;
  query->rendered.filter = unparsable_filter;
}
static void source_query(void* call) {
  ((query_call*)call)->source_query = true;
}
static void empty_source_id(void* call) {
  query_call* query = call;
  query->source_query = true;
  query->source_id = (mln_buffer_view){.data = "", .size = 0};
}
static void source_id_without_bytes(void* call) {
  query_call* query = call;
  query->source_query = true;
  query->source_id = (mln_buffer_view){.data = NULL, .size = 1};
}
static void undersized_source_options(void* call) {
  query_call* query = call;
  query->source_query = true;
  query->source.size = sizeof(mln_source_feature_query_options) - 1;
}
static void unknown_source_field(void* call) {
  query_call* query = call;
  query->source_query = true;
  query->source.fields = UINT32_C(1) << 31;
}
static void source_layer_without_bytes(void* call) {
  query_call* query = call;
  query->source_query = true;
  query->source.fields = MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  query->source.source_layer_ids = &no_bytes_layer;
  query->source.source_layer_id_count = 1;
}
static void unparsable_source_filter(void* call) {
  query_call* query = call;
  query->source_query = true;
  query->source.fields = MLN_SOURCE_FEATURE_QUERY_OPTION_FILTER;
  query->source.filter = unparsable_filter;
}

static mln_status submit_query(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_render_session session = *(const mln_render_session*)context;
  const query_call* call = descriptor;
  mln_completion completion = mln_test_discard_completion();
  if (call->source_query) {
    return mln_render_session_query_source_features(
      session, call->source_id, &call->source, &completion, diagnostic
    );
  }
  return mln_render_session_query_rendered_features(
    session, &call->geometry, &call->rendered, &completion, diagnostic
  );
}

// One MVT tile with two layers, "points" and "labels", each holding one point
// feature at the tile's center: ID 1 in "points" and ID 2 in "labels". Each
// layer is version 2 with extent 4096, and its feature is a MoveTo of (2048,
// 2048), zigzag-encoded.
static const uint8_t two_layer_tile[] = {
  0x1a, 0x1a, 0x78, 0x02, 0x0a, 0x06, 'p',  'o',  'i',  'n',  't',  's',
  0x12, 0x0b, 0x08, 0x01, 0x18, 0x01, 0x22, 0x05, 0x09, 0x80, 0x20, 0x80,
  0x20, 0x28, 0x80, 0x20, 0x1a, 0x1a, 0x78, 0x02, 0x0a, 0x06, 'l',  'a',
  'b',  'e',  'l',  's',  0x12, 0x0b, 0x08, 0x02, 0x18, 0x01, 0x22, 0x05,
  0x09, 0x80, 0x20, 0x80, 0x20, 0x28, 0x80, 0x20,
};

static void count_fetch(void* user_data, mln_canonical_tile_id tile_id) {
  if (tile_id.z == 0) {
    atomic_fetch_add((atomic_int*)user_data, 1);
  }
  mln_test_pulse();
}

static void ignore_cancel(void* user_data, mln_canonical_tile_id tile_id) {
  (void)user_data;
  (void)tile_id;
}

typedef struct fetch_wait {
  atomic_int* fetches;
} fetch_wait;

static bool root_fetched(void* context) {
  return atomic_load(((fetch_wait*)context)->fetches) > 0;
}

typedef struct source_count {
  const mln_test_render_fixture* fixture;
} source_count;

static bool both_layers_loaded(void* context) {
  const source_count* wait = context;
  const mln_buffer_view layers[] = {
    MLN_BUFFER_LITERAL("points"), MLN_BUFFER_LITERAL("labels")
  };
  mln_source_feature_query_options query =
    mln_source_feature_query_options_default();
  query.fields = MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
  query.source_layer_ids = layers;
  query.source_layer_id_count = 2;
  const mln_test_feature_list list =
    mln_test_style_query_source_with(wait->fixture, "tiles", &query);
  return list.status == MLN_STATUS_OK && list.count == 2;
}

typedef struct source_row {
  const char* label;
  const char* layers[2];
  size_t layer_count;
  const char* filter;
  size_t count;
  // The ID every hit must carry, or null for any.
  const char* id;
} source_row;

// A source query reads a vector source through the source layers it names.
// Its results name the source but not the source layer.
static void source_queries_read_the_named_source_layers(void) {
  static atomic_int fetches;
  atomic_store(&fetches, 0);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_background_style_json);
  mln_custom_mvt_vector_source_options options =
    mln_custom_mvt_vector_source_options_default();
  options.fetch_tile = count_fetch;
  options.cancel_tile = ignore_cancel;
  options.user_data = &fetches;
  MLN_TEST_AWAIT_OK(mln_map_add_custom_mvt_vector_source(
    map, MLN_BUFFER_LITERAL("tiles"), &options, &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map,
    MLN_BUFFER_LITERAL(
      "{\"id\":\"dots\",\"type\":\"circle\",\"source\":\"tiles\","
      "\"source-layer\":\"points\"}"
    ),
    (mln_buffer_view){0}, &completion.descriptor, NULL
  ));
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  fetch_wait fetched = {.fetches = &fetches};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, root_fetched, &fetched, "the root tile fetch"
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_custom_mvt_vector_source_tile_data(
    map, MLN_BUFFER_LITERAL("tiles"),
    (mln_canonical_tile_id){.z = 0, .x = 0, .y = 0},
    mln_test_buffer_view(two_layer_tile, sizeof(two_layer_tile)),
    &completion.descriptor, NULL
  ));
  source_count loaded = {.fixture = &fixture};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, both_layers_loaded, &loaded, "both source layers"
  ));

  const source_row rows[] = {
    // MapLibre reads a vector source only through the layers a query names.
    {"no source layer reads nothing", {NULL}, 0, NULL, 0, NULL},
    {"one source layer reads that layer", {"labels"}, 1, NULL, 1, "\"id\":2"},
    {"two source layers read both", {"points", "labels"}, 2, NULL, 2, NULL},
    {"a missing source layer reads nothing", {"roads"}, 1, NULL, 0, NULL},
    {"a filter narrows the named layers",
     {"points", "labels"},
     2,
     "[\"==\",[\"id\"],1]",
     1,
     "\"id\":1"},
  };
  for (size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); index += 1) {
    const source_row* row = &rows[index];
    mln_source_feature_query_options query =
      mln_source_feature_query_options_default();
    mln_buffer_view layers[2] = {0};
    for (size_t layer = 0; layer < row->layer_count; layer += 1) {
      layers[layer] = mln_test_view_of(row->layers[layer]);
    }
    if (row->layer_count != 0) {
      query.fields = MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
      query.source_layer_ids = layers;
      query.source_layer_id_count = row->layer_count;
    }
    if (row->filter != NULL) {
      query.fields |= MLN_SOURCE_FEATURE_QUERY_OPTION_FILTER;
      query.filter = mln_test_view_of(row->filter);
    }
    const mln_test_feature_list list =
      mln_test_style_query_source_with(&fixture, "tiles", &query);
    MLN_TEST_OK_MESSAGE(list.status, row->label);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(row->count, list.count, row->label);
    for (size_t hit = 0; hit < list.count; hit += 1) {
      TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "tiles", list.features[hit].source_id, row->label
      );
      TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "", list.features[hit].source_layer_id, row->label
      );
      if (row->id != NULL) {
        TEST_ASSERT_NOT_NULL_MESSAGE(
          strstr(list.features[hit].feature, row->id), row->label
        );
      }
    }
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Three weighted points close enough to cluster at zoom 0, drawn only as
// their cluster.
static const char cluster_style_json[] =
  "{\"version\":8,\"sources\":{\"clustered\":{\"type\":\"geojson\","
  "\"cluster\":true,\"clusterRadius\":50,\"data\":{\"type\":"
  "\"FeatureCollection\",\"features\":["
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"one\"},\"geometry\":"
  "{\"type\":\"Point\",\"coordinates\":[0,0]}},"
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"two\"},\"geometry\":"
  "{\"type\":\"Point\",\"coordinates\":[0.001,0.001]}},"
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"three\"},\"geometry\":"
  "{\"type\":\"Point\",\"coordinates\":[0.002,0.002]}}]}}},"
  "\"layers\":[{\"id\":\"clusters\",\"type\":\"circle\",\"source\":"
  "\"clustered\",\"filter\":[\"has\",\"point_count\"],\"paint\":"
  "{\"circle-radius\":8}}]}";

// Runs one supercluster query and copies its JSON into `out`.
static mln_status query_extension(
  const mln_test_render_fixture* fixture, const char* feature,
  const char* field, const char* arguments, char* out, size_t capacity
) {
  const mln_buffer_view argument_view =
    arguments == NULL ? (mln_buffer_view){0} : mln_test_view_of(arguments);
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_render_session_query_feature_extensions(
    fixture->session, MLN_BUFFER_LITERAL("clustered"),
    mln_test_view_of(feature), MLN_BUFFER_LITERAL("supercluster"),
    mln_test_view_of(field), arguments == NULL ? NULL : &argument_view,
    &completion.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  (void)mln_test_render_fixture_finish_operation(fixture, &completion);
  bool found = false;
  const mln_status status =
    mln_test_style_finish_text(&completion, out, capacity, &found);
  TEST_ASSERT_TRUE(status != MLN_STATUS_OK || found);
  return status;
}

// Counts the features in a FeatureCollection's JSON.
static size_t count_features(const char* collection) {
  size_t count = 0;
  for (const char* cursor = strstr(collection, "\"type\":\"Feature\"");
       cursor != NULL; cursor = strstr(cursor + 1, "\"type\":\"Feature\"")) {
    count += 1;
  }
  return count;
}

// Parses the cluster_id property of a queried cluster, which MapLibre writes
// as an unsigned integer.
static unsigned long long cluster_id_of(const char* feature) {
  const char* property = strstr(feature, "\"cluster_id\":");
  TEST_ASSERT_NOT_NULL_MESSAGE(property, feature);
  const char* digits = property + strlen("\"cluster_id\":");
  TEST_ASSERT_TRUE_MESSAGE(*digits >= '0' && *digits <= '9', feature);
  char* end = NULL;
  const unsigned long long id = strtoull(digits, &end, 10);
  TEST_ASSERT_TRUE_MESSAGE(*end == ',' || *end == '}', feature);
  return id;
}

typedef struct extension_row {
  const char* label;
  const char* field;
  const char* arguments;
  // The features the result holds, or SIZE_MAX for a scalar result.
  size_t features;
} extension_row;

static void cluster_extensions_resolve_an_unsigned_cluster_id(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  render_style(
    runtime, map, MLN_BUFFER_LITERAL(cluster_style_json), 1, &fixture
  );
  const mln_test_feature_list clusters =
    mln_test_style_query_rendered(&fixture);
  TEST_ASSERT_EQUAL_size_t(1, clusters.count);
  const char* cluster = clusters.features[0].feature;
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(cluster, "\"point_count\":3"), cluster);

  static char result[4096];
  const extension_row rows[] = {
    // The points stay clustered one zoom level down.
    {"children descend one zoom level", "children", NULL, 1},
    {"leaves default to every leaf", "leaves", NULL, 3},
    {"a leaf limit bounds the leaves", "leaves", "{\"limit\":2}", 2},
    {"an offset past the leaves finds none", "leaves",
     "{\"limit\":1,\"offset\":3}", 0},
    {"the expansion zoom is a number", "expansion-zoom", NULL, SIZE_MAX},
  };
  for (size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); index += 1) {
    const extension_row* row = &rows[index];
    MLN_TEST_OK_MESSAGE(
      query_extension(
        &fixture, cluster, row->field, row->arguments, result, sizeof(result)
      ),
      row->label
    );
    if (row->features == SIZE_MAX) {
      char* end = NULL;
      const unsigned long zoom = strtoul(result, &end, 10);
      TEST_ASSERT_TRUE_MESSAGE(end != result && *end == '\0', result);
      TEST_ASSERT_GREATER_THAN_UINT32_MESSAGE(0, zoom, row->label);
    } else {
      TEST_ASSERT_EQUAL_size_t_MESSAGE(
        row->features, count_features(result), row->label
      );
    }
  }

  // An offset selects a later leaf than the one before it.
  char first[1024];
  char second[1024];
  (void)query_extension(
    &fixture, cluster, "leaves", "{\"limit\":1,\"offset\":0}", first,
    sizeof(first)
  );
  (void)query_extension(
    &fixture, cluster, "leaves", "{\"limit\":1,\"offset\":1}", second,
    sizeof(second)
  );
  TEST_ASSERT_EQUAL_size_t(1, count_features(first));
  TEST_ASSERT_EQUAL_size_t(1, count_features(second));
  TEST_ASSERT_TRUE_MESSAGE(strcmp(first, second) != 0, first);

  // The feature's JSON reaches the source with its number types intact, so
  // only an unsigned cluster_id names a cluster: the same number written as a
  // signed or fractional value finds nothing.
  const unsigned long long id = cluster_id_of(cluster);
  char feature[256];
  static const char* const written_ids[] = {"%llu", "-%llu", "%llu.5"};
  static const char* const expected[] = {NULL, "null", "null"};
  for (size_t index = 0; index < 3; index += 1) {
    char written[32];
    (void)snprintf(written, sizeof(written), written_ids[index], id);
    (void)snprintf(
      feature, sizeof(feature),
      "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
      "\"coordinates\":[0,0]},\"properties\":{\"cluster\":true,"
      "\"cluster_id\":%s}}",
      written
    );
    MLN_TEST_OK(
      query_extension(&fixture, feature, "leaves", NULL, result, sizeof(result))
    );
    if (expected[index] == NULL) {
      TEST_ASSERT_EQUAL_size_t_MESSAGE(3, count_features(result), written);
    } else {
      TEST_ASSERT_EQUAL_STRING_MESSAGE(expected[index], result, written);
    }
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

typedef struct extension_call {
  mln_buffer_view source_id;
  mln_buffer_view feature;
  mln_buffer_view extension;
  mln_buffer_view field;
  mln_buffer_view arguments;
  bool has_arguments;
} extension_call;

static void empty_extension_source(void* call) {
  ((extension_call*)call)->source_id = (mln_buffer_view){.data = "", .size = 0};
}
static void empty_extension(void* call) {
  ((extension_call*)call)->extension = (mln_buffer_view){.data = "", .size = 0};
}
static void empty_extension_field(void* call) {
  ((extension_call*)call)->field = (mln_buffer_view){.data = "", .size = 0};
}
static void geometry_instead_of_feature(void* call) {
  ((extension_call*)call)->feature =
    MLN_BUFFER_LITERAL("{\"type\":\"Point\",\"coordinates\":[0,0]}");
}
static void unparsable_feature(void* call) {
  ((extension_call*)call)->feature = MLN_BUFFER_LITERAL("{\"type\":");
}
static void array_arguments(void* call) {
  extension_call* extension = call;
  extension->arguments = MLN_BUFFER_LITERAL("[1]");
  extension->has_arguments = true;
}

static mln_status submit_extension(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_render_session session = *(const mln_render_session*)context;
  const extension_call* call = descriptor;
  mln_completion completion = mln_test_discard_completion();
  return mln_render_session_query_feature_extensions(
    session, call->source_id, call->feature, call->extension, call->field,
    call->has_arguments ? &call->arguments : NULL, &completion, diagnostic
  );
}

// Each malformed input is rejected before the query reaches the driver, so
// no completion runs. A frame of an empty style renders no feature, so a
// well-formed rendered query still completes, with an empty borrowed array.
static void malformed_queries_are_rejected_at_submission(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  static const mln_test_validation_case cases[] = {
    {"undersized geometry", undersized_geometry, MLN_STATUS_INVALID_ARGUMENT,
     "too small"},
    {"unknown geometry type", unknown_geometry_type,
     MLN_STATUS_INVALID_ARGUMENT, "geometry type is invalid"},
    {"non-finite point", non_finite_point, MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"line string without points", empty_line_string,
     MLN_STATUS_INVALID_ARGUMENT, "must contain points"},
    {"line string with null points", null_line_string_points,
     MLN_STATUS_INVALID_ARGUMENT, "must not be null"},
    {"undersized rendered options", undersized_rendered_options,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"unknown rendered option field", unknown_rendered_field,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"layer IDs present but null", null_layer_ids, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"layer ID without bytes", layer_id_without_bytes,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"unparsable rendered filter", unparsable_rendered_filter,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"well-formed source query", source_query, MLN_STATUS_OK, NULL},
    {"empty source ID", empty_source_id, MLN_STATUS_INVALID_ARGUMENT,
     "must not be empty"},
    {"source ID without bytes", source_id_without_bytes,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"undersized source options", undersized_source_options,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"unknown source option field", unknown_source_field,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"source layer without bytes", source_layer_without_bytes,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
    {"unparsable source filter", unparsable_source_filter,
     MLN_STATUS_INVALID_ARGUMENT, NULL},
  };
  const query_call defaults = {
    .geometry =
      mln_rendered_query_geometry_point((mln_screen_point){.x = 32, .y = 32}),
    .rendered = mln_rendered_feature_query_options_default(),
    .source = mln_source_feature_query_options_default(),
    .source_id = MLN_BUFFER_LITERAL("points"),
  };
  mln_test_run_validation_table(
    cases, sizeof(cases) / sizeof(cases[0]), &defaults, sizeof(defaults),
    submit_query, &fixture.session
  );

  static const mln_test_validation_case extension_cases[] = {
    {"empty source ID", empty_extension_source, MLN_STATUS_INVALID_ARGUMENT,
     "source_id must not be empty"},
    {"empty extension", empty_extension, MLN_STATUS_INVALID_ARGUMENT,
     "extension must not be empty"},
    {"empty extension field", empty_extension_field,
     MLN_STATUS_INVALID_ARGUMENT, "extension_field must not be empty"},
    {"a geometry instead of a feature", geometry_instead_of_feature,
     MLN_STATUS_INVALID_ARGUMENT, "one GeoJSON Feature"},
    {"unparsable feature", unparsable_feature, MLN_STATUS_INVALID_ARGUMENT,
     NULL},
    {"arguments that are not an object", array_arguments,
     MLN_STATUS_INVALID_ARGUMENT, "must be a JSON object"},
  };
  const extension_call extension_defaults = {
    .source_id = MLN_BUFFER_LITERAL("clustered"),
    .feature = MLN_BUFFER_LITERAL(
      "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
      "\"coordinates\":[0,0]},\"properties\":{\"cluster_id\":1}}"
    ),
    .extension = MLN_BUFFER_LITERAL("supercluster"),
    .field = MLN_BUFFER_LITERAL("children"),
  };
  mln_test_run_validation_table(
    extension_cases, sizeof(extension_cases) / sizeof(extension_cases[0]),
    &extension_defaults, sizeof(extension_defaults), submit_extension,
    &fixture.session
  );

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(&fixture)
  );
  const mln_test_feature_list list = mln_test_style_query_rendered(&fixture);
  MLN_TEST_OK(list.status);
  TEST_ASSERT_EQUAL_size_t(0, list.count);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(rendered_queries_select_by_geometry_layer_and_filter);
  RUN_TEST(source_queries_read_the_named_source_layers);
  RUN_TEST(cluster_extensions_resolve_an_unsigned_cluster_id);
  RUN_TEST(malformed_queries_are_rejected_at_submission);
}
