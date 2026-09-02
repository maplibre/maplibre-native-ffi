// Raw C ABI coverage: malformed descriptor counts and unknown enum values are
// hidden by binding-owned style values.

#include <math.h>
#include <string.h>

#include "abi_tests.h"
#include "maplibre_native_c/callback_adapter.h"
#include "test_support.h"
#include "unity.h"

#define MLN_STRING_LITERAL(text) \
  ((mln_buffer_view){.data = (text), .size = sizeof(text) - 1})

static void style_value_helpers_reject_unsafe_raw_descriptors(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_lat_lng coordinates[] = {
    {.latitude = 38.0, .longitude = -123.0},
    {.latitude = 38.0, .longitude = -122.0},
    {.latitude = 37.0, .longitude = -122.0},
    {.latitude = 37.0, .longitude = -123.0},
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_image_source_coordinates(
      map, (mln_buffer_view){.data = "image-url-source", .size = 16},
      coordinates, 3
    )
  );

  mln_style_tile_source_options options =
    mln_style_tile_source_options_default();
  options.fields = MLN_STYLE_TILE_SOURCE_OPTION_RASTER_ENCODING;
  options.raster_encoding = 99;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_raster_dem_source_url(
      map, (mln_buffer_view){.data = "bad-dem", .size = 7},
      (mln_buffer_view){.data = "https://example.com/bad.json", .size = 28},
      &options
    )
  );

  static const char nul_style[] =
    "{\"version\":8,\"sources\":{},\"layers\":[]}\0garbage";
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_style_json(
                                   map, (mln_buffer_view){
                                          .data = nul_style,
                                          .size = sizeof(nul_style) - 1,
                                        }
                                 )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Bindings emit a full struct header and one complete cluster-properties
// object, so only raw C callers can present these unsafe values.
static void geojson_source_options_reject_unsafe_raw_values(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view url = {
    .data = "https://example.com/points.geojson", .size = 34
  };

  mln_geojson_source_options short_size = mln_geojson_source_options_default();
  short_size.size = (uint32_t)(sizeof(mln_geojson_source_options) - 1);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_buffer_view){.data = "short-header", .size = 12}, url,
      &short_size
    )
  );

  mln_geojson_source_options unknown_field =
    mln_geojson_source_options_default();
  unknown_field.fields = 1U << 31U;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_buffer_view){.data = "unknown-field", .size = 13}, url,
      &unknown_field
    )
  );

  mln_geojson_source_options fractional_min_zoom =
    mln_geojson_source_options_default();
  fractional_min_zoom.fields = MLN_GEOJSON_SOURCE_OPTION_MIN_ZOOM;
  fractional_min_zoom.min_zoom = 1.5;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_buffer_view){.data = "fractional-min", .size = 14}, url,
      &fractional_min_zoom
    )
  );

  mln_geojson_source_options fractional_max_zoom =
    mln_geojson_source_options_default();
  fractional_max_zoom.fields = MLN_GEOJSON_SOURCE_OPTION_MAX_ZOOM;
  fractional_max_zoom.max_zoom = 17.9;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_buffer_view){.data = "fractional-max", .size = 14}, url,
      &fractional_max_zoom
    )
  );

  mln_geojson_source_options fractional_cluster_max_zoom =
    mln_geojson_source_options_default();
  fractional_cluster_max_zoom.fields =
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_MAX_ZOOM;
  fractional_cluster_max_zoom.cluster_max_zoom = 12.25;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_buffer_view){.data = "fractional-cluster", .size = 18}, url,
      &fractional_cluster_max_zoom
    )
  );

  mln_geojson_source_options null_cluster_properties =
    mln_geojson_source_options_default();
  null_cluster_properties.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_buffer_view){.data = "null-properties", .size = 15}, url,
      &null_cluster_properties
    )
  );

  mln_geojson_source_options non_object_cluster_properties =
    mln_geojson_source_options_default();
  non_object_cluster_properties.fields =
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  non_object_cluster_properties.cluster_properties = MLN_BUFFER_LITERAL("null");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_buffer_view){.data = "json-null-properties", .size = 20}, url,
      &non_object_cluster_properties
    )
  );

  static const char injected_cluster_properties[] =
    "{\"total\":[\"+\",1]},\"cluster\":true";
  mln_geojson_source_options injected_properties =
    mln_geojson_source_options_default();
  injected_properties.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  injected_properties.cluster_properties = (mln_buffer_view){
    .data = injected_cluster_properties,
    .size = sizeof(injected_cluster_properties) - 1,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, MLN_BUFFER_LITERAL("injected-properties"), url, &injected_properties
    )
  );

  static const char trailing_cluster_properties[] =
    "{\"total\":[\"+\",1]}\0garbage";
  mln_geojson_source_options trailing_properties =
    mln_geojson_source_options_default();
  trailing_properties.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  trailing_properties.cluster_properties = (mln_buffer_view){
    .data = trailing_cluster_properties,
    .size = sizeof(trailing_cluster_properties) - 1,
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, MLN_BUFFER_LITERAL("trailing-properties"), url, &trailing_properties
    )
  );

  static const char trailing_geojson[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}garbage";
  mln_geojson_source_data trailing_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_geojson_source_data_create(
                                   (mln_buffer_view){
                                     .data = trailing_geojson,
                                     .size = sizeof(trailing_geojson) - 1,
                                   },
                                   NULL, &trailing_data
                                 )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, trailing_data);

  static const char nul_geojson[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}\0garbage";
  mln_geojson_source_data nul_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_geojson_source_data_create(
                                   (mln_buffer_view){
                                     .data = nul_geojson,
                                     .size = sizeof(nul_geojson) - 1,
                                   },
                                   NULL, &nul_data
                                 )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, nul_data);

  // A populated output handle is rejected rather than silently overwritten.
  mln_geojson_source_data populated = MLN_HANDLE_NULL;
  static const char empty_collection[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}";
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(empty_collection), NULL, &populated
                   )
  );
  mln_geojson_source_data reused = populated;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(empty_collection), NULL, &reused
    )
  );
  mln_geojson_source_data_destroy(populated);

  // Unsafe raw options reject data preparation the same way they reject a
  // URL add.
  mln_geojson_source_data short_size_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(empty_collection), &short_size, &short_size_data
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, short_size_data);

  // A rejected descriptor leaves the source ID free for a later valid add.
  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_geojson_source_url(
                     map, (mln_buffer_view){.data = "short-header", .size = 12},
                     url, &clustered
                   )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
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
      MLN_BUFFER_LITERAL(data), &clustered, &prepared
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, prepared);

  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "point geometry on every feature"));
  TEST_ASSERT_NOT_NULL(strstr(message, "feature 1"));
  TEST_ASSERT_NOT_NULL(strstr(message, "geometry collection"));

  // The constraint belongs to clustering alone, so the same data tiles fine
  // without it.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_geojson_source_data_create(MLN_BUFFER_LITERAL(data), NULL, &prepared)
  );
  mln_geojson_source_data_destroy(prepared);
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
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(bare_geometry), &clustered, &prepared
    )
  );
  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "requires a feature collection"));
  TEST_ASSERT_NOT_NULL(strstr(message, "a bare geometry"));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(single_feature), &clustered, &prepared
    )
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_thread_last_error_message(), "a single feature")
  );

  // The constraint belongs to clustering alone, so the same data tiles fine
  // without it.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(bare_geometry), NULL, &prepared
                   )
  );
  mln_geojson_source_data_destroy(prepared);
  prepared = MLN_HANDLE_NULL;

  // An empty feature collection carries nothing to cluster, so it stays
  // accepted and a later update supplies the features to cluster.
  static const char empty[] =
    "{\"type\":\"FeatureCollection\",\"features\":[]}";
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(empty), &clustered, &prepared
                   )
  );
  mln_geojson_source_data_destroy(prepared);
}

