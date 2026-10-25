// Surface sessions that own their EGL context.

#include "support/frames.h"
#include "support/test_support.h"

// The contract a dedicated session offers a host: it creates its own context
// from a descriptor that names no share context, renders through it, and leaves
// it current so the next render costs no EGL call.
static void dedicated_egl_surface_renders_and_keeps_its_context_current(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  // Attaching with no share context is the behavior under test.
  TEST_ASSERT_TRUE(mln_test_dedicated_egl_surface_create(map, &fixture));

  MLN_TEST_OK(
    mln_test_map_set_style_json(map, mln_test_red_background_style_json)
  );
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_FORCE | MLN_FRAME_DEMAND_PRESENT;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(&fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_TRUE(mln_test_egl_context_is_current());

  mln_test_dedicated_egl_surface_destroy(&fixture);
  // Destroying the session releases the thread it had taken over.
  TEST_ASSERT_FALSE(mln_test_egl_context_is_current());
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void render_dedicated_frame(mln_test_render_fixture* fixture) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_FORCE | MLN_FRAME_DEMAND_PRESENT;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
}

// Each render makes the session's own context current again when something
// else took the thread since the last one: another dedicated session, the
// host's own context, or another session's teardown.
static void dedicated_egl_surfaces_share_one_graphics_thread(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map first_map = mln_test_create_map(runtime);
  mln_map second_map = mln_test_create_map(runtime);
  mln_test_render_fixture first = {0};
  mln_test_render_fixture second = {0};
  TEST_ASSERT_TRUE(mln_test_dedicated_egl_surface_create(first_map, &first));
  TEST_ASSERT_TRUE(mln_test_dedicated_egl_surface_create(second_map, &second));
  MLN_TEST_OK(
    mln_test_map_set_style_json(first_map, mln_test_red_background_style_json)
  );
  MLN_TEST_OK(
    mln_test_map_set_style_json(second_map, mln_test_red_background_style_json)
  );
  MLN_TEST_OK(mln_test_map_fence(first_map));
  MLN_TEST_OK(mln_test_map_fence(second_map));

  render_dedicated_frame(&first);
  TEST_ASSERT_TRUE(mln_test_dedicated_egl_surface_is_current(&first));
  render_dedicated_frame(&second);
  TEST_ASSERT_TRUE(mln_test_dedicated_egl_surface_is_current(&second));
  render_dedicated_frame(&first);
  TEST_ASSERT_TRUE(mln_test_dedicated_egl_surface_is_current(&first));

  TEST_ASSERT_TRUE(mln_test_dedicated_egl_make_host_context_current(&first));
  TEST_ASSERT_FALSE(mln_test_dedicated_egl_surface_is_current(&first));
  render_dedicated_frame(&first);
  TEST_ASSERT_TRUE(mln_test_dedicated_egl_surface_is_current(&first));

  render_dedicated_frame(&second);
  mln_test_dedicated_egl_surface_destroy(&first);
  render_dedicated_frame(&second);
  TEST_ASSERT_TRUE(mln_test_dedicated_egl_surface_is_current(&second));

  mln_test_dedicated_egl_surface_destroy(&second);
  TEST_ASSERT_FALSE(mln_test_egl_context_is_current());
  mln_test_destroy_map(second_map);
  mln_test_destroy_map(first_map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(dedicated_egl_surface_renders_and_keeps_its_context_current);
  RUN_TEST(dedicated_egl_surfaces_share_one_graphics_thread);
}
