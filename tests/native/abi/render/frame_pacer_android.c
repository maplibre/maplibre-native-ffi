// The Android frame pacer, which demands a session's frames at the vsyncs that
// a looper's AChoreographer reports.
//
// These executables have no app main thread, so each case that needs a pacer
// runs a thread that prepares an ALooper, creates the pacer there, and polls
// the looper as an app's main thread does. The thread stops before the case
// ends, even after a failed assertion: a thread that polls its looper while
// the process exits can crash in libbinder.

#include <android/looper.h>
#include <pthread.h>

#include "support/frames.h"
#include "support/test_support.h"

typedef struct looper_thread {
  pthread_t thread;
  ALooper* looper;
  mln_status create_status;
  mln_frame_pacer pacer;
  atomic_bool ready;
  atomic_bool stop;
} looper_thread;

static void* run_looper(void* context) {
  looper_thread* state = context;
  state->looper = ALooper_prepare(0);
  state->create_status = mln_frame_pacer_create(NULL, &state->pacer, NULL);
  mln_test_flag_set(&state->ready);
  while (!atomic_load(&state->stop)) {
    (void)ALooper_pollOnce(-1, NULL, NULL, NULL);
  }
  return NULL;
}

// Runs `body` with a pacer on a looper thread, then stops the thread.
static void with_looper_pacer(void (*body)(mln_frame_pacer pacer)) {
  looper_thread state = {0};
  if (pthread_create(&state.thread, NULL, run_looper, &state) != 0) {
    TEST_FAIL_MESSAGE("could not start the looper thread");
  }
  if (TEST_PROTECT()) {
    TEST_ASSERT_TRUE(mln_test_wait_for_flag(&state.ready));
    MLN_TEST_OK(state.create_status);
    body(state.pacer);
  }
  if (mln_test_wait_for_flag(&state.ready)) {
    atomic_store(&state.stop, true);
    ALooper_wake(state.looper);
    pthread_join(state.thread, NULL);
  }
}

#define LOOPER_CASE(name)                                    \
  static void name##_body(mln_frame_pacer pacer);            \
  static void name(void) { with_looper_pacer(name##_body); } \
  static void name##_body(mln_frame_pacer pacer)

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

// Paces a new session with `pacer`, which it releases first when `release` is
// set, and expects a camera change to reach a frame that no one else demands.
static void expect_paced_frames(mln_frame_pacer pacer, bool release) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  MLN_TEST_OK(mln_render_session_set_frame_pacer(fixture.session, pacer, NULL));
  if (release) mln_frame_pacer_release(pacer);

  mln_test_render_wait_for_update(&fixture, move_camera(map, 2.0));
  mln_test_render_wait_for_update(&fixture, move_camera(map, 3.0));

  mln_test_render_fixture_destroy(&fixture);
  if (!release) mln_frame_pacer_release(pacer);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void* create_pacer_without_looper(void* context) {
  looper_thread* state = context;
  state->create_status = mln_frame_pacer_create(NULL, &state->pacer, NULL);
  return NULL;
}

static void a_thread_without_a_looper_creates_no_pacer(void) {
  looper_thread state = {0};
  TEST_ASSERT_EQUAL_INT(
    0, pthread_create(&state.thread, NULL, create_pacer_without_looper, &state)
  );
  TEST_ASSERT_EQUAL_INT(0, pthread_join(state.thread, NULL));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_STATE, state.create_status);
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, state.pacer);
}

LOOPER_CASE(a_paced_session_draws_each_change_without_a_demand) {
  expect_paced_frames(pacer, false);
}

LOOPER_CASE(a_released_pacer_keeps_pacing_its_sessions) {
  expect_paced_frames(pacer, true);
}

// The changes come from a thread other than the pacer's looper, and each can
// come while the session waits for a refresh.
LOOPER_CASE(a_pacer_keeps_pacing_its_sessions_as_its_limit_changes) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_offscreen(map, &fixture));
  MLN_TEST_OK(mln_render_session_set_frame_pacer(fixture.session, pacer, NULL));
  mln_frame_pacer_options options = {.size = sizeof(mln_frame_pacer_options)};

  uint64_t generation = move_camera(map, 2.0);
  options.fields = MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND;
  options.maximum_frames_per_second = 30;
  MLN_TEST_OK(
    mln_frame_pacer_set_options(pacer, &options, MLN_TEST_DIAGNOSTIC)
  );
  mln_test_render_wait_for_update(&fixture, generation);

  generation = move_camera(map, 3.0);
  options.fields = MLN_FRAME_PACER_OPTION_UNLIMITED_FRAMES_PER_SECOND;
  MLN_TEST_OK(
    mln_frame_pacer_set_options(pacer, &options, MLN_TEST_DIAGNOSTIC)
  );
  mln_test_render_wait_for_update(&fixture, generation);

  mln_test_render_fixture_destroy(&fixture);
  mln_frame_pacer_release(pacer);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

LOOPER_CASE(set_options_refuses_an_invalid_limit_and_a_released_pacer) {
  mln_frame_pacer_options options = {.size = sizeof(mln_frame_pacer_options)};
  options.fields = MLN_FRAME_PACER_OPTION_MAXIMUM_FRAMES_PER_SECOND;
  options.maximum_frames_per_second = 0;
  MLN_TEST_INVALID(mln_frame_pacer_set_options(pacer, &options, NULL));
  options.fields |= MLN_FRAME_PACER_OPTION_UNLIMITED_FRAMES_PER_SECOND;
  options.maximum_frames_per_second = 30;
  MLN_TEST_INVALID(mln_frame_pacer_set_options(pacer, &options, NULL));

  mln_frame_pacer_release(pacer);
  options.fields = MLN_FRAME_PACER_OPTION_UNLIMITED_FRAMES_PER_SECOND;
  MLN_TEST_INVALID_STATE(mln_frame_pacer_set_options(pacer, &options, NULL));
}

LOOPER_CASE(a_session_with_a_redraw_wake_takes_no_pacer) {
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

  MLN_TEST_INVALID_STATE(
    mln_render_session_set_frame_pacer(fixture.session, pacer, NULL)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_frame_pacer_release(pacer);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_thread_without_a_looper_creates_no_pacer);
  RUN_TEST(a_paced_session_draws_each_change_without_a_demand);
  RUN_TEST(a_released_pacer_keeps_pacing_its_sessions);
  RUN_TEST(a_pacer_keeps_pacing_its_sessions_as_its_limit_changes);
  RUN_TEST(set_options_refuses_an_invalid_limit_and_a_released_pacer);
  RUN_TEST(a_session_with_a_redraw_wake_takes_no_pacer);
}