// Prepared data installs on any number of sources; an update requires data
// whose baked-in options match the source's, and released handles reject
// installs while installed sources keep their own reference.
static void prepared_geojson_data_installs_and_checks_options(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  static const char points[] =
    "{\"type\":\"FeatureCollection\",\"features\":["
    "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Point\","
    "\"coordinates\":[-122.5,37.7]},\"properties\":{}}]}";

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;

  mln_geojson_source_data plain_data = MLN_HANDLE_NULL;
  mln_geojson_source_data clustered_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(points), NULL, &plain_data
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(points), &clustered, &clustered_data
                   )
  );

  const mln_buffer_view plain_id = {.data = "plain", .size = 5};
  const mln_buffer_view clustered_id = {.data = "clustered", .size = 9};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_geojson_source_data(map, plain_id, plain_data)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, clustered_id, clustered_data)
  );

  // Data prepared under different options tiles inconsistently with the
  // source, so the mismatch is rejected and names the source.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(map, clustered_id, plain_data)
  );
  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "\"clustered\""));
  TEST_ASSERT_NOT_NULL(strstr(message, "do not match"));

  // One prepared handle installs on any number of matching sources.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_data(map, clustered_id, clustered_data)
  );

  // Cluster aggregations are part of the options match: a different
  // expression is rejected, while equivalent JSON with different formatting
  // compares equal by parsed expression.
  mln_geojson_source_options aggregated = clustered;
  aggregated.fields |= MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  aggregated.cluster_properties =
    MLN_BUFFER_LITERAL("{\"total\":[\"+\",[\"get\",\"rank\"]]}");
  const mln_buffer_view aggregated_id = {.data = "aggregated", .size = 10};
  mln_geojson_source_data aggregated_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(points), &aggregated, &aggregated_data
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, aggregated_id, aggregated_data)
  );
  mln_geojson_source_options reaggregated = aggregated;
  reaggregated.cluster_properties =
    MLN_BUFFER_LITERAL("{\"total\":[\"max\",[\"get\",\"rank\"]]}");
  mln_geojson_source_data reaggregated_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_geojson_source_data_create(
      MLN_BUFFER_LITERAL(points), &reaggregated, &reaggregated_data
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(map, aggregated_id, reaggregated_data)
  );
  mln_geojson_source_data_destroy(reaggregated_data);
  mln_geojson_source_options reformatted = aggregated;
  reformatted.cluster_properties =
    MLN_BUFFER_LITERAL(" { \"total\" : [\"+\", [\"get\", \"rank\"]] } ");
  mln_geojson_source_data reformatted_data = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_geojson_source_data_create(
                     MLN_BUFFER_LITERAL(points), &reformatted, &reformatted_data
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_data(map, aggregated_id, reformatted_data)
  );
  mln_geojson_source_data_destroy(reformatted_data);
  mln_geojson_source_data_destroy(aggregated_data);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_geojson_source_data(map, plain_id, plain_data)
  );

  // The runtime override takes a live GeoJSON source alone.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_synchronous_tiling(map, plain_id, true)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_geojson_source_synchronous_tiling(map, plain_id, false)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_synchronous_tiling(
      map, (mln_buffer_view){.data = "missing", .size = 7}, true
    )
  );

  // Destroy releases the handle while installed sources keep their reference;
  // the released id then rejects installs, and a second destroy is a no-op.
  mln_geojson_source_data_destroy(plain_data);
  mln_geojson_source_data_destroy(plain_data);
  mln_geojson_source_data_destroy(MLN_HANDLE_NULL);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(map, plain_id, plain_data)
  );
  mln_geojson_source_data_destroy(clustered_data);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A vector source plus one layer that takes a source and one that does not, so
// the typed layer accessors can be exercised against both.
static const char layer_accessor_style_json[] =
  "{\"version\":8,\"sources\":{\"vec\":{\"type\":\"vector\","
  "\"tiles\":[\"https://example.com/{z}/{x}/{y}.mvt\"]}},"
  "\"layers\":[{\"id\":\"lines\",\"type\":\"line\",\"source\":\"vec\","
  "\"source-layer\":\"roads\"},{\"id\":\"bg\",\"type\":\"background\"}]}";

