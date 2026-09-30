// GeoJSON sources: prepared data and its options, installing one prepared
// handle on many sources, the synchronous tiling override, and URL data served
// through a resource provider.

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

// One point at the map's center, named so a query can tell datasets apart.
#define POINT_COLLECTION(name)                                           \
  "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\"," \
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"             \
  "\"properties\":{\"name\":\"" name "\"}}]}"

static const char point_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";

static mln_geojson_source_data prepare(
  const char* json, const mln_geojson_source_options* options
) {
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     mln_test_view_of(json), options, &data, MLN_TEST_DIAGNOSTIC
                   )
  );
  return data;
}

// Adds a circle layer that draws `source`, so the source's tiles render.
static void draw_source(mln_map map, const char* source) {
  char layer[160];
  snprintf(
    layer, sizeof(layer),
    "{\"id\":\"%s-dots\",\"type\":\"circle\",\"source\":\"%s\"}", source, source
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_style_layer_json(
                     map, mln_test_view_of(layer), MLN_BUFFER_LITERAL(""),
                     &completion.descriptor, NULL
                   )
  );
}

typedef struct source_query {
  const mln_test_render_fixture* fixture;
  const char* const* sources;
  size_t source_count;
  // The name every source must hold exactly one feature under.
  const char* name;
  mln_test_feature_list last;
} source_query;

static bool sources_hold_the_name(void* context) {
  source_query* query = context;
  char property[64];
  snprintf(property, sizeof(property), "\"name\":\"%s\"", query->name);
  for (size_t index = 0; index < query->source_count; index += 1) {
    query->last =
      mln_test_style_query_source(query->fixture, query->sources[index], NULL);
    if (
      query->last.status != MLN_STATUS_OK || query->last.count != 1 ||
      strstr(query->last.features[0].feature, property) == NULL
    ) {
      return false;
    }
  }
  return true;
}

// Prepared GeoJSON data creation is synchronous and touches no runtime or
// map, so validation reports through status and the call's diagnostic.
static void geojson_source_data_create_rejects_unsafe_raw_values(void) {
  static const char trailing_geojson[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}garbage";
  mln_geojson_source_data trailing_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(trailing_geojson), NULL, &trailing_data, NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, trailing_data);

  static const char nul_geojson[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}\0garbage";
  mln_geojson_source_data nul_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      (mln_buffer_view){.data = nul_geojson, .size = sizeof(nul_geojson) - 1},
      NULL, &nul_data, NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, nul_data);

  // A populated output handle is rejected rather than silently overwritten.
  static const char empty_collection[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}";
  mln_geojson_source_data populated = prepare(empty_collection, NULL);
  mln_geojson_source_data reused = populated;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(empty_collection), NULL, &reused, NULL
    )
  );
  mln_geojson_source_data_destroy(populated);

  // Unsafe raw options reject data preparation up front.
  mln_geojson_source_options short_size = mln_geojson_source_options_default();
  short_size.size = sizeof(uint32_t);
  mln_geojson_source_data short_size_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(empty_collection), &short_size, &short_size_data, NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, short_size_data);

  // Cluster properties are expressions, so a JSON object of plain values
  // fails MapLibre Native's conversion.
  mln_geojson_source_options unconvertible =
    mln_geojson_source_options_default();
  unconvertible.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER |
                         MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  unconvertible.cluster = true;
  unconvertible.cluster_properties = MLN_BUFFER_LITERAL("{\"total\":5}");
  mln_geojson_source_data unconvertible_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(empty_collection), &unconvertible, &unconvertible_data,
      MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(
    strstr(mln_test_last_error(), "GeoJSON source options"),
    mln_test_last_error()
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, unconvertible_data);
}

// Supercluster reads every feature geometry as a point, so data preparation
// rejects other geometry up front and names the feature and constraint.
static void clustered_geojson_data_reports_non_point_geometry(void) {
  static const char data[] =
    "{\"type\":\"FeatureCollection\",\"features\":["
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[-122.5,37.7]},\"properties\":{}},"
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"GeometryCollection\","
    "\"geometries\":[{\"type\":\"Point\",\"coordinates\":[-122.4,37.8]}]},"
    "\"properties\":{}}]}";

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;
  mln_geojson_source_data prepared = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(data), &clustered, &prepared, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, prepared);

  const char* message = mln_test_last_error();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "point geometry on every feature"));
  TEST_ASSERT_NOT_NULL(strstr(message, "feature 1"));
  TEST_ASSERT_NOT_NULL(strstr(message, "geometry collection"));

  // The constraint belongs to clustering alone, so the same data tiles fine
  // without it.
  mln_geojson_source_data_destroy(prepare(data, NULL));
}

