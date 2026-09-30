// What a session refuses while it holds its target: destruction before detach,
// its map's release, a frame release its backend cannot wait on, and, for a
// target the host created, frame acquisition and a resize that only the host
// can make. A surface session resizes itself and keeps rendering.

#include <stdint.h>
#include <string.h>

#include "support/frames.h"
#include "support/harness.h"
#include "support/host_graphics.h"
#include "support/style.h"
#include "support/test_support.h"
#include "unity.h"

static void an_attached_session_holds_off_its_destroy_and_its_map(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_destroy(fixture.session, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "detached or abandoned"));
  mln_test_completion release = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_map_release(map, &release.descriptor, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL(
    strstr(mln_test_last_error(), "attached render session")
  );
  mln_test_completion_reject(&release);
  mln_test_completion_destroy(&release);

  // Neither refusal disturbed the session.
  mln_test_render_prepare_map(runtime, map);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(&fixture)
  );

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_frame_release_the_backend_cannot_wait_on_keeps_the_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_test_render_prepare_map(runtime, map);
  mln_acquired_frame frame = mln_test_render_and_acquire(&fixture, 1);

  mln_gpu_sync unknown = mln_gpu_sync_default();
  unknown.kind = 999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_acquired_frame_release(&frame, &unknown, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "gpu sync kind"));
  // The host still owns the frame and its accessors still answer.
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_get_result(frame, &result, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);

  const mln_gpu_sync sync = mln_gpu_sync_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_release(&frame, &sync, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if !defined(__EMSCRIPTEN__)
static mln_render_target_extent host_extent(uint32_t side) {
  return (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = side,
    .height = side,
    .scale_factor = 1.0,
  };
}

static void a_borrowed_texture_has_no_frames_and_is_sized_by_its_host(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(
    mln_test_render_fixture_create_borrowed_texture(map, &fixture)
  );

  mln_acquired_frame frame = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED, mln_render_session_acquire_frame(
                              fixture.session, &frame, MLN_TEST_DIAGNOSTIC
                            )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);

  const mln_render_target_extent extent =
    host_extent(MLN_TEST_HOST_TARGET_SIZE / 2);
  mln_test_completion resize = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_UNSUPPORTED,
    mln_render_session_resize(
      fixture.session, &extent, &resize.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "sized by its owner"));
  mln_test_completion_reject(&resize);
  mln_test_completion_destroy(&resize);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_surface_session_resizes_and_keeps_rendering(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_surface(map, &fixture));
  mln_test_render_prepare_map(runtime, map);

  const mln_render_target_extent extent =
    host_extent(MLN_TEST_HOST_TARGET_SIZE / 2);
  mln_test_completion resize = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_resize(
                     fixture.session, &extent, &resize.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, &resize)
  );
  mln_test_completion_destroy(&resize);

  // A frame that is not presented is rendered and then discarded.
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, mln_test_style_render_frame(&fixture)
  );
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_snapshot_get(map, &snapshot, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(extent.width, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_UINT32(extent.height, snapshot.logical_extent.height);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(an_attached_session_holds_off_its_destroy_and_its_map);
  RUN_TEST(a_frame_release_the_backend_cannot_wait_on_keeps_the_frame);
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(a_borrowed_texture_has_no_frames_and_is_sized_by_its_host);
  RUN_TEST(a_surface_session_resizes_and_keeps_rendering);
#endif
}
