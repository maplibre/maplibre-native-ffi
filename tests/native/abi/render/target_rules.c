// What a session refuses while it holds its target: a frame release its
// backend cannot wait on and, for a texture the host created, frame
// acquisition.

#include "support/frames.h"
#include "support/host_graphics.h"
#include "support/style.h"
#include "support/test_support.h"

static void a_frame_release_the_backend_cannot_wait_on_keeps_the_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_render_prepare_map(runtime, map);
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 1);

  mln_gpu_sync unknown = mln_gpu_sync_default();
  unknown.kind = 999;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_acquired_frame_release(&frame, &unknown, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "gpu sync kind"));
  // The host still owns the frame and its accessors still answer.
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  MLN_TEST_OK(mln_acquired_frame_get_result(frame, &result, NULL));
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);

  const mln_gpu_sync sync = mln_gpu_sync_default();
  MLN_TEST_OK(mln_acquired_frame_release(&frame, &sync, NULL));
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if !defined(__EMSCRIPTEN__)
static void a_borrowed_texture_has_no_frames_to_acquire(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(
    mln_test_render_fixture_create_borrowed_texture(map, &fixture)
  );

  mln_acquired_frame frame = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED, mln_render_session_acquire_frame(
                              fixture.session, &frame, MLN_TEST_DIAGNOSTIC
                            )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#endif

MLN_TEST_GROUP {
  RUN_TEST(a_frame_release_the_backend_cannot_wait_on_keeps_the_frame);
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(a_borrowed_texture_has_no_frames_to_acquire);
#endif
}