static void optional_json_style_values_return_null_handles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(layer_accessor_style_json))
  );

  mln_buffer value = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_light_property(
                     map, MLN_STRING_LITERAL("intensity"), &value
                   )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, value);

  const mln_buffer_view layer_id = MLN_STRING_LITERAL("bg");
  const mln_buffer_view property_name =
    MLN_STRING_LITERAL("background-opacity");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_layer_property(map, layer_id, property_name, &value)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, value);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_filter(map, layer_id, &value)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, value);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_layer_property(
                     map, layer_id, property_name, MLN_BUFFER_LITERAL("0.5")
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_layer_property(map, layer_id, property_name, &value)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, value);
  mln_buffer_destroy(value);
  value = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_layer_property(
                     map, layer_id, property_name, MLN_BUFFER_LITERAL("null")
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_layer_property(map, layer_id, property_name, &value)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, value);

  const mln_buffer_view filter =
    MLN_BUFFER_LITERAL("[\"==\",[\"get\",\"kind\"],\"park\"]");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_layer_filter(map, layer_id, &filter)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_filter(map, layer_id, &value)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, value);
  mln_buffer_destroy(value);
  value = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_layer_filter(map, layer_id, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_filter(map, layer_id, &value)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, value);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// MapLibre's own setProperty path accepts a sourceless layer as a silent no-op;
