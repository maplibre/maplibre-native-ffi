// Raw C ABI coverage: undersized query descriptors, unknown option masks, and
// invalid buffer-view pointers are hidden by bindings.

#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static void feature_query_validation_rejects_raw_descriptor_shapes(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_queried_feature_list result = MLN_HANDLE_NULL;
  mln_rendered_query_geometry geometry = mln_rendered_query_geometry_point(
    (mln_screen_point){.x = 256.0, .y = 256.0}
  );
  geometry.size = sizeof(mln_rendered_query_geometry) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_query_rendered_features(
                                   fixture.session, &geometry, NULL, &result
                                 )
  );
  mln_queried_feature_list_destroy(result);

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
  mln_queried_feature_list_destroy(result);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_query_source_features(
      fixture.session, (mln_buffer_view){.data = NULL, .size = 1}, NULL, &result
    )
  );
  mln_queried_feature_list_destroy(result);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void feature_query_hits_are_owned_by_one_list_handle(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_map_set_style_json(
      map, MLN_BUFFER_LITERAL("{\"version\":8,\"sources\":{},\"layers\":[]}")
    )
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_render_result render_result = MLN_RENDER_RESULT_NO_UPDATE;
  for (unsigned int attempt = 0;
       attempt < 500 && render_result != MLN_RENDER_RESULT_RENDERED;
       attempt += 1) {
    TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_pump(runtime, 0, -1));
    bool needs_repaint = false;
    TEST_ASSERT_EQUAL_INT(
      MLN_STATUS_OK, mln_render_session_render_update(
                       fixture.session, &render_result, &needs_repaint
                     )
    );
    if (render_result != MLN_RENDER_RESULT_RENDERED) {
      mln_test_sleep_millisecond();
    }
  }
  TEST_ASSERT_EQUAL_INT(MLN_RENDER_RESULT_RENDERED, render_result);

  const mln_rendered_query_geometry geometry =
    mln_rendered_query_geometry_point((mln_screen_point){0});
  mln_queried_feature_list result = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_query_rendered_features(
                     fixture.session, &geometry, NULL, &result
                   )
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, result);

  size_t count = 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_queried_feature_list_count(result, &count)
  );
  TEST_ASSERT_EQUAL_size_t(0, count);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_queried_feature_list_count(result, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_queried_feature_list_count(map, &count)
  );

  mln_queried_feature hit = mln_queried_feature_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_queried_feature_list_get(result, 0, &hit)
  );
  hit.size = sizeof(mln_queried_feature) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_queried_feature_list_get(result, 0, &hit)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_queried_feature_list_get(result, 0, NULL)
  );

  mln_queried_feature_list_destroy(result);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_queried_feature_list_count(result, &count)
  );
  mln_queried_feature_list_destroy(result);
  mln_queried_feature_list_destroy(MLN_HANDLE_NULL);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_query_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(feature_query_validation_rejects_raw_descriptor_shapes);
  RUN_TEST(feature_query_hits_are_owned_by_one_list_handle);
}
