// Raw C ABI coverage for ordered detach and forced target abandonment.

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static void normal_detach_runs_on_the_driver_and_retires_map_attachment(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_operation detach = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_detach_start(fixture.session, &detach)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, detach)
  );
  mln_operation_release(detach);

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_get_snapshot(fixture.session, &snapshot)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_DETACHED, snapshot.state);
  mln_operation rejected = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_barrier_start(fixture.session, &rejected)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, rejected);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void stale_and_null_sessions_reject_the_new_control_surface(void) {
  mln_operation operation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_reduce_memory_use_start(MLN_HANDLE_NULL, &operation)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_clear_data_start(MLN_HANDLE_NULL, &operation)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_dump_debug_logs_start(MLN_HANDLE_NULL, &operation)
  );

  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  const mln_render_session stale = fixture.session;
  mln_test_render_fixture_destroy(&fixture);
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_get_snapshot(stale, &snapshot)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_destroy(stale)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_render_target_lifecycle_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(normal_detach_runs_on_the_driver_and_retires_map_attachment);
  RUN_TEST(stale_and_null_sessions_reject_the_new_control_surface);
}