// the typed accessors reject it.
static void layer_source_accessors_reject_sourceless_layer_types(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(layer_accessor_style_json))
  );

  const mln_buffer_view background = MLN_STRING_LITERAL("bg");
  const mln_buffer_view source_layer = MLN_STRING_LITERAL("roads");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_layer_source_layer(map, background, source_layer)
  );
  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "source-layer"));

  const mln_buffer_view source_id = MLN_STRING_LITERAL("vec");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_layer_source_id(map, background, source_id)
  );

  // The style-spec property path still reaches the same layer and reports OK
  // without changing it, which is why the typed setters exist.
  const mln_buffer_view property_name = MLN_STRING_LITERAL("source-layer");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_layer_property(
      map, background, property_name, MLN_BUFFER_LITERAL("\"roads\"")
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void layer_text_accessors_report_required_capacity(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(layer_accessor_style_json))
  );

  // A null buffer with zero capacity is a size probe, so it reports the length
  // and succeeds rather than sharing a status with a missing layer.
  const mln_buffer_view lines = MLN_STRING_LITERAL("lines");
  size_t required = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_layer_source_layer(map, lines, NULL, 0, &required)
  );
  TEST_ASSERT_EQUAL_size_t(5, required);

  char too_small[4] = {0};
  required = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_layer_source_layer(
      map, lines, too_small, sizeof(too_small), &required
    )
  );
  TEST_ASSERT_EQUAL_size_t(5, required);

  char buffer[8] = {0};
  required = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_layer_source_layer(
                     map, lines, buffer, sizeof(buffer), &required
                   )
  );
  TEST_ASSERT_EQUAL_size_t(5, required);
  TEST_ASSERT_EQUAL_INT(0, memcmp(buffer, "roads", 5));

  // A sourceless layer reads back as empty rather than failing.
  const mln_buffer_view background = MLN_STRING_LITERAL("bg");
  required = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_layer_source_id(
                     map, background, buffer, sizeof(buffer), &required
                   )
  );
  TEST_ASSERT_EQUAL_size_t(0, required);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_layer_source_layer(map, lines, buffer, sizeof(buffer), NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_layer_source_layer(map, lines, NULL, sizeof(buffer), &required)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// An unbounded zoom range crosses the ABI as infinities.
static void layer_zoom_and_visibility_accessors_carry_raw_domains(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(layer_accessor_style_json))
  );

  const mln_buffer_view lines = MLN_STRING_LITERAL("lines");
  double min_zoom = 0.0;
  double max_zoom = 0.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_min_zoom(map, lines, &min_zoom)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_max_zoom(map, lines, &max_zoom)
  );
  TEST_ASSERT_TRUE(isinf(min_zoom) && min_zoom < 0.0);
  TEST_ASSERT_TRUE(isinf(max_zoom) && max_zoom > 0.0);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_layer_min_zoom(map, lines, 4.0)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_layer_max_zoom(map, lines, 12.5)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_min_zoom(map, lines, &min_zoom)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_max_zoom(map, lines, &max_zoom)
  );
  TEST_ASSERT_EQUAL_DOUBLE(4.0, min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(12.5, max_zoom);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_layer_min_zoom(map, lines, NAN)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_layer_max_zoom(map, lines, NAN)
  );

  uint32_t visibility = 999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_visibility(map, lines, &visibility)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_LAYER_VISIBILITY_VISIBLE, visibility);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_layer_visibility(map, lines, MLN_STYLE_LAYER_VISIBILITY_NONE)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_layer_visibility(map, lines, &visibility)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_LAYER_VISIBILITY_NONE, visibility);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_layer_visibility(map, lines, 999)
  );

  // A missing layer is rejected the same way the rest of the layer family does.
  const mln_buffer_view missing = MLN_STRING_LITERAL("nope");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_layer_min_zoom(map, missing, &min_zoom)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void style_image_stretch_descriptors_reject_unsafe_raw_values(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL(layer_accessor_style_json))
  );

  const uint8_t pixels[4 * 4] = {0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 2;
  image.height = 2;
  image.stride = 8;
  image.pixels = pixels;
  image.byte_length = sizeof(pixels);

  const mln_buffer_view image_id = MLN_STRING_LITERAL("patch");

  // A backwards interval, a non-finite bound, and a null array with a non-zero
  // count are all rejected.
  const mln_image_stretch backwards[] = {{.from = 2.0F, .to = 1.0F}};
  mln_style_image_options options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  options.stretch_x = backwards;
  options.stretch_x_count = 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  // A zero-width interval would leave MapLibre dividing by a zero stretch sum.
  const mln_image_stretch zero_width[] = {{.from = 1.0F, .to = 1.0F}};
  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  options.stretch_x = zero_width;
  options.stretch_x_count = 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  // Intervals that run out of order or overlap their predecessor are rejected.
  const mln_image_stretch unordered[] = {
    {.from = 2.0F, .to = 3.0F}, {.from = 0.0F, .to = 1.0F}
  };
  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_X;
  options.stretch_x = unordered;
  options.stretch_x_count = 2;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  const mln_image_stretch overlapping[] = {
    {.from = 0.0F, .to = 2.0F}, {.from = 1.0F, .to = 3.0F}
  };
  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
  options.stretch_y = overlapping;
  options.stretch_y_count = 2;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_STRETCH_Y;
  options.stretch_y = NULL;
  options.stretch_y_count = 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_CONTENT;
  options.content.left = 2.0F;
  options.content.right = 1.0F;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  options = mln_style_image_options_default();
  options.fields = MLN_STYLE_IMAGE_OPTION_TEXT_FIT_WIDTH;
  options.text_fit_width = 999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_image(map, image_id, &image, &options)
  );

  // An accepted nine-patch reports its stretches, content, and text fit back.
  const mln_image_stretch stretch_x[] = {{.from = 0.0F, .to = 1.0F}};
  const mln_image_stretch stretch_y[] = {
    {.from = 0.0F, .to = 1.0F}, {.from = 1.0F, .to = 2.0F}
  };
  options = mln_style_image_options_default();
  options.fields =
    MLN_STYLE_IMAGE_OPTION_STRETCH_X | MLN_STYLE_IMAGE_OPTION_STRETCH_Y |
    MLN_STYLE_IMAGE_OPTION_CONTENT | MLN_STYLE_IMAGE_OPTION_TEXT_FIT_HEIGHT;
  options.stretch_x = stretch_x;
  options.stretch_x_count = 1;
  options.stretch_y = stretch_y;
  options.stretch_y_count = 2;
  options.content.left = 0.5F;
  options.content.top = 0.5F;
  options.content.right = 1.5F;
  options.content.bottom = 1.5F;
  options.text_fit_height = MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_image(map, image_id, &image, &options)
  );

  mln_style_image_info info = mln_style_image_info_default();
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_image_info(map, image_id, &info, &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(1, info.stretch_x_count);
  TEST_ASSERT_EQUAL_size_t(2, info.stretch_y_count);
  TEST_ASSERT_TRUE(info.has_content);
  TEST_ASSERT_EQUAL_FLOAT(0.5F, info.content.left);
  TEST_ASSERT_EQUAL_FLOAT(1.5F, info.content.bottom);
  TEST_ASSERT_FALSE(info.has_text_fit_width);
  TEST_ASSERT_TRUE(info.has_text_fit_height);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_IMAGE_TEXT_FIT_PROPORTIONAL, info.text_fit_height
  );

  // Null arrays with zero capacity probe the counts and succeed.
  size_t x_count = 0;
  size_t y_count = 0;
  found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_stretches(
                     map, image_id, NULL, 0, &x_count, NULL, 0, &y_count, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(1, x_count);
  TEST_ASSERT_EQUAL_size_t(2, y_count);

  // An undersized array reports the counts and fails.
  mln_image_stretch too_small[1] = {{.from = 0.0F, .to = 0.0F}};
  x_count = 0;
  y_count = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_image_stretches(
      map, image_id, NULL, 0, &x_count, too_small, 1, &y_count, &found
    )
  );
  TEST_ASSERT_EQUAL_size_t(2, y_count);

  mln_image_stretch copied_x[1] = {{.from = 0.0F, .to = 0.0F}};
  mln_image_stretch copied_y[2] = {
    {.from = 0.0F, .to = 0.0F}, {.from = 0.0F, .to = 0.0F}
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_image_stretches(
      map, image_id, copied_x, 1, &x_count, copied_y, 2, &y_count, &found
    )
  );
  TEST_ASSERT_EQUAL_FLOAT(1.0F, copied_x[0].to);
  TEST_ASSERT_EQUAL_FLOAT(1.0F, copied_y[1].from);
  TEST_ASSERT_EQUAL_FLOAT(2.0F, copied_y[1].to);

  // A missing image reports zero counts without failing.
  const mln_buffer_view missing = MLN_STRING_LITERAL("nope");
  x_count = 123;
  y_count = 123;
  found = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_stretches(
                     map, missing, NULL, 0, &x_count, NULL, 0, &y_count, &found
                   )
  );
  TEST_ASSERT_FALSE(found);
  TEST_ASSERT_EQUAL_size_t(0, x_count);
  TEST_ASSERT_EQUAL_size_t(0, y_count);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void copy_entry_points_answer_a_null_buffer_as_a_size_probe(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(
      map,
      MLN_BUFFER_LITERAL(
        "{\"version\":8,\"sources\":{\"vec\":{\"type\":\"vector\","
        "\"attribution\":\"probe\","
        "\"tiles\":[\"https://example.com/{z}/{x}/{y}.mvt\"]}},\"layers\":[]}"
      )
    )
  );

  // Source attribution reports its length and succeeds.
  const mln_buffer_view source_id = MLN_STRING_LITERAL("vec");
  size_t attribution_size = 0;
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_source_attribution(
                     map, source_id, NULL, 0, &attribution_size, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(5, attribution_size);

  // A non-null buffer that is too small still reports the length and fails.
  char too_small[2] = {0};
  attribution_size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_source_attribution(
      map, source_id, too_small, sizeof(too_small), &attribution_size, &found
    )
  );
  TEST_ASSERT_EQUAL_size_t(5, attribution_size);

  // Image pixels report their byte length and succeed.
  const uint8_t pixels[4] = {0};
  mln_premultiplied_rgba8_image image = mln_premultiplied_rgba8_image_default();
  image.width = 1;
  image.height = 1;
  image.stride = 4;
  image.pixels = pixels;
  image.byte_length = sizeof(pixels);
  const mln_buffer_view image_id = MLN_STRING_LITERAL("dot");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_image(map, image_id, &image, NULL)
  );

  size_t byte_length = 0;
  found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_image_premultiplied_rgba8(
                     map, image_id, NULL, 0, &byte_length, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(4, byte_length);

  uint8_t small_pixels[2] = {0};
  byte_length = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_image_premultiplied_rgba8(
      map, image_id, small_pixels, sizeof(small_pixels), &byte_length, &found
    )
  );
  TEST_ASSERT_EQUAL_size_t(4, byte_length);

  // The map-level copy entry points answer a probe the same way, with no id to
  // resolve first.
  size_t document_size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_loaded_style_json(map, NULL, 0, &document_size)
  );
  TEST_ASSERT_TRUE(document_size > 0);

  char small_document[2] = {0};
  size_t reported = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_loaded_style_json(
      map, small_document, sizeof(small_document), &reported
    )
  );
  TEST_ASSERT_EQUAL_size_t(document_size, reported);

  size_t url_size = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_url(map, NULL, 0, &url_size)
  );
  TEST_ASSERT_EQUAL_size_t(0, url_size);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Source metadata is split between fixed fields, copy-out strings, and an owned
