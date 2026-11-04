// The redraw wake, which tells the host that a session has something new to
// draw.

#include "support/frames.h"
#include "support/test_support.h"

static atomic_int redraws;

static void count_redraw(void* user_data) {
  (void)user_data;
  atomic_fetch_add(&redraws, 1);
  mln_test_pulse();
}

static mln_render_session_attach_options redraw_options(void) {
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.redraw_wake.callback = count_redraw;
  return options;
}

// Runs after every driver item queued before it, and publishes no map update.
// The case has no demands for the barrier to end.
static void fence_driver(const mln_test_render_fixture* fixture) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_barrier(fixture->session, &completion.descriptor, NULL)
  );
}

static void move_camera(mln_map map, double zoom) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  MLN_TEST_AWAIT_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
}

// Demands a frame without flags, as a host does after a redraw wake, and
// returns its disposition.
static mln_render_result redraw(const mln_test_render_fixture* fixture) {
  const mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
  MLN_TEST_OK(mln_render_session_demand_frame(fixture->session, &demand, NULL));
  mln_render_frame_batch batch = mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  return result.disposition;
}

static void a_change_asks_for_one_redraw_until_the_next_demand(void) {
  atomic_store(&redraws, 0);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  const mln_render_session_attach_options options = redraw_options();
  mln_test_completion attach = mln_test_completion_default(0);
  mln_test_render_fixture fixture = {0};
  MLN_TEST_OK(mln_test_render_fixture_start_attach(
    map, &options, &attach.descriptor, &fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);

  // The new session has the map to draw.
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&redraws, 1));
  // Further changes wait for the host to demand the frame it asked for.
  move_camera(map, 2.0);
  fence_driver(&fixture);
  TEST_ASSERT_EQUAL_INT(1, atomic_load(&redraws));
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, redraw(&fixture));

  mln_test_render_until_idle(runtime, map, &fixture);
  const int idle = atomic_load(&redraws);
  move_camera(map, 3.0);
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&redraws, idle + 1));
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, redraw(&fixture));

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_EGL)
static void a_session_without_a_target_asks_for_no_redraw(void) {
  atomic_store(&redraws, 0);
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  const mln_render_session_attach_options options = redraw_options();
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(
    mln_test_dedicated_egl_surface_create_with_options(map, &options, &fixture)
  );
  mln_test_render_until_idle(runtime, map, &fixture);
  // Answers any redraw that the settling map asked for.
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_NO_UPDATE, redraw(&fixture));

  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_release_target(
      fixture.session, &completion.descriptor, NULL
    )
  );
  const int released = atomic_load(&redraws);
  move_camera(map, 2.0);
  fence_driver(&fixture);
  TEST_ASSERT_EQUAL_INT(released, atomic_load(&redraws));

  // The new target holds no frame, so the session asks to draw one.
  MLN_TEST_OK(mln_test_dedicated_egl_surface_restore(&fixture));
  TEST_ASSERT_TRUE(mln_test_wait_for_count(&redraws, released + 1));
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, redraw(&fixture));

  mln_test_dedicated_egl_surface_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(a_change_asks_for_one_redraw_until_the_next_demand);
#if defined(MLN_FFI_TEST_BACKEND_OPENGL) && defined(MLN_FFI_TEST_OPENGL_EGL)
  RUN_TEST(a_session_without_a_target_asks_for_no_redraw);
#endif
}
