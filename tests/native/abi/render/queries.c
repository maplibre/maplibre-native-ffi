// Feature queries against a render session's latest frame.
//
// The map is 64 x 64 at zoom 0 around (0, 0), so a longitude of 10 degrees
// lies about 14 pixels from the center.

#include <math.h>

#include "support/map.h"
#include "support/resources.h"
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
  mln_test_load_style_and_wait(map, style);
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, fixture));
  rendered_count wait = {.fixture = fixture, .count = rendered};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    fixture, renders_count, &wait, "the style's features to render"
  ));
}

static bool feature_named(const mln_test_feature* feature, const char* name) {
  char property[64];
  (void)snprintf(property, sizeof(property), "\"name\":\"%s\"", name);
  return strstr(feature->feature, property) != NULL;
}

// Names the points a list holds, in the order west, east, as a bit mask.
enum { WEST = 1U << 0U, EAST = 1U << 1U };

// Names the style layers a list's hits came from as a bit mask.
enum { CIRCLES = 1U << 0U, HALOS = 1U << 1U };

static unsigned int named_points(const mln_test_feature_list* list) {
  unsigned int found = 0;
  for (size_t index = 0; index < list->count; index += 1) {
    found |= feature_named(&list->features[index], "west") ? WEST : 0U;
    found |= feature_named(&list->features[index], "east") ? EAST : 0U;
  }
  return found;
}

static unsigned int named_layers(const mln_test_feature_list* list) {
  unsigned int found = 0;
  for (size_t index = 0; index < list->count; index += 1) {
    const char* layer = list->features[index].layer_id;
    found |= strcmp(layer, "circles") == 0 ? CIRCLES : 0U;
    found |= strcmp(layer, "halos") == 0 ? HALOS : 0U;
  }
  return found;
}

static const mln_screen_point crossing_line[] = {
  {.x = 4, .y = 32}, {.x = 60, .y = 32}
};