static void clustered_geojson_data_requires_a_feature_collection(void) {
  static const char bare_geometry[] =
    "{\"type\":\"Point\",\"coordinates\":[-122.5,37.7]}";
  static const char single_feature[] =
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[-122.5,37.7]},\"properties\":{}}";

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;

  // MapLibre Native clusters feature collections only, so both of these would
  // tile unclustered rather than honouring the requested cluster option.
  mln_geojson_source_data prepared = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_geojson_source_data_create(
                                   MLN_BUFFER_LITERAL(bare_geometry),
                                   &clustered, &prepared, MLN_TEST_DIAGNOSTIC
                                 )
  );
  const char* message = mln_test_last_error();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "requires a feature collection"));
  TEST_ASSERT_NOT_NULL(strstr(message, "a bare geometry"));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_geojson_source_data_create(
                                   MLN_BUFFER_LITERAL(single_feature),
                                   &clustered, &prepared, MLN_TEST_DIAGNOSTIC
                                 )
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "a single feature"));

  // The constraint belongs to clustering alone, so the same data tiles fine
  // without it.
  mln_geojson_source_data_destroy(prepare(bare_geometry, NULL));

  // An empty feature collection carries nothing to cluster, so it stays
  // accepted and a later update supplies the features to cluster.
  mln_geojson_source_data_destroy(
    prepare("{\"type\":\"FeatureCollection\",\"features\":[]}", &clustered)
  );
}

