// Option fields of the GeoJSON and custom sources: every field a caller sets
// reaches validation, so a value out of range fails naming its field, and a
// source whose every field is set in range is added.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "support/harness.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static const char point_collection[] =
  "{\"type\":\"FeatureCollection\",\"features\":[{\"type\":\"Feature\","
  "\"geometry\":{\"type\":\"Point\",\"coordinates\":[0,0]},"
  "\"properties\":{}}]}";

// Every GeoJSON option field, each set to a value in range.
static mln_geojson_source_options every_geojson_option(void) {
  mln_geojson_source_options options = mln_geojson_source_options_default();
  options.fields =
    MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM | MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM |
    MLN_GEOJSON_SOURCE_OPTION_TOLERANCE |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES |
    MLN_GEOJSON_SOURCE_OPTION_TILE_SIZE | MLN_GEOJSON_SOURCE_OPTION_BUFFER |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_RADIUS |
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MIN_POINTS |
    MLN_GEOJSON_SOURCE_OPTION_LINE_METRICS | MLN_GEOJSON_SOURCE_OPTION_CLUSTER |
    MLN_GEOJSON_SOURCE_OPTION_SYNCHRONOUS_TILING;
  options.min_zoom = 2.0;
  options.max_zoom = 14.0;
  options.tolerance = 0.5;
  options.cluster_max_zoom = 12.0;
  options.cluster_properties =
    MLN_BUFFER_LITERAL("{\"total\":[\"+\",[\"get\",\"weight\"]]}");
  options.tile_size = 256;
  options.buffer = 64;
  options.cluster_radius = 40;
  options.cluster_min_points = 3;
  options.line_metrics = false;
  options.cluster = true;
  options.synchronous_tiling = true;
  return options;
}

static void inverted_zoom_range(mln_geojson_source_options* options) {
  options->min_zoom = 10.0;
  options->max_zoom = 4.0;
}
static void negative_tolerance(mln_geojson_source_options* options) {
  options->tolerance = -1.0;
}
static void fractional_geojson_max_zoom(mln_geojson_source_options* options) {
  options->max_zoom = 2.5;
}
static void cluster_zoom_out_of_range(mln_geojson_source_options* options) {
  options->cluster_max_zoom = 300.0;
}
static void fractional_cluster_zoom(mln_geojson_source_options* options) {
  options->cluster_max_zoom = 1.5;
}
static void oversized_cluster_radius(mln_geojson_source_options* options) {
  options->cluster_radius = 70000;
}
static void empty_tiles(mln_geojson_source_options* options) {
  options->tile_size = 0;
}
static void oversized_buffer(mln_geojson_source_options* options) {
  options->buffer = 70000;
}
static void cluster_properties_array(mln_geojson_source_options* options) {
  options->cluster_properties = MLN_BUFFER_LITERAL("[]");
}

typedef struct geojson_option_case {
  const char* label;
  void (*mutate)(mln_geojson_source_options* options);
  const char* fragment;
} geojson_option_case;

static const geojson_option_case geojson_option_cases[] = {
  {"inverted zoom range", inverted_zoom_range,
   "min_zoom must be less than or equal to max_zoom"},
  {"fractional max zoom", fractional_geojson_max_zoom, "max_zoom"},
  {"negative tolerance", negative_tolerance, "tolerance"},
  {"cluster zoom out of range", cluster_zoom_out_of_range, "cluster_max_zoom"},
  {"fractional cluster zoom", fractional_cluster_zoom, "cluster_max_zoom"},
  {"empty tiles", empty_tiles, "tile_size"},
  {"oversized buffer", oversized_buffer, "buffer"},
  {"oversized cluster radius", oversized_cluster_radius, "cluster_radius"},
  {"cluster properties that are not an object", cluster_properties_array,
   "cluster_properties must contain a JSON object"},
};

