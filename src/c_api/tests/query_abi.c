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

  mln_operation operation = MLN_HANDLE_NULL;
  mln_rendered_query_geometry geometry = mln_rendered_query_geometry_point(
    (mln_screen_point){.x = 256.0, .y = 256.0}
  );
  geometry.size = sizeof(mln_rendered_query_geometry) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_query_rendered_features_start(
      fixture.session, &geometry, NULL, &operation
    )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, operation);

  mln_rendered_feature_query_options options =
    mln_rendered_feature_query_options_default();
  options.fields = UINT32_C(1) << 31;
  geometry = mln_rendered_query_geometry_point(
    (mln_screen_point){.x = 256.0, .y = 256.0}
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_query_rendered_features_start(
      fixture.session, &geometry, &options, &operation
    )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_query_source_features_start(
      fixture.session, (mln_buffer_view){.data = NULL, .size = 1}, NULL,
      &operation
    )
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void feature_query_hits_are_owned_by_one_list_handle(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_map_set_style_json(
      map, MLN_BUFFER_LITERAL("{\"version\":8,\"sources\":{},\"layers\":[]}")
    )
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_request_frame(fixture.session, &demand)
  );
  mln_operation barrier = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_barrier_start(fixture.session, 0, &barrier)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, barrier)
  );
  mln_operation_release(barrier);
  mln_render_frame_batch frame_batch = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_drain_frame_results(
                     fixture.session, SIZE_MAX, &frame_batch
                   )
  );
  mln_render_frame_batch_release(frame_batch);

  const mln_rendered_query_geometry geometry =
    mln_rendered_query_geometry_point((mln_screen_point){0});
  mln_operation query = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_query_rendered_features_start(
                     fixture.session, &geometry, NULL, &query
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, query)
  );
  mln_queried_feature_list result = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_query_features_take_result(query, &result)
  );
  mln_operation_release(query);

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