// list because no caller-owned struct can retain variable-length TileJSON data
// across the ABI.
static void style_source_info_rebuilds_an_inline_tile_source(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(
      map, MLN_BUFFER_LITERAL(
             "{\"version\":8,\"sources\":{\"vec\":{\"type\":\"vector\","
             "\"tiles\":[\"https://a.example/{z}/{x}/{y}.mlt\","
             "\"https://b.example/{z}/{x}/{y}.mlt\"],\"minzoom\":2,"
             "\"maxzoom\":7,\"scheme\":\"tms\",\"bounds\":[-10,-5,20,15],"
             "\"encoding\":\"mlt\",\"attribution\":\"example\"}},"
             "\"layers\":[]}"
           )
    )
  );

  const mln_buffer_view source_id = MLN_STRING_LITERAL("vec");
  mln_style_source_info info = {.size = sizeof(mln_style_source_info)};
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(map, source_id, &info, &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_SOURCE_TYPE_VECTOR, info.type);
  TEST_ASSERT_BITS_HIGH(
    MLN_STYLE_SOURCE_INFO_TILEJSON | MLN_STYLE_SOURCE_INFO_BOUNDS |
      MLN_STYLE_SOURCE_INFO_TILE_SIZE | MLN_STYLE_SOURCE_INFO_VECTOR_ENCODING,
    info.fields
  );
  TEST_ASSERT_BITS_LOW(MLN_STYLE_SOURCE_INFO_URL, info.fields);
  TEST_ASSERT_EQUAL_size_t(2, info.tile_count);
  TEST_ASSERT_EQUAL_DOUBLE(2.0, info.min_zoom);
  TEST_ASSERT_EQUAL_DOUBLE(7.0, info.max_zoom);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_TILE_SCHEME_TMS, info.scheme);
  TEST_ASSERT_EQUAL_DOUBLE(-5.0, info.bounds.southwest.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(-10.0, info.bounds.southwest.longitude);
  TEST_ASSERT_EQUAL_DOUBLE(15.0, info.bounds.northeast.latitude);
  TEST_ASSERT_EQUAL_DOUBLE(20.0, info.bounds.northeast.longitude);
  TEST_ASSERT_EQUAL_UINT32(512, info.tile_size);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_VECTOR_TILE_ENCODING_MLT, info.vector_encoding
  );

  mln_style_string_list tile_urls = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_source_tile_urls(map, source_id, &tile_urls, &found)
  );
  TEST_ASSERT_TRUE(found);
  size_t tile_count = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_style_string_list_count(tile_urls, &tile_count)
  );
  TEST_ASSERT_EQUAL_size_t(2, tile_count);

  mln_buffer_view tiles[2] = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_style_string_list_get(tile_urls, 0, &tiles[0])
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_style_string_list_get(tile_urls, 1, &tiles[1])
  );
  TEST_ASSERT_EQUAL_STRING_LEN(
    "https://a.example/{z}/{x}/{y}.mlt", tiles[0].data, tiles[0].size
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_style_string_list_get(tile_urls, tile_count, &tiles[0])
  );

  char attribution[7] = {0};
  size_t attribution_size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_source_attribution(
                     map, source_id, attribution, sizeof(attribution),
                     &attribution_size, &found
                   )
  );
  TEST_ASSERT_EQUAL_size_t(7, attribution_size);
  TEST_ASSERT_EQUAL_STRING_LEN("example", attribution, attribution_size);

  mln_style_tile_source_options options =
    mln_style_tile_source_options_default();
  options.fields = MLN_STYLE_TILE_SOURCE_OPTION_MIN_ZOOM |
                   MLN_STYLE_TILE_SOURCE_OPTION_MAX_ZOOM |
                   MLN_STYLE_TILE_SOURCE_OPTION_ATTRIBUTION |
                   MLN_STYLE_TILE_SOURCE_OPTION_SCHEME |
                   MLN_STYLE_TILE_SOURCE_OPTION_BOUNDS |
                   MLN_STYLE_TILE_SOURCE_OPTION_VECTOR_ENCODING;
  options.min_zoom = info.min_zoom;
  options.max_zoom = info.max_zoom;
  options.attribution =
    (mln_buffer_view){.data = attribution, .size = attribution_size};
  options.scheme = info.scheme;
  options.bounds = info.bounds;
  options.vector_encoding = info.vector_encoding;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_vector_source_tiles(
      map, MLN_STRING_LITERAL("rebuilt"), tiles, tile_count, &options
    )
  );

  mln_style_string_list_destroy(tile_urls);
  mln_style_string_list_destroy(tile_urls);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_style_string_list_count(tile_urls, &tile_count)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A URL-backed tile source stays represented by its URL. The resolved remote
