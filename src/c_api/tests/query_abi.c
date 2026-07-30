// Raw C ABI coverage: undersized query descriptors, unknown option masks, and
// invalid string-view pointers are hidden by bindings.

#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

// This verifies undersized query geometry, unknown masks, and invalid string
// views hidden by binding values.
static void feature_query_validation_rejects_raw_descriptor_shapes(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_feature_query_result result = MLN_HANDLE_NULL;
  mln_rendered_query_geometry geometry = mln_rendered_query_geometry_point(
    (mln_screen_point){.x = 256.0, .y = 256.0}
  );
  geometry.size = sizeof(mln_rendered_query_geometry) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_query_rendered_features(
                                   fixture.session, &geometry, NULL, &result
                                 )
  );

  mln_rendered_feature_query_options options =
    mln_rendered_feature_query_options_default();
  options.fields = UINT32_C(1) << 31;
  geometry = mln_rendered_query_geometry_point(
    (mln_screen_point){.x = 256.0, .y = 256.0}
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_query_rendered_features(
                                   fixture.session, &geometry, &options, &result
                                 )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_query_source_features(
      fixture.session, (mln_string_view){.data = NULL, .size = 1}, NULL, &result
    )
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_query_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(feature_query_validation_rejects_raw_descriptor_shapes);
}
