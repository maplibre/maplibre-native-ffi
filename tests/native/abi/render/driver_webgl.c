// A WebGL surface whose OffscreenCanvas the host transfers to the library,
// which the core worker then drives.

#include <stdint.h>

#include "support/frames.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void transferred_offscreen_canvas_runs_on_core_worker(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_transferred_webgl_surface_create(map, &fixture));

  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture.session, &snapshot, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_DRIVER_CORE_WORKER, snapshot.driver);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_SESSION_STATE_ATTACHED, snapshot.state);

  mln_test_render_request_forced(&fixture, 901);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture, 1);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  TEST_ASSERT_EQUAL_UINT64(901, result.token);
  mln_render_frame_batch_release(batch);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP { RUN_TEST(transferred_offscreen_canvas_runs_on_core_worker); }