typedef struct rendered_row {
  const char* label;
  mln_rendered_query_geometry geometry;
  const char* layer;
  const char* filter;
  size_t count;
  unsigned int points;
  unsigned int layers;
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
     {.size = sizeof(mln_rendered_query_geometry),
      .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT,
      .data = {.point = {.x = 46, .y = 32}}},
     NULL,
     NULL,
     2,
     EAST,
     CIRCLES | HALOS},
    {"a layer ID narrows a hit to that layer",
     {.size = sizeof(mln_rendered_query_geometry),
      .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT,
      .data = {.point = {.x = 46, .y = 32}}},
     "circles",
     NULL,
     1,
     EAST,
     CIRCLES},
    {"a filter keeps the matching feature",
     {.size = sizeof(mln_rendered_query_geometry),
      .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
      .data = {.box = {.min = {.x = 0, .y = 0}, .max = {.x = 64, .y = 64}}}},
     "circles",
     "[\"==\",[\"get\",\"kind\"],\"a\"]",
     1,
     WEST,
     CIRCLES},
    {"an empty filter is no filter",
     {.size = sizeof(mln_rendered_query_geometry),
      .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
      .data = {.box = {.min = {.x = 0, .y = 0}, .max = {.x = 64, .y = 64}}}},
     "circles",
     "",
     2,
     WEST | EAST,
     CIRCLES},
    {"a box past every edge is clipped to the viewport",
     {.size = sizeof(mln_rendered_query_geometry),
      .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
      .data =
        {.box =
           {.min = {.x = -4096, .y = -4096}, .max = {.x = 4096, .y = 4096}}}},
     "circles",
     NULL,
     2,
     WEST | EAST,
     CIRCLES},
    {"a box with swapped corners is normalized",
     {.size = sizeof(mln_rendered_query_geometry),
      .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
      .data = {.box = {.min = {.x = 64, .y = 40}, .max = {.x = 36, .y = 24}}}},
     "circles",
     NULL,
     1,
     EAST,
     CIRCLES},
    {"a box beyond the viewport finds nothing",
     {.size = sizeof(mln_rendered_query_geometry),
      .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_BOX,
      .data = {.box = {.min = {.x = 100, .y = 0}, .max = {.x = 200, .y = 64}}}},
     NULL,
     NULL,
     0,
     0,
     0},
    {"a line string through both circles hits both",
     {.size = sizeof(mln_rendered_query_geometry),
      .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
      .data = {.line_string = {.points = crossing_line, .point_count = 2}}},
     "circles",
     NULL,
     2,
     WEST | EAST,
     CIRCLES},
  };
  for (size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); index += 1) {
    const rendered_row* row = &rows[index];
    mln_rendered_feature_query_options options = {
      .size = sizeof(mln_rendered_feature_query_options)
    };
    const mln_buffer_view layer = row->layer == NULL
                                    ? (mln_buffer_view){0}
                                    : mln_buffer_view_from_cstr(row->layer);
    if (row->layer != NULL) {
      options.fields = MLN_RENDERED_FEATURE_QUERY_OPTION_LAYER_IDS;
      options.layer_ids = &layer;
      options.layer_id_count = 1;
    }
    if (row->filter != NULL) {
      options.filter = mln_buffer_view_from_cstr(row->filter);
    }
    const mln_test_feature_list list =
      mln_test_style_query_rendered_with(&fixture, &row->geometry, &options);
    MLN_TEST_OK_MESSAGE(list.status, row->label);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(row->count, list.count, row->label);
    TEST_ASSERT_EQUAL_UINT_MESSAGE(
      row->points, named_points(&list), row->label
    );
    TEST_ASSERT_EQUAL_UINT_MESSAGE(
      row->layers, named_layers(&list), row->label
    );
    for (size_t hit = 0; hit < list.count; hit += 1) {
      TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "points", list.features[hit].source_id, row->label
      );
    }
  }

  const mln_test_feature_list source =
    mln_test_style_query_source_with(&fixture, "points", NULL);
  MLN_TEST_OK(source.status);
  TEST_ASSERT_EQUAL_size_t(2, source.count);
  TEST_ASSERT_EQUAL_UINT(WEST | EAST, named_points(&source));
  // A GeoJSON source has no source layers to name.
  for (size_t hit = 0; hit < source.count; hit += 1) {
    TEST_ASSERT_EQUAL_STRING("", source.features[hit].source_layer_id);
  }

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
  ((query_call*)call)->geometry = (mln_rendered_query_geometry){
    .size = sizeof(mln_rendered_query_geometry),
    .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
    .data = {.line_string = {.points = crossing_line, .point_count = 0}}
  };
}
static void null_line_string_points(void* call) {
  ((query_call*)call)->geometry = (mln_rendered_query_geometry){
    .size = sizeof(mln_rendered_query_geometry),
    .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_LINE_STRING,
    .data = {.line_string = {.points = NULL, .point_count = 2}}
  };
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

static void count_fetch(
  void* user_data, mln_map map, mln_buffer_view source_id,
  mln_canonical_tile_id tile_id
) {
  (void)map;
  (void)source_id;
  if (tile_id.z == 0) {
    atomic_fetch_add((atomic_int*)user_data, 1);
  }
  mln_test_pulse();
}

static void ignore_cancel(
  void* user_data, mln_map map, mln_buffer_view source_id,
  mln_canonical_tile_id tile_id
) {
  (void)user_data;
  (void)map;
  (void)source_id;
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
  mln_source_feature_query_options query = {
    .size = sizeof(mln_source_feature_query_options)
  };
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

static void source_queries_read_the_named_source_layers(void) {
  static atomic_int fetches;
  atomic_store(&fetches, 0);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_background_style_json);
  mln_custom_vector_source_options options = {
    .size = sizeof(mln_custom_vector_source_options)
  };
  options.fetch_tile = count_fetch;
  options.cancel_tile = ignore_cancel;
  options.user_data = &fetches;
  MLN_TEST_AWAIT_OK(mln_map_add_custom_vector_source(
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
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  fetch_wait fetched = {.fetches = &fetches};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, root_fetched, &fetched, "the root tile fetch"
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_custom_vector_source_tile_data(
    map, MLN_BUFFER_LITERAL("tiles"),
    (mln_canonical_tile_id){.z = 0, .x = 0, .y = 0},
    MLN_STYLE_VECTOR_TILE_ENCODING_MVT,
    mln_test_buffer_view(two_layer_tile, sizeof(two_layer_tile)),
    &completion.descriptor, NULL
  ));
  source_count loaded = {.fixture = &fixture};
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, both_layers_loaded, &loaded, "both source layers"
  ));

  const source_row rows[] = {
    {"one source layer reads that layer", {"labels"}, 1, NULL, 1, "\"id\":2"},
    {"two source layers read both", {"points", "labels"}, 2, NULL, 2, NULL},
    {"a filter narrows the named layers",
     {"points", "labels"},
     2,
     "[\"==\",[\"id\"],1]",
     1,
     "\"id\":1"},
    {"an empty filter is no filter", {"points", "labels"}, 2, "", 2, NULL},
  };
  for (size_t index = 0; index < sizeof(rows) / sizeof(rows[0]); index += 1) {
    const source_row* row = &rows[index];
    mln_source_feature_query_options query = {
      .size = sizeof(mln_source_feature_query_options)
    };
    mln_buffer_view layers[2] = {0};
    for (size_t layer = 0; layer < row->layer_count; layer += 1) {
      layers[layer] = mln_buffer_view_from_cstr(row->layers[layer]);
    }
    if (row->layer_count != 0) {
      query.fields = MLN_SOURCE_FEATURE_QUERY_OPTION_SOURCE_LAYER_IDS;
      query.source_layer_ids = layers;
      query.source_layer_id_count = row->layer_count;
    }
    if (row->filter != NULL) {
      query.filter = mln_buffer_view_from_cstr(row->filter);
    }
    const mln_test_feature_list list =
      mln_test_style_query_source_with(&fixture, "tiles", &query);
    MLN_TEST_OK_MESSAGE(list.status, row->label);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(row->count, list.count, row->label);
    for (size_t hit = 0; hit < list.count; hit += 1) {
      const mln_test_feature* feature = &list.features[hit];
      if (row->id != NULL) {
        TEST_ASSERT_NOT_NULL_MESSAGE(
          strstr(feature->feature, row->id), row->label
        );
      }
      const bool in_points = strstr(feature->feature, "\"id\":1") != NULL;
      TEST_ASSERT_EQUAL_STRING_MESSAGE(
        in_points ? "points" : "labels", feature->source_layer_id, row->label
      );
    }
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Three points close enough to form one cluster at every zoom level up to the
// cluster max zoom, 5, drawn only as their cluster. "empty" clusters no
// points, "plain" doesn't cluster, and "tiles" isn't a GeoJSON source; no
// layer draws them.
#define CLUSTERED_SOURCE_JSON                                               \
  "{\"type\":\"geojson\",\"cluster\":true,\"clusterRadius\":50,"            \
  "\"clusterMaxZoom\":5,\"data\":{\"type\":\"FeatureCollection\","          \
  "\"features\":["                                                          \
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"one\"},\"geometry\":"   \
  "{\"type\":\"Point\",\"coordinates\":[0,0]}},"                            \
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"two\"},\"geometry\":"   \
  "{\"type\":\"Point\",\"coordinates\":[0.001,0.001]}},"                    \
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"three\"},\"geometry\":" \
  "{\"type\":\"Point\",\"coordinates\":[0.002,0.002]}}]}}"
#define CLUSTER_LAYER_JSON                                             \
  "{\"id\":\"clusters\",\"type\":\"circle\",\"source\":\"clustered\"," \
  "\"filter\":[\"has\",\"point_count\"],\"paint\":{\"circle-radius\":8}}"
static const char cluster_style_json[] =
  "{\"version\":8,\"sources\":{\"clustered\":" CLUSTERED_SOURCE_JSON
  ","
  "\"empty\":{\"type\":\"geojson\",\"cluster\":true,\"data\":"
  "{\"type\":\"FeatureCollection\",\"features\":[]}},"
  "\"plain\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"FeatureCollection\",\"features\":[]}},"
  "\"tiles\":{\"type\":\"vector\","
  "\"tiles\":[\"fixture://tiles/{z}/{x}/{y}.mvt\"]}},"
  "\"layers\":[" CLUSTER_LAYER_JSON "]}";

static size_t count_features(const char* collection) {
  size_t count = 0;
  for (const char* cursor = strstr(collection, "\"type\":\"Feature\"");
       cursor != NULL; cursor = strstr(cursor + 1, "\"type\":\"Feature\"")) {
    count += 1;
  }
  return count;
}

// Parses the cluster_id property of a queried cluster.
static uint32_t cluster_id_of(const char* feature) {
  const char* property = strstr(feature, "\"cluster_id\":");
  TEST_ASSERT_NOT_NULL_MESSAGE(property, feature);
  const char* digits = property + strlen("\"cluster_id\":");
  char* end = NULL;
  const unsigned long long id = strtoull(digits, &end, 10);
  TEST_ASSERT_TRUE_MESSAGE(
    end != digits && (*end == ',' || *end == '}'), feature
  );
  TEST_ASSERT_TRUE_MESSAGE(id <= UINT32_MAX, feature);
  return (uint32_t)id;
}

// Services a cluster query's completion and copies its feature collection
// into `out`.
static mln_status finish_cluster_collection(
  const mln_test_render_fixture* fixture, mln_test_completion* completion,
  char* out, size_t capacity
) {
  (void)mln_test_render_fixture_finish_operation(fixture, completion);
  bool found = false;
  const mln_status status =
    mln_test_style_finish_text(completion, out, capacity, &found);
  TEST_ASSERT_TRUE(status != MLN_STATUS_OK || found);
  if (status == MLN_STATUS_OK) {
    TEST_ASSERT_NOT_NULL_MESSAGE(strstr(out, "\"FeatureCollection\""), out);
  }
  return status;
}

static mln_status cluster_children(
  const mln_test_render_fixture* fixture, const char* source_id,
  uint32_t cluster_id, char* out, size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_render_session_get_cluster_children(
    fixture->session, mln_buffer_view_from_cstr(source_id), cluster_id,
    &completion.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  return finish_cluster_collection(fixture, &completion, out, capacity);
}

static mln_status cluster_leaves(
  const mln_test_render_fixture* fixture, const char* source_id,
  uint32_t cluster_id, uint32_t limit, uint32_t offset, char* out,
  size_t capacity
) {
  mln_test_completion completion = mln_test_completion_buffer_view();
  MLN_TEST_OK(mln_render_session_get_cluster_leaves(
    fixture->session, mln_buffer_view_from_cstr(source_id), cluster_id, limit,
    offset, &completion.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  return finish_cluster_collection(fixture, &completion, out, capacity);
}

static mln_status cluster_expansion_zoom(
  const mln_test_render_fixture* fixture, const char* source_id,
  uint32_t cluster_id, uint32_t* out_zoom
) {
  mln_test_completion completion =
    mln_test_completion_default(sizeof(uint32_t));
  MLN_TEST_OK(mln_render_session_get_cluster_expansion_zoom(
    fixture->session, mln_buffer_view_from_cstr(source_id), cluster_id,
    &completion.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  const mln_status status =
    mln_test_render_fixture_finish_operation(fixture, &completion);
  if (status == MLN_STATUS_OK) {
    TEST_ASSERT_TRUE(
      mln_test_completion_copy_value(&completion, out_zoom, sizeof(*out_zoom))
    );
  }
  mln_test_completion_destroy(&completion);
  return status;
}

// Names the original points a collection holds, in the order one, two, three,
// as a bit mask.
enum { ONE = 1U << 0U, TWO = 1U << 1U, THREE = 1U << 2U };

static unsigned int leaf_names(const char* collection) {
  static const char* const names[] = {
    "\"name\":\"one\"", "\"name\":\"two\"", "\"name\":\"three\""
  };
  unsigned int found = 0;
  for (size_t index = 0; index < 3; index += 1) {
    if (strstr(collection, names[index]) != NULL) found |= 1U << index;
  }
  return found;
}

typedef struct leaves_row {
  const char* label;
  uint32_t limit;
  uint32_t offset;
  size_t features;
} leaves_row;

typedef struct cluster_failure_row {
  const char* label;
  const char* source_id;
  uint32_t cluster_id;
  mln_status expected;
} cluster_failure_row;

static void cluster_queries_read_a_clustered_source(void) {
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
  const uint32_t id = cluster_id_of(cluster);

  // One zoom level closer, the three points still form one cluster.
  static char result[4096];
  MLN_TEST_OK(
    cluster_children(&fixture, "clustered", id, result, sizeof(result))
  );
  TEST_ASSERT_EQUAL_size_t_MESSAGE(1, count_features(result), result);
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(result, "\"point_count\":3"), result);

  const leaves_row leaves[] = {
    {"every leaf", 10, 0, 3},
    {"a limit bounds the leaves", 2, 0, 2},
    {"an offset skips leaves", 2, 2, 1},
    {"a limit of 0 gives no leaves", 0, 0, 0},
  };
  for (size_t index = 0; index < sizeof(leaves) / sizeof(leaves[0]);
       index += 1) {
    const leaves_row* row = &leaves[index];
    MLN_TEST_OK_MESSAGE(
      cluster_leaves(
        &fixture, "clustered", id, row->limit, row->offset, result,
        sizeof(result)
      ),
      row->label
    );
    TEST_ASSERT_EQUAL_size_t_MESSAGE(
      row->features, count_features(result), row->label
    );
  }

  // The leaves are the original points, and two pages cover each one once.
  MLN_TEST_OK(
    cluster_leaves(&fixture, "clustered", id, 10, 0, result, sizeof(result))
  );
  TEST_ASSERT_EQUAL_UINT(ONE | TWO | THREE, leaf_names(result));
  TEST_ASSERT_NULL_MESSAGE(strstr(result, "point_count"), result);
  MLN_TEST_OK(
    cluster_leaves(&fixture, "clustered", id, 2, 0, result, sizeof(result))
  );
  const unsigned int first_page = leaf_names(result);
  MLN_TEST_OK(
    cluster_leaves(&fixture, "clustered", id, 2, 2, result, sizeof(result))
  );
  const unsigned int second_page = leaf_names(result);
  TEST_ASSERT_EQUAL_UINT(0, first_page & second_page);
  TEST_ASSERT_EQUAL_UINT(ONE | TWO | THREE, first_page | second_page);

  // The points stay one cluster through the cluster max zoom, 5, and show
  // apart one level closer.
  uint32_t zoom = 0;
  MLN_TEST_OK(cluster_expansion_zoom(&fixture, "clustered", id, &zoom));
  TEST_ASSERT_EQUAL_UINT32(6, zoom);

  // A cluster ID's low five bits hold its zoom level plus one, so 0 and an ID
  // whose zoom lies beyond the source's cluster max zoom name no cluster.
  const cluster_failure_row failures[] = {
    {"a missing source", "missing", id, MLN_STATUS_NOT_FOUND},
    {"a GeoJSON source that doesn't cluster", "plain", id,
     MLN_STATUS_INVALID_ARGUMENT},
    {"a source that isn't GeoJSON", "tiles", id, MLN_STATUS_INVALID_ARGUMENT},
    {"a clustered source with no points", "empty", id, MLN_STATUS_NOT_FOUND},
    {"cluster ID 0", "clustered", 0, MLN_STATUS_NOT_FOUND},
    {"an ID past the source's clusters", "clustered",
     (id & UINT32_C(0x1F)) | (UINT32_C(0x7FFFFFF) << 5), MLN_STATUS_NOT_FOUND},
    {"an ID beyond the cluster max zoom", "clustered",
     (id & ~UINT32_C(0x1F)) | UINT32_C(0x1F), MLN_STATUS_NOT_FOUND},
  };
  for (size_t index = 0; index < sizeof(failures) / sizeof(failures[0]);
       index += 1) {
    const cluster_failure_row* row = &failures[index];
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      row->expected,
      cluster_children(
        &fixture, row->source_id, row->cluster_id, result, sizeof(result)
      ),
      row->label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      row->expected,
      cluster_leaves(
        &fixture, row->source_id, row->cluster_id, 10, 0, result, sizeof(result)
      ),
      row->label
    );
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      row->expected,
      cluster_expansion_zoom(&fixture, row->source_id, row->cluster_id, &zoom),
      row->label
    );
  }

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The vector tile that a static map's still image waits on, which the
// provider holds.
static const char held_tile_url[] = "custom://held/0/0/0.pbf";
static const mln_test_provided_resource held_tile[] = {
  {.url = held_tile_url, .held = true},
};

// Two of the three points, which form one cluster as the three do.
static const char two_point_source_json[] =
  "{\"type\":\"geojson\",\"cluster\":true,\"clusterRadius\":50,"
  "\"clusterMaxZoom\":5,\"data\":{\"type\":\"FeatureCollection\","
  "\"features\":["
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"one\"},\"geometry\":"
  "{\"type\":\"Point\",\"coordinates\":[0,0]}},"
  "{\"type\":\"Feature\",\"properties\":{\"name\":\"two\"},\"geometry\":"
  "{\"type\":\"Point\",\"coordinates\":[0.001,0.001]}}]}}";

typedef struct two_point_cluster {
  const mln_test_render_fixture* fixture;
  mln_test_provider* provider;
  char feature[512];
} two_point_cluster;

// Whether the renderer has asked for the held tile, so it has started
// rendering the new style, and a source query finds the two-point cluster.
static bool two_point_cluster_found(void* context) {
  two_point_cluster* wait = context;
  if (mln_test_provider_requests(wait->provider, held_tile_url) < 1) {
    return false;
  }
  const mln_test_feature_list list =
    mln_test_style_query_source(wait->fixture, "clustered", NULL);
  for (size_t index = 0; index < list.count; index += 1) {
    const char* feature = list.features[index].feature;
    if (strstr(feature, "\"point_count\":2") != NULL) {
      (void)snprintf(wait->feature, sizeof(wait->feature), "%s", feature);
      return true;
    }
  }
  return false;
}

// Cluster queries read the source of the session's most recent render
// attempt, not the last frame it completed. A static map renders only once
// everything it loads has arrived, so while a tile is held, an attempt holds a
// source that no frame has drawn. The test replaces the source rather than
// its data, so the new source's tiles hold only the new data.
static void cluster_queries_read_the_most_recent_render_attempt(void) {
  mln_test_provider* provider = mln_test_provider_create(held_tile, 1);
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_provider_install(runtime, provider);
  mln_map_options options = mln_test_map_options();
  options.initial_extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_load_style_and_wait(
    map, MLN_BUFFER_LITERAL(
           "{\"version\":8,\"sources\":{\"clustered\":" CLUSTERED_SOURCE_JSON
           "},\"layers\":[" CLUSTER_LAYER_JSON "]}"
         )
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  static char result[4096];
  // A static map renders nothing until a still image asks for it.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NOT_FOUND,
    cluster_leaves(&fixture, "clustered", 1, 10, 0, result, sizeof(result))
  );
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));

  // Replace the three points with two, and add a source whose tile is held,
  // so the next still image's render attempts never complete a frame.
  MLN_TEST_AWAIT_OK(mln_map_remove_style_layer(
    map, MLN_BUFFER_LITERAL("clusters"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_remove_style_source(
    map, MLN_BUFFER_LITERAL("clustered"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("clustered"),
    MLN_BUFFER_LITERAL(two_point_source_json), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map, MLN_BUFFER_LITERAL(CLUSTER_LAYER_JSON), (mln_buffer_view){0},
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_style_source_json(
    map, MLN_BUFFER_LITERAL("held"),
    MLN_BUFFER_LITERAL(
      "{\"type\":\"vector\",\"tiles\":[\"custom://held/{z}/{x}/{y}.pbf\"],"
      "\"minzoom\":0,\"maxzoom\":0}"
    ),
    &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_style_layer_json(
    map,
    MLN_BUFFER_LITERAL(
      "{\"id\":\"fill\",\"type\":\"fill\",\"source\":\"held\","
      "\"source-layer\":\"any\"}"
    ),
    (mln_buffer_view){0}, &completion.descriptor, NULL
  ));
  mln_test_completion still = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_request_still_image(map, &still.descriptor, NULL));
  two_point_cluster wait = {.fixture = &fixture, .provider = provider};
  MLN_TEST_OK(mln_test_render_step_until(
    &fixture, two_point_cluster_found, &wait, mln_test_deadline_default(),
    "a source query's two-point cluster"
  ));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&still));

  // The cluster that the new source's tiles hold traverses to its own two
  // points.
  const uint32_t id = cluster_id_of(wait.feature);
  MLN_TEST_OK(
    cluster_leaves(&fixture, "clustered", id, 10, 0, result, sizeof(result))
  );
  TEST_ASSERT_EQUAL_size_t_MESSAGE(2, count_features(result), result);
  TEST_ASSERT_EQUAL_UINT(ONE | TWO, leaf_names(result));
  MLN_TEST_OK(
    cluster_children(&fixture, "clustered", id, result, sizeof(result))
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(strstr(result, "\"point_count\":2"), result);

  const mln_test_provider_request* request =
    mln_test_provider_request_at(provider, held_tile_url, 0);
  TEST_ASSERT_NOT_NULL(request);
  const mln_resource_response empty = {
    .size = sizeof(mln_resource_response),
    .status = MLN_RESOURCE_RESPONSE_STATUS_OK,
  };
  mln_test_request_answer(request->handle, &empty);
  MLN_TEST_OK(mln_test_render_pending_still_image(&fixture, &still));
  mln_test_completion_destroy(&still);

  // A detached session has no renderer to read.
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  mln_completion discarded = mln_test_discard_completion();
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_get_cluster_leaves(
      fixture.session, MLN_BUFFER_LITERAL("clustered"), id, 10, 0, &discarded,
      MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "not attached"));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
  mln_test_provider_destroy(provider);
}

typedef enum cluster_query {
  CLUSTER_CHILDREN,
  CLUSTER_LEAVES,
  CLUSTER_EXPANSION_ZOOM,
} cluster_query;

typedef struct cluster_call {
  cluster_query query;
  mln_buffer_view source_id;
} cluster_call;

static void empty_children_source(void* call) {
  ((cluster_call*)call)->query = CLUSTER_CHILDREN;
}
static void empty_leaves_source(void* call) {
  ((cluster_call*)call)->query = CLUSTER_LEAVES;
}
static void empty_expansion_zoom_source(void* call) {
  ((cluster_call*)call)->query = CLUSTER_EXPANSION_ZOOM;
}

static mln_status submit_cluster_query(
  void* context, const void* descriptor, mln_diagnostic* diagnostic
) {
  const mln_render_session session = *(const mln_render_session*)context;
  const cluster_call* call = descriptor;
  mln_completion completion = mln_test_discard_completion();
  switch (call->query) {
    case CLUSTER_CHILDREN:
      return mln_render_session_get_cluster_children(
        session, call->source_id, 1, &completion, diagnostic
      );
    case CLUSTER_LEAVES:
      return mln_render_session_get_cluster_leaves(
        session, call->source_id, 1, 10, 0, &completion, diagnostic
      );
    case CLUSTER_EXPANSION_ZOOM:
      return mln_render_session_get_cluster_expansion_zoom(
        session, call->source_id, 1, &completion, diagnostic
      );
  }
  return MLN_STATUS_NATIVE_ERROR;
}

// A frame of an empty style renders no feature, so a well-formed rendered
// query still completes, with an empty borrowed array.
static void malformed_queries_are_rejected_at_submission(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_empty_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));

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
  };
  const query_call defaults = {
    .geometry =
      (mln_rendered_query_geometry){
        .size = sizeof(mln_rendered_query_geometry),
        .type = MLN_RENDERED_QUERY_GEOMETRY_TYPE_POINT,
        .data = {.point = (mln_screen_point){.x = 32, .y = 32}}
      },
    .rendered =
      (mln_rendered_feature_query_options){
        .size = sizeof(mln_rendered_feature_query_options)
      },
    .source =
      (mln_source_feature_query_options){
        .size = sizeof(mln_source_feature_query_options)
      },
    .source_id = MLN_BUFFER_LITERAL("points"),
  };
  mln_test_run_validation_table(
    cases, sizeof(cases) / sizeof(cases[0]), &defaults, sizeof(defaults),
    submit_query, &fixture.session
  );

  static const mln_test_validation_case cluster_cases[] = {
    {"children of an empty source ID", empty_children_source,
     MLN_STATUS_INVALID_ARGUMENT, "source_id must not be empty"},
    {"leaves of an empty source ID", empty_leaves_source,
     MLN_STATUS_INVALID_ARGUMENT, "source_id must not be empty"},
    {"expansion zoom of an empty source ID", empty_expansion_zoom_source,
     MLN_STATUS_INVALID_ARGUMENT, "source_id must not be empty"},
  };
  const cluster_call cluster_defaults = {
    .source_id = (mln_buffer_view){.data = "", .size = 0},
  };
  mln_test_run_validation_table(
    cluster_cases, sizeof(cluster_cases) / sizeof(cluster_cases[0]),
    &cluster_defaults, sizeof(cluster_defaults), submit_cluster_query,
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
  RUN_TEST(cluster_queries_read_a_clustered_source);
  RUN_TEST(cluster_queries_read_the_most_recent_render_attempt);
  RUN_TEST(malformed_queries_are_rejected_at_submission);
}
