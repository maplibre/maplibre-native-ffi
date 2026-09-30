// Raw C ABI coverage for ordered detach and forced target abandonment.

#include <stdatomic.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void normal_detach_runs_on_the_driver_and_retires_map_attachment(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  mln_test_completion detach = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_detach(fixture.session, &detach.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, &detach)
  );
  mln_test_completion_destroy(&detach);

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_DETACHED, snapshot.state);
  mln_completion rejected = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_barrier(fixture.session, &rejected, NULL)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void stale_and_null_sessions_reject_the_new_control_surface(void) {
  mln_completion operation = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_reduce_memory_use(MLN_HANDLE_NULL, &operation, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_clear_data(MLN_HANDLE_NULL, &operation, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_dump_debug_logs(MLN_HANDLE_NULL, &operation, NULL)
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
    mln_render_session_get_snapshot(stale, &snapshot, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_destroy(stale, NULL)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void retirement_wake(void* context) { (void)context; }
static void retirement_release(void* context) {
  atomic_store((atomic_bool*)context, true);
}

static void parent_first_disposal_retires_a_native_render_attachment(void) {
  atomic_bool retired = false;
  mln_runtime_options options = mln_runtime_options_default();
  options.event_wake.callback = retirement_wake;
  options.event_wake.user_data = &retired;
  options.event_wake.release_user_data = retirement_release;
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create(&options, &runtime, NULL)
  );
  mln_map map = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_create_status(runtime, NULL, &map)
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_dispose(runtime, NULL));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_dispose(map, NULL));
  TEST_ASSERT_FALSE(atomic_load(&retired));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_dispose(fixture.session, NULL)
  );
  mln_render_session_snapshot snapshot = {.size = sizeof(snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&retired));
  mln_test_render_fixture_destroy(&fixture);
}

MLN_TEST_GROUP {
  RUN_TEST(normal_detach_runs_on_the_driver_and_retires_map_attachment);
  RUN_TEST(parent_first_disposal_retires_a_native_render_attachment);
  RUN_TEST(stale_and_null_sessions_reject_the_new_control_surface);
}
