// Raw C ABI coverage: malformed descriptor counts and unknown enum values are
// hidden by binding-owned style values.

#include "test_support.h"
#include "unity.h"

// PRUNING REVIEW: KEEP.
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

void run_style_values_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(style_value_helpers_reject_unsafe_raw_descriptors);
}
