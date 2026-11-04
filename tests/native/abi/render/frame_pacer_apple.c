// The Apple frame pacer, which demands a session's frames at the vsyncs that a
// CADisplayLink reports. The display link runs on a thread of the library's
// own, so these cases need no run loop.

#include <TargetConditionals.h>

#include "support/frames.h"
#include "support/test_support.h"

#if TARGET_OS_VISION
static void visionos_has_no_apple_frame_pacer(void) {
  mln_frame_pacer pacer = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_apple_frame_pacer_create(NULL, &pacer, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, pacer);
}
#else

static mln_frame_pacer create_pacer(const mln_frame_pacer_options* options) {
  mln_frame_pacer pacer = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_apple_frame_pacer_create(options, &pacer, MLN_TEST_DIAGNOSTIC)
  );
  return pacer;
}

// Returns the render update generation that shows the change.
static uint64_t move_camera(mln_map map, double zoom) {
  mln_camera_update update = {
    .size = sizeof(mln_camera_update),
    .camera = {.size = sizeof(mln_camera_options)}
  };
  update.camera.fields = MLN_CAMERA_OPTION_ZOOM;
  update.camera.zoom = zoom;
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_map_update_camera(map, &update, &completion.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_completion_finish(&completion));
  const uint64_t generation =
    mln_test_completion_render_update_generation(&completion);
  mln_test_completion_destroy(&completion);
  return generation;
}

// Paces a new session with a new pacer made from `options`, which it releases
// first when `release` is set, and expects a camera change to reach a frame
// that no one else demands.
static void expect_paced_frames(
  const mln_frame_pacer_options* options, bool release
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  const mln_frame_pacer pacer = create_pacer(options);
  MLN_TEST_OK(mln_render_session_set_frame_pacer(fixture.session, pacer, NULL));
  if (release) mln_frame_pacer_release(pacer);

  mln_test_render_wait_for_update(&fixture, move_camera(map, 2.0));
  mln_test_render_wait_for_update(&fixture, move_camera(map, 3.0));

  mln_test_render_fixture_destroy(&fixture);
  if (!release) mln_frame_pacer_release(pacer);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_paced_session_draws_each_change_without_a_demand(void) {
  expect_paced_frames(NULL, false);
}

static void a_released_pacer_keeps_pacing_its_sessions(void) {
  expect_paced_frames(NULL, true);
}

// The limit also sets the display link's preferred frame rate range.
static void a_pacer_with_a_frame_rate_limit_paces_its_sessions(void) {
  mln_frame_pacer_options options = {.size = sizeof(mln_frame_pacer_options)};
  options.fields = MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND;
  options.maximum_frames_per_second = 30;
  expect_paced_frames(&options, false);
}

static void a_session_with_a_redraw_wake_takes_no_pacer(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.redraw_wake = mln_test_pulse_wake();
  mln_test_completion attach = mln_test_completion_default(0);
  mln_test_render_fixture fixture = {0};
  MLN_TEST_OK(mln_test_render_fixture_start_attach(
    map, &options, &attach.descriptor, &fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);
  const mln_frame_pacer pacer = create_pacer(NULL);

  MLN_TEST_INVALID_STATE(
    mln_render_session_set_frame_pacer(fixture.session, pacer, NULL)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_frame_pacer_release(pacer);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#endif

MLN_TEST_GROUP {
#if TARGET_OS_VISION
  RUN_TEST(visionos_has_no_apple_frame_pacer);
#else
  RUN_TEST(a_paced_session_draws_each_change_without_a_demand);
  RUN_TEST(a_released_pacer_keeps_pacing_its_sessions);
  RUN_TEST(a_pacer_with_a_frame_rate_limit_paces_its_sessions);
  RUN_TEST(a_session_with_a_redraw_wake_takes_no_pacer);
#endif
}