// TileJSON is runtime state rather than part of its reconstructible descriptor.
static void style_source_info_reports_url_backed_tile_source(void) {
  static const char source_url[] = "https://example.com/source.json";
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view source_id = MLN_STRING_LITERAL("remote");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_vector_source_url(
                     map, source_id, MLN_STRING_LITERAL(source_url), NULL
                   )
  );

  mln_style_source_info info = {.size = sizeof(mln_style_source_info)};
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(map, source_id, &info, &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_BITS_HIGH(MLN_STYLE_SOURCE_INFO_URL, info.fields);
  TEST_ASSERT_BITS_LOW(MLN_STYLE_SOURCE_INFO_TILEJSON, info.fields);
  TEST_ASSERT_EQUAL_size_t(sizeof(source_url) - 1, info.url_size);

  size_t copied_size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_source_url(map, source_id, NULL, 0, &copied_size, &found)
  );
  TEST_ASSERT_EQUAL_size_t(sizeof(source_url) - 1, copied_size);
  char too_small[2] = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_source_url(
      map, source_id, too_small, sizeof(too_small), &copied_size, &found
    )
  );
  char copied_url[sizeof(source_url)] = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_source_url(
      map, source_id, copied_url, sizeof(copied_url), &copied_size, &found
    )
  );
  TEST_ASSERT_EQUAL_STRING_LEN(source_url, copied_url, copied_size);

  mln_style_string_list urls = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_source_tile_urls(map, source_id, &urls, &found)
  );
  size_t count = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_style_string_list_count(urls, &count)
  );
  TEST_ASSERT_EQUAL_size_t(0, count);
  mln_style_string_list_destroy(urls);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void style_source_volatility_round_trips(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_buffer_view source_id = MLN_STRING_LITERAL("volatile-vector");
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_vector_source_tiles(
                     map, source_id,
                     (mln_buffer_view[]){
                       MLN_STRING_LITERAL("https://example.com/{z}/{x}/{y}.mvt")
                     },
                     1, NULL
                   )
  );

  mln_style_source_info info = {.size = sizeof(mln_style_source_info)};
  bool found = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(map, source_id, &info, &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_FALSE(info.is_volatile);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_source_volatile(map, source_id, true)
  );
  info = (mln_style_source_info){.size = sizeof(mln_style_source_info)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(map, source_id, &info, &found)
  );
  TEST_ASSERT_TRUE(info.is_volatile);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_source_volatile(map, source_id, false)
  );
  info = (mln_style_source_info){.size = sizeof(mln_style_source_info)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(map, source_id, &info, &found)
  );
  TEST_ASSERT_FALSE(info.is_volatile);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_source_volatile(map, MLN_STRING_LITERAL("missing"), true)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void style_source_info_reports_other_source_shapes(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(
      map,
      MLN_BUFFER_LITERAL(
        "{\"version\":8,\"sources\":{"
        "\"dem\":{\"type\":\"raster-dem\",\"tiles\":[\"https://example.com/dem/"
        "{z}/{x}/{y}.png\"],\"tileSize\":256,\"encoding\":\"terrarium\"},"
        "\"geo-url\":{\"type\":\"geojson\",\"data\":\"https://example.com/"
        "data.geojson\"},"
        "\"geo-data\":{\"type\":\"geojson\",\"data\":{\"type\":"
        "\"FeatureCollection\",\"features\":[]}},"
        "\"image\":{\"type\":\"image\",\"url\":\"https://example.com/"
        "image.png\",\"coordinates\":[[-1,1],[1,1],[1,-1],[-1,-1]]}},"
        "\"layers\":[]}"
      )
    )
  );

  bool found = false;
  mln_style_source_info info = {.size = sizeof(mln_style_source_info)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_get_style_source_info(map, MLN_STRING_LITERAL("dem"), &info, &found)
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_BITS_HIGH(
    MLN_STYLE_SOURCE_INFO_TILEJSON | MLN_STYLE_SOURCE_INFO_TILE_SIZE |
      MLN_STYLE_SOURCE_INFO_RASTER_ENCODING,
    info.fields
  );
  TEST_ASSERT_EQUAL_UINT32(256, info.tile_size);
  TEST_ASSERT_EQUAL_DOUBLE(0.0, info.min_zoom);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_TILE_SCHEME_XYZ, info.scheme);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_STYLE_RASTER_DEM_ENCODING_TERRARIUM, info.raster_encoding
  );

  const mln_buffer_view url_ids[] = {
    MLN_STRING_LITERAL("geo-url"), MLN_STRING_LITERAL("image")
  };
  for (size_t index = 0; index < 2; index += 1) {
    info = (mln_style_source_info){.size = sizeof(mln_style_source_info)};
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK,
      mln_map_get_style_source_info(map, url_ids[index], &info, &found)
    );
    TEST_ASSERT_TRUE(found);
    TEST_ASSERT_BITS_HIGH(MLN_STYLE_SOURCE_INFO_URL, info.fields);
    TEST_ASSERT_TRUE(info.url_size > 0);
  }

  info = (mln_style_source_info){.size = sizeof(mln_style_source_info)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(
                     map, MLN_STRING_LITERAL("geo-data"), &info, &found
                   )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_UINT32(0, info.fields);

  size_t url_size = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_source_url(
      map, MLN_STRING_LITERAL("geo-data"), NULL, 0, &url_size, &found
    )
  );
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL_size_t(0, url_size);

  mln_style_string_list empty = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_tile_urls(
                     map, MLN_STRING_LITERAL("geo-data"), &empty, &found
                   )
  );
  size_t count = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_style_string_list_count(empty, &count)
  );
  TEST_ASSERT_EQUAL_size_t(0, count);
  mln_style_string_list_destroy(empty);

  mln_style_string_list missing = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_tile_urls(
                     map, MLN_STRING_LITERAL("missing"), &missing, &found
                   )
  );
  TEST_ASSERT_FALSE(found);
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, missing);
  url_size = 123;
  found = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_style_source_url(
      map, MLN_STRING_LITERAL("missing"), NULL, 0, &url_size, &found
    )
  );
  TEST_ASSERT_FALSE(found);
  TEST_ASSERT_EQUAL_size_t(0, url_size);

  info = (mln_style_source_info){.size = sizeof(mln_style_source_info)};
  found = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_source_info(
                     map, MLN_STRING_LITERAL("missing"), &info, &found
                   )
  );
  TEST_ASSERT_FALSE(found);
  TEST_ASSERT_EQUAL_UINT32(MLN_STYLE_SOURCE_TYPE_UNKNOWN, info.type);
  TEST_ASSERT_EQUAL_UINT32(0, info.fields);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_style_source_tile_urls(
      map, MLN_STRING_LITERAL("dem"), &empty, &found
    )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The loaded document is what the style loader last parsed, not a serialization
