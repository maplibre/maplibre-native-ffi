// Raw C ABI coverage: null and stale render-session handles are hidden by
// binding-owned handle state.

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

// This verifies null render-session handles across backend-neutral maintenance
// entry points.
static void render_session_maintenance_rejects_null_raw_handles(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_reduce_memory_use(NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_clear_data(NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_dump_debug_logs(NULL)
  );
}

// This verifies the raw registry rejects destroyed render-session handles after
// binding handle state would intervene.
static void render_session_rejects_stale_raw_handles(void) {
  mln_runtime* runtime = mln_test_create_runtime();
  mln_map* map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_render_session* stale_session = fixture.session;
  mln_test_render_fixture_destroy(&fixture);
  bool rendered = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_render_session_render_update(stale_session, &rendered)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_render_session_destroy(stale_session)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

void run_owned_texture_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(render_session_maintenance_rejects_null_raw_handles);
  RUN_TEST(render_session_rejects_stale_raw_handles);
}