static void geojson_data_validates_every_option_field(void) {
  mln_geojson_source_options options = every_geojson_option();
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_geojson_source_data_create(
    MLN_BUFFER_LITERAL(point_collection), &options, &data, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, data);
  mln_geojson_source_data_destroy(data);

  for (size_t index = 0;
       index < sizeof(geojson_option_cases) / sizeof(geojson_option_cases[0]);
       index += 1) {
    const geojson_option_case* row = &geojson_option_cases[index];
    options = every_geojson_option();
    row->mutate(&options);
    data = MLN_HANDLE_NULL;
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_geojson_source_data_create(
        MLN_BUFFER_LITERAL(point_collection), &options, &data,
        MLN_TEST_DIAGNOSTIC
      ),
      row->label
    );
    TEST_ASSERT_EQUAL_UINT64_MESSAGE(MLN_HANDLE_NULL, data, row->label);
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(mln_test_last_error(), row->fragment), row->label
    );
  }
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

  mln_geojson_source_options options = every_geojson_option();
  mln_geojson_source_data data = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_geojson_source_data_create(
    MLN_BUFFER_LITERAL(point_collection), &options, &data, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_data(
    map, MLN_BUFFER_LITERAL("prepared"), data, &completion.descriptor, NULL
  ));
  mln_geojson_source_data_destroy(data);

  MLN_TEST_AWAIT_OK(mln_map_add_geojson_source_url(
    map, MLN_BUFFER_LITERAL("points"),
    MLN_BUFFER_LITERAL("fixture://points.geojson"), &options,
    &completion.descriptor, NULL
  ));

  inverted_zoom_range(&options);
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "min_zoom must be less than or equal to max_zoom",
    mln_map_add_geojson_source_url(
      map, MLN_BUFFER_LITERAL("inverted"),
      MLN_BUFFER_LITERAL("fixture://points.geojson"), &options,
      &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );

  options = every_geojson_option();
  cluster_properties_array(&options);
  MLN_TEST_EXPECT_COMMAND_FAILED(
    MLN_STATUS_INVALID_ARGUMENT,
    "cluster_properties must contain a JSON object",
    mln_map_add_geojson_source_url(
      map, MLN_BUFFER_LITERAL("array-properties"),
      MLN_BUFFER_LITERAL("fixture://points.geojson"), &options,
      &completion.descriptor, NULL
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void ignore_tile(void* user_data, mln_canonical_tile_id tile_id) {
  (void)user_data;
  (void)tile_id;
}

// Every custom geometry option field, each set to a value in range.
static mln_custom_geometry_source_options every_geometry_option(void) {
  mln_custom_geometry_source_options options =
    mln_custom_geometry_source_options_default();
  options.fields = MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MIN_ZOOM |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_MAX_ZOOM |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TOLERANCE |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_TILE_SIZE |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_BUFFER |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_CLIP |
                   MLN_CUSTOM_GEOMETRY_SOURCE_OPTION_WRAP;
  options.fetch_tile = ignore_tile;
  options.min_zoom = 1.0;
  options.max_zoom = 12.0;
  options.tolerance = 0.5;
  options.tile_size = 256;
  options.buffer = 64;
  options.clip = true;
  options.wrap = true;
  return options;
}

typedef struct geometry_option_case {
  const char* label;
  void (*mutate)(mln_custom_geometry_source_options* options);
  const char* fragment;
} geometry_option_case;

static void fractional_max_zoom(mln_custom_geometry_source_options* options) {
  options->max_zoom = 4.5;
}
static void inverted_geometry_zoom(
  mln_custom_geometry_source_options* options
) {
  options->min_zoom = 10.0;
  options->max_zoom = 4.0;
}
static void negative_geometry_tolerance(
  mln_custom_geometry_source_options* options
) {
  options->tolerance = -1.0;
}
static void empty_geometry_tiles(mln_custom_geometry_source_options* options) {
  options->tile_size = 0;
}
static void oversized_geometry_buffer(
  mln_custom_geometry_source_options* options
) {
  options->buffer = 70000;
}

static const geometry_option_case geometry_option_cases[] = {
  {"fractional max zoom", fractional_max_zoom,
   "max_zoom must be an integer within [0, 32]"},
  {"inverted zoom range", inverted_geometry_zoom,
   "min_zoom must be less than or equal to max_zoom"},
  {"negative tolerance", negative_geometry_tolerance, "tolerance"},
  {"empty tiles", empty_geometry_tiles, "tile_size"},
  {"oversized buffer", oversized_geometry_buffer, "buffer"},
};

// Every custom MVT vector option field, each set to a value in range.
static mln_custom_mvt_vector_source_options every_mvt_option(void) {
  mln_custom_mvt_vector_source_options options =
    mln_custom_mvt_vector_source_options_default();
  options.fields = MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MIN_ZOOM |
                   MLN_CUSTOM_MVT_VECTOR_SOURCE_OPTION_MAX_ZOOM;
  options.fetch_tile = ignore_tile;
  options.min_zoom = 1.0;
  options.max_zoom = 12.0;
  return options;
}

static void custom_sources_validate_every_option_field(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(runtime, map, mln_test_empty_style_json);

  mln_custom_geometry_source_options geometry = every_geometry_option();
  MLN_TEST_AWAIT_OK(mln_map_add_custom_geometry_source(
    map, MLN_BUFFER_LITERAL("geometry"), &geometry, &completion.descriptor, NULL
  ));
  for (size_t index = 0;
       index < sizeof(geometry_option_cases) / sizeof(geometry_option_cases[0]);
       index += 1) {
    const geometry_option_case* row = &geometry_option_cases[index];
    geometry = every_geometry_option();
    row->mutate(&geometry);
    char id[24];
    snprintf(id, sizeof(id), "geometry-%zu", index);
    mln_test_completion completion = mln_test_completion_default(0);
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_map_add_custom_geometry_source(
        map, mln_test_view_of(id), &geometry, &completion.descriptor,
        MLN_TEST_DIAGNOSTIC
      ),
      row->label
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(mln_test_last_error(), row->fragment), row->label
    );
    mln_test_completion_reject(&completion);
    mln_test_completion_destroy(&completion);
  }

  mln_custom_mvt_vector_source_options mvt = every_mvt_option();
  MLN_TEST_AWAIT_OK(mln_map_add_custom_mvt_vector_source(
    map, MLN_BUFFER_LITERAL("mvt"), &mvt, &completion.descriptor, NULL
  ));
  mvt.max_zoom = 4.5;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "max_zoom must be an integer within [0, 32]",
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL("mvt-fractional"), &mvt, &completion.descriptor,
      MLN_TEST_DIAGNOSTIC
    )
  );
  mvt = every_mvt_option();
  mvt.min_zoom = 10.0;
  mvt.max_zoom = 4.0;
  MLN_TEST_EXPECT_COMMAND_REJECTED(
    "min_zoom must be less than or equal to max_zoom",
    mln_map_add_custom_mvt_vector_source(
      map, MLN_BUFFER_LITERAL("mvt-inverted"), &mvt, &completion.descriptor,
      MLN_TEST_DIAGNOSTIC
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(geojson_data_validates_every_option_field);
  RUN_TEST(geojson_sources_take_and_validate_their_options);
  RUN_TEST(custom_sources_validate_every_option_field);
}