// of the live style.
static void loaded_style_document_reports_the_parsed_bytes(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  // A map that has parsed no style reports no document and no URL.
  size_t size = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_loaded_style_json(map, NULL, 0, &size)
  );
  TEST_ASSERT_EQUAL_size_t(0, size);
  size = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_url(map, NULL, 0, &size)
  );
  TEST_ASSERT_EQUAL_size_t(0, size);

  static const char style_json[] =
    "{\"version\":8,\"name\":\"probe\",\"sources\":{},\"layers\":[]}";
  const size_t style_json_length = sizeof(style_json) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(map, MLN_BUFFER_LITERAL(style_json))
  );

  // The copy is byte-for-byte the string that was loaded, so a host can hand it
  // straight back to mln_map_set_style_json().
  size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_loaded_style_json(map, NULL, 0, &size)
  );
  TEST_ASSERT_EQUAL_size_t(style_json_length, size);

  uint8_t document[sizeof(style_json)] = {0};
  size = 0;
  // An exact-length buffer is enough because the copy carries no terminator.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_loaded_style_json(map, document, style_json_length, &size)
  );
  TEST_ASSERT_EQUAL_size_t(style_json_length, size);
  TEST_ASSERT_EQUAL_INT(0, memcmp(document, style_json, style_json_length));

  // Loading inline JSON clears the style URL.
  size = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_url(map, NULL, 0, &size)
  );
  TEST_ASSERT_EQUAL_size_t(0, size);

  // Mutating the live style does not rewrite the parsed document.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_style_layer_json(
                     map,
                     MLN_BUFFER_LITERAL(
                       "{\"id\":\"stale-background\",\"type\":\"background\"}"
                     ),
                     (mln_buffer_view){NULL, 0}
                   )
  );

  uint8_t after[sizeof(style_json)] = {0};
  size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_loaded_style_json(map, after, sizeof(after), &size)
  );
  TEST_ASSERT_EQUAL_size_t(style_json_length, size);
  TEST_ASSERT_EQUAL_INT(0, memcmp(after, style_json, style_json_length));

  // A failed parse leaves the previously parsed document in place.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_NATIVE_ERROR,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL("{\"version\":"))
  );
  uint8_t retained[sizeof(style_json)] = {0};
  size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_copy_loaded_style_json(map, retained, sizeof(retained), &size)
  );
  TEST_ASSERT_EQUAL_size_t(style_json_length, size);
  TEST_ASSERT_EQUAL_INT(0, memcmp(retained, style_json, style_json_length));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The style URL is recorded when the request is made, so it reports live state
// while the document reports what last parsed.
static void style_url_reports_the_requested_url(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  // An unreachable URL is still recorded: the URL is request state, not a
  // result. The load fails later through the event stream.
  static const char style_url[] = "custom://style.json";
  const size_t style_url_length = sizeof(style_url) - 1;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_set_style_url(map, style_url));

  size_t size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_url(map, NULL, 0, &size)
  );
  TEST_ASSERT_EQUAL_size_t(style_url_length, size);

  char url[sizeof(style_url)] = {0};
  size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_url(map, url, style_url_length, &size)
  );
  TEST_ASSERT_EQUAL_size_t(style_url_length, size);
  TEST_ASSERT_EQUAL_INT(0, memcmp(url, style_url, style_url_length));

  char too_small[4] = {0};
  size = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_url(map, too_small, sizeof(too_small), &size)
  );
  TEST_ASSERT_EQUAL_size_t(style_url_length, size);

  // Nothing parsed yet, so the document stays empty while the URL is set.
  size = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_loaded_style_json(map, NULL, 0, &size)
  );
  TEST_ASSERT_EQUAL_size_t(0, size);

  // An empty URL is accepted and reads back as zero bytes, which this entry
  // point cannot tell apart from no URL requested.
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_set_style_url(map, ""));
  size = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_url(map, NULL, 0, &size)
  );
  TEST_ASSERT_EQUAL_size_t(0, size);
  size = 123;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_copy_style_url(map, url, sizeof(url), &size)
  );
  TEST_ASSERT_EQUAL_size_t(0, size);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Bindings always emit a full struct header and reject non-finite durations