// The submit-time lease keeps the prepared index alive, so one handle serves
// any number of sources, each of which keeps its own reference after the host
// destroys the handle. A destroyed handle rejects new installs at submission.
static void one_prepared_handle_serves_many_sources_and_outlives_itself(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(point_style_json)
  );
  static const char* const sources[] = {"first", "second", "third"};
  mln_geojson_source_data data = prepare(POINT_COLLECTION("shared"), NULL);
  for (size_t index = 0; index < 3; index += 1) {
    mln_test_completion completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_map_add_geojson_source_data(
                       map, mln_test_view_of(sources[index]), data,
                       &completion.descriptor, NULL
                     )
    );
    // The last install is still pending when the handle goes away.
    if (index == 2) {
      mln_geojson_source_data_destroy(data);
    }
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_test_completion_settle(&completion)
    );
    draw_source(map, sources[index]);
  }

  // A second destroy is a no-op, and the dead handle installs nowhere.
  mln_geojson_source_data_destroy(data);
  mln_geojson_source_data_destroy(MLN_HANDLE_NULL);
  mln_completion rejected = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(
      map, MLN_BUFFER_LITERAL("first"), data, &rejected, NULL
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_data(
      map, MLN_BUFFER_LITERAL("fourth"), data, &rejected, NULL
    )
  );

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  source_query query = {
    .fixture = &fixture,
    .sources = sources,
    .source_count = 3,
    .name = "shared",
  };
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, sources_hold_the_name, &query, "every source's feature"
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

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER |
                     MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  clustered.cluster = true;
  clustered.cluster_properties = MLN_BUFFER_LITERAL(
    "{\"total\":[\"+\",[\"get\",\"rank\"]],\"top\":[\"max\",[\"get\","
    "\"rank\"]]}"
  );
  mln_geojson_source_data source_data = prepare(points, &clustered);
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_geojson_source_data(
                     map, MLN_BUFFER_LITERAL("clustered"), source_data,
                     &completion.descriptor, NULL
                   )
  );
  mln_geojson_source_data_destroy(source_data);

  // Data prepared without clustering tiles inconsistently with the source.
  mln_geojson_source_data plain = prepare(points, NULL);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "do not match",
    mln_map_set_geojson_source_data(
      map, MLN_BUFFER_LITERAL("clustered"), plain, &completion.descriptor, NULL
    )
  );
  mln_geojson_source_data_destroy(plain);

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
    mln_geojson_source_options variant = clustered;
    if (cases[index].cluster_properties != NULL) {
      variant.cluster_properties =
        mln_test_view_of(cases[index].cluster_properties);
    }
    mln_geojson_source_data data = prepare(points, &variant);
    mln_test_completion completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_map_set_geojson_source_data(
        map, MLN_BUFFER_LITERAL("clustered"), data, &completion.descriptor, NULL
      )
    );
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
    mln_geojson_source_options variant = clustered;
    variant.fields |= fields[index].field;
    fields[index].change(&variant);
    mln_geojson_source_data data = prepare(points, &variant);
    MLN_TEST_EXPECT_COMMAND_FAILED(
      MLN_STATUS_INVALID_ARGUMENT, "do not match",
      mln_map_set_geojson_source_data(
        map, MLN_BUFFER_LITERAL("clustered"), data, &completion.descriptor, NULL
      )
    );
    mln_geojson_source_data_destroy(data);
  }

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The synchronous tiling override belongs to GeoJSON sources alone.
// internal/geojson_tiling.cpp shows that it reaches the next frame.
static void the_synchronous_tiling_override_rejects_other_sources(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(point_style_json)
  );
  const mln_buffer_view tiles[] = {
    MLN_BUFFER_LITERAL("fixture://tiles/{z}/{x}/{y}.mvt"),
  };
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_vector_source_tiles(
                     map, MLN_BUFFER_LITERAL("vector"), tiles, 1, NULL,
                     &completion.descriptor, NULL
                   )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not a GeoJSON source",
    mln_map_set_geojson_source_synchronous_tiling(
      map, MLN_BUFFER_LITERAL("vector"), true, &completion.descriptor, NULL
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
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
  mln_test_load_style_and_wait(
    runtime, map, MLN_BUFFER_LITERAL(point_style_json)
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_geojson_source_url(
                     map, MLN_BUFFER_LITERAL("points"),
                     MLN_BUFFER_LITERAL("fixture://first.geojson"), NULL,
                     &completion.descriptor, NULL
                   )
  );
  draw_source(map, "points");

  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  static const char* const sources[] = {"points"};
  source_query query = {
    .fixture = &fixture,
    .sources = sources,
    .source_count = 1,
    .name = "first",
  };
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, sources_hold_the_name, &query, "the first URL's feature"
  ));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&routes[0].requests));

  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_set_geojson_source_url(
                     map, MLN_BUFFER_LITERAL("points"),
                     MLN_BUFFER_LITERAL("fixture://second.geojson"),
                     &completion.descriptor, NULL
                   )
  );
  query.name = "second";
  TEST_ASSERT_TRUE(mln_test_style_render_until(
    &fixture, sources_hold_the_name, &query, "the second URL's feature"
  ));
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&routes[1].requests));

  // A URL update belongs to GeoJSON sources alone.
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK, mln_map_add_image_source_image(
                     map, MLN_BUFFER_LITERAL("image"),
                     (const mln_lat_lng[4]){
                       {.latitude = 1.0, .longitude = 0.0},
                       {.latitude = 1.0, .longitude = 1.0},
                       {.latitude = 0.0, .longitude = 1.0},
                       {.latitude = 0.0, .longitude = 0.0},
                     },
                     4,
                     &(const mln_premultiplied_rgba8_image){
                       .size = sizeof(mln_premultiplied_rgba8_image),
                       .width = 1,
                       .height = 1,
                       .stride = 4,
                       .pixels = (const uint8_t[4]){0, 0, 0, 0},
                       .byte_length = 4,
                     },
                     &completion.descriptor, NULL
                   )
  );
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT, "not a GeoJSON source",
    mln_map_set_geojson_source_url(
      map, MLN_BUFFER_LITERAL("image"),
      MLN_BUFFER_LITERAL("fixture://first.geojson"), &completion.descriptor,
      NULL
    )
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(geojson_source_data_create_rejects_unsafe_raw_values);
  RUN_TEST(clustered_geojson_data_reports_non_point_geometry);
  RUN_TEST(clustered_geojson_data_requires_a_feature_collection);
  RUN_TEST(one_prepared_handle_serves_many_sources_and_outlives_itself);
  RUN_TEST(prepared_data_must_match_the_source_options);
  RUN_TEST(the_synchronous_tiling_override_rejects_other_sources);
  RUN_TEST(a_url_source_loads_through_the_resource_provider);
}
