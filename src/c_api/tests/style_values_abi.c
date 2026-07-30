// Raw C ABI coverage: malformed descriptor counts and unknown enum values are
// hidden by binding-owned style values.

#include <string.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

// This verifies malformed coordinate counts and unknown rasterization enum
// values hidden by binding value types.
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
      map, (mln_string_view){.data = "image-url-source", .size = 16},
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
      map, (mln_string_view){.data = "bad-dem", .size = 7},
      (mln_string_view){.data = "https://example.com/bad.json", .size = 28},
      &options
    )
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Bindings always emit a full struct header, so only raw C callers can present
// a short size or unknown field bits to the GeoJSON source adders.
static void geojson_source_options_reject_unsafe_raw_headers(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_string_view url = {
    .data = "https://example.com/points.geojson", .size = 34
  };

  mln_geojson_source_options short_size = mln_geojson_source_options_default();
  short_size.size = (uint32_t)(sizeof(mln_geojson_source_options) - 1);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "short-header", .size = 12}, url,
      &short_size
    )
  );

  mln_geojson_source_options unknown_field =
    mln_geojson_source_options_default();
  unknown_field.fields = 1U << 31U;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "unknown-field", .size = 13}, url,
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
      map, (mln_string_view){.data = "fractional-min", .size = 14}, url,
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
      map, (mln_string_view){.data = "fractional-max", .size = 14}, url,
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
      map, (mln_string_view){.data = "fractional-cluster", .size = 18}, url,
      &fractional_cluster_max_zoom
    )
  );

  mln_geojson_source_options null_cluster_properties =
    mln_geojson_source_options_default();
  null_cluster_properties.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "null-properties", .size = 15}, url,
      &null_cluster_properties
    )
  );

  mln_json_value json_null_cluster_properties = {
    .size = sizeof(mln_json_value),
    .type = MLN_JSON_VALUE_TYPE_NULL,
  };
  mln_geojson_source_options non_object_cluster_properties =
    mln_geojson_source_options_default();
  non_object_cluster_properties.fields =
    MLN_GEOJSON_SOURCE_OPTION_CLUSTER_PROPERTIES;
  non_object_cluster_properties.cluster_properties =
    &json_null_cluster_properties;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_url(
      map, (mln_string_view){.data = "json-null-properties", .size = 20}, url,
      &non_object_cluster_properties
    )
  );

  // A rejected descriptor leaves the source ID free for a later valid add.
  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_add_geojson_source_url(
                     map, (mln_string_view){.data = "short-header", .size = 12},
                     url, &clustered
                   )
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Supercluster reads every feature geometry as a point, so clustered data
// carrying other geometry used to surface a bare variant access message. The
// C API rejects it up front and names the source, feature, and constraint.
static void clustered_geojson_data_reports_non_point_geometry(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_geometry child = {
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_POINT,
    .data = {.point = {.latitude = 37.8, .longitude = -122.4}}
  };
  const mln_geometry point = {
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_POINT,
    .data = {.point = {.latitude = 37.7, .longitude = -122.5}}
  };
  const mln_geometry collection = {
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_GEOMETRY_COLLECTION,
    .data = {.geometry_collection = {.geometries = &child, .geometry_count = 1}}
  };
  const mln_feature features[] = {
    {.size = sizeof(mln_feature), .geometry = &point},
    {.size = sizeof(mln_feature), .geometry = &collection},
  };
  const mln_geojson data = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {.feature_collection = {.features = features, .feature_count = 2}}
  };

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;
  const mln_string_view clustered_id = {.data = "quakes", .size = 6};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_add_geojson_source_data(map, clustered_id, &data, &clustered)
  );

  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "\"quakes\""));
  TEST_ASSERT_NOT_NULL(strstr(message, "point geometry on every feature"));
  TEST_ASSERT_NOT_NULL(strstr(message, "feature 1"));
  TEST_ASSERT_NOT_NULL(strstr(message, "geometry collection"));

  // The constraint belongs to clustering alone, so the same data tiles fine on
  // an unclustered source, and the rejected ID stays free.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, clustered_id, &data, NULL)
  );

  // A source added with clustering rejects the same data on update.
  const mln_string_view updated_id = {.data = "clustered-quakes", .size = 16};
  const mln_geojson points = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {.feature_collection = {.features = features, .feature_count = 1}}
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, updated_id, &points, &clustered)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(map, updated_id, &data)
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_thread_last_error_message(), "\"clustered-quakes\"")
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void clustered_geojson_data_requires_a_feature_collection(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);

  const mln_geometry point = {
    .size = sizeof(mln_geometry),
    .type = MLN_GEOMETRY_TYPE_POINT,
    .data = {.point = {.latitude = 37.7, .longitude = -122.5}}
  };
  const mln_feature feature = {.size = sizeof(mln_feature), .geometry = &point};
  const mln_geojson bare_geometry = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_GEOMETRY,
    .data = {.geometry = &point}
  };
  const mln_geojson single_feature = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_FEATURE,
    .data = {.feature = &feature}
  };

  mln_geojson_source_options clustered = mln_geojson_source_options_default();
  clustered.fields = MLN_GEOJSON_SOURCE_OPTION_CLUSTER;
  clustered.cluster = true;

  // MapLibre Native clusters feature collections only, so both of these would
  // tile unclustered rather than honouring the requested cluster option.
  const mln_string_view geometry_id = {.data = "quakes", .size = 6};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_add_geojson_source_data(
                                   map, geometry_id, &bare_geometry, &clustered
                                 )
  );
  const char* message = mln_thread_last_error_message();
  TEST_ASSERT_NOT_NULL(message);
  TEST_ASSERT_NOT_NULL(strstr(message, "\"quakes\""));
  TEST_ASSERT_NOT_NULL(strstr(message, "requires a feature collection"));
  TEST_ASSERT_NOT_NULL(strstr(message, "a bare geometry"));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_add_geojson_source_data(
                                   map, geometry_id, &single_feature, &clustered
                                 )
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_thread_last_error_message(), "a single feature")
  );

  // The constraint belongs to clustering alone, so the same data tiles fine on
  // an unclustered source, and the rejected ID stays free.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, geometry_id, &bare_geometry, NULL)
  );

  // An empty feature collection carries nothing to cluster, so it stays
  // accepted and a later update supplies the features to cluster.
  const mln_geojson empty = {
    .size = sizeof(mln_geojson),
    .type = MLN_GEOJSON_TYPE_FEATURE_COLLECTION,
    .data = {.feature_collection = {.features = NULL, .feature_count = 0}}
  };
  const mln_string_view empty_id = {.data = "pending", .size = 7};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_add_geojson_source_data(map, empty_id, &empty, &clustered)
  );

  // A source added with clustering rejects a bare geometry on update too.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_geojson_source_data(map, empty_id, &bare_geometry)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_thread_last_error_message(), "\"pending\""));

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_style_values_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(style_value_helpers_reject_unsafe_raw_descriptors);
  RUN_TEST(geojson_source_options_reject_unsafe_raw_headers);
  RUN_TEST(clustered_geojson_data_reports_non_point_geometry);
  RUN_TEST(clustered_geojson_data_requires_a_feature_collection);
}