// before the call, so only raw C callers reach these rejections.
static void style_transition_options_reject_unsafe_raw_headers(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  // Every rejection below leaves this applied configuration untouched, so a
  // setter that committed one field before validating another fails here.
  mln_style_transition_options applied = mln_style_transition_options_default();
  applied.fields = MLN_STYLE_TRANSITION_OPTION_DURATION |
                   MLN_STYLE_TRANSITION_OPTION_DELAY |
                   MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS;
  applied.duration_ms = 42.0;
  applied.delay_ms = 7.0;
  applied.enable_placement_transitions = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_transition_options(map, &applied)
  );

  // Omitting the placement bit leaves the cross-fade on rather than carrying
  // the struct's zero value through. The getter always reports the bit.
  mln_style_transition_options omitted = mln_style_transition_options_default();
  omitted.fields = MLN_STYLE_TRANSITION_OPTION_DURATION;
  omitted.duration_ms = 5.0;
  omitted.enable_placement_transitions = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_transition_options(map, &omitted)
  );
  mln_style_transition_options kept = mln_style_transition_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_transition_options(map, &kept)
  );
  TEST_ASSERT_TRUE(
    (kept.fields & MLN_STYLE_TRANSITION_OPTION_ENABLE_PLACEMENT_TRANSITIONS) !=
    0U
  );
  TEST_ASSERT_TRUE(kept.enable_placement_transitions);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_transition_options(map, &applied)
  );

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_style_transition_options(map, NULL)
  );

  mln_style_transition_options short_size =
    mln_style_transition_options_default();
  short_size.size = (uint32_t)(sizeof(mln_style_transition_options) - 1);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_transition_options(map, &short_size)
  );

  mln_style_transition_options unknown_field =
    mln_style_transition_options_default();
  unknown_field.fields = 1U << 20U;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_transition_options(map, &unknown_field)
  );

  // Each rejected duration carries a valid delay and vice versa, so a setter
  // that commits before validating leaves the baseline half-overwritten in
  // whichever order it validates.
  mln_style_transition_options infinite_duration =
    mln_style_transition_options_default();
  infinite_duration.fields =
    MLN_STYLE_TRANSITION_OPTION_DURATION | MLN_STYLE_TRANSITION_OPTION_DELAY;
  infinite_duration.duration_ms = INFINITY;
  infinite_duration.delay_ms = 99.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_transition_options(map, &infinite_duration)
  );

  mln_style_transition_options negative_delay =
    mln_style_transition_options_default();
  negative_delay.fields =
    MLN_STYLE_TRANSITION_OPTION_DURATION | MLN_STYLE_TRANSITION_OPTION_DELAY;
  negative_delay.duration_ms = 99.0;
  negative_delay.delay_ms = -1.0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_transition_options(map, &negative_delay)
  );

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_style_transition_options(map, NULL)
  );

  mln_style_transition_options short_out =
    mln_style_transition_options_default();
  short_out.size = (uint32_t)(sizeof(mln_style_transition_options) - 1);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_get_style_transition_options(map, &short_out)
  );

  // The largest accepted duration must survive conversion into native ticks,
  // where a rounded-up maximum wraps to the most negative duration. The ceiling
  // is bisected rather than named because it follows from the native duration
  // type rather than from this API.
  mln_style_transition_options probe = mln_style_transition_options_default();
  probe.fields = MLN_STYLE_TRANSITION_OPTION_DURATION;
  // Zero is the one duration this API always accepts.
  double accepted = 0.0;
  double rejected = 1.0;
  probe.duration_ms = accepted;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_transition_options(map, &probe)
  );
  // Doubling until one is rejected brackets the bisection below. A non-finite
  // bound is rejected too, so this terminates.
  for (int step = 0; step < 4096; step++) {
    probe.duration_ms = rejected;
    if (mln_map_set_style_transition_options(map, &probe) != MLN_STATUS_OK) {
      break;
    }
    accepted = rejected;
    rejected *= 2.0;
  }
  for (int step = 0; step < 4096; step++) {
    const double midpoint = accepted + ((rejected - accepted) / 2.0);
    if (midpoint <= accepted || midpoint >= rejected) {
      break;
    }
    probe.duration_ms = midpoint;
    if (mln_map_set_style_transition_options(map, &probe) == MLN_STATUS_OK) {
      accepted = midpoint;
    } else {
      rejected = midpoint;
    }
  }
  probe.duration_ms = accepted;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_transition_options(map, &probe)
  );
  mln_style_transition_options ceiling = mln_style_transition_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_transition_options(map, &ceiling)
  );
  TEST_ASSERT_TRUE(ceiling.duration_ms >= 0.0);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_transition_options(map, &applied)
  );

  mln_style_transition_options unchanged =
    mln_style_transition_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_get_style_transition_options(map, &unchanged)
  );
  TEST_ASSERT_EQUAL_UINT32(applied.fields, unchanged.fields);
  TEST_ASSERT_EQUAL_DOUBLE(applied.duration_ms, unchanged.duration_ms);
  TEST_ASSERT_EQUAL_DOUBLE(applied.delay_ms, unchanged.delay_ms);
  TEST_ASSERT_FALSE(unchanged.enable_placement_transitions);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_style_values_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(style_value_helpers_reject_unsafe_raw_descriptors);
  RUN_TEST(geojson_source_options_reject_unsafe_raw_values);
  RUN_TEST(clustered_geojson_data_reports_non_point_geometry);
  RUN_TEST(clustered_geojson_data_requires_a_feature_collection);
  RUN_TEST(prepared_geojson_data_installs_and_checks_options);
  RUN_TEST(optional_json_style_values_return_null_handles);
  RUN_TEST(layer_source_accessors_reject_sourceless_layer_types);
  RUN_TEST(layer_text_accessors_report_required_capacity);
  RUN_TEST(layer_zoom_and_visibility_accessors_carry_raw_domains);
  RUN_TEST(style_image_stretch_descriptors_reject_unsafe_raw_values);
  RUN_TEST(copy_entry_points_answer_a_null_buffer_as_a_size_probe);
  RUN_TEST(style_source_info_rebuilds_an_inline_tile_source);
  RUN_TEST(style_source_info_reports_url_backed_tile_source);
  RUN_TEST(style_source_volatility_round_trips);
  RUN_TEST(style_source_info_reports_other_source_shapes);
  RUN_TEST(loaded_style_document_reports_the_parsed_bytes);
  RUN_TEST(style_url_reports_the_requested_url);
  RUN_TEST(style_transition_options_reject_unsafe_raw_headers);
}
