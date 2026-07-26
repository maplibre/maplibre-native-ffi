// Raw C ABI coverage: malformed descriptor counts and unknown enum values are
// hidden by binding-owned style values.

#include "test_support.h"
#include "unity.h"

// This verifies malformed coordinate counts and unknown rasterization enum
// values hidden by binding value types.
static void style_value_helpers_reject_unsafe_raw_descriptors(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
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
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
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

void run_style_values_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(style_value_helpers_reject_unsafe_raw_descriptors);
  RUN_TEST(geojson_source_options_reject_unsafe_raw_headers);
}
