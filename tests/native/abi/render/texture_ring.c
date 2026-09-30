// A session-owned texture ring across a resize.

#include <stdint.h>

#include "support/frames.h"
#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void release_frame(mln_acquired_frame* frame) {
  const mln_gpu_sync sync = mln_gpu_sync_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_acquired_frame_release(frame, &sync, NULL)
  );
}

static void render_and_release(
  const mln_test_render_fixture* fixture, uint64_t token
) {
  mln_acquired_frame frame = mln_test_render_and_acquire(fixture, token);
  release_frame(&frame);
}

// With both slots of the two-deep ring holding textures of the old size, a
// resize retires each of them, so the next frame renders at the new size
// whichever slot it lands in. Holding the first frame while the second renders
// is what puts a texture in the second slot.
static void a_resize_retires_every_old_size_slot(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create(map, &fixture));
  mln_acquired_frame held = mln_test_render_and_acquire(&fixture, 301);
  render_and_release(&fixture, 302);
  release_frame(&held);

  const mln_render_target_extent resized = {
    .size = sizeof(mln_render_target_extent),
    .width = 48,
    .height = 24,
    .scale_factor = 1.0,
  };
  mln_test_completion resize = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_render_session_resize(
                     fixture.session, &resized, &resize.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, &resize)
  );
  mln_test_completion_destroy(&resize);
  render_and_release(&fixture, 303);
  render_and_release(&fixture, 304);

  mln_test_completion readback = mln_test_completion_readback();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_texture_read_premultiplied_rgba8(
                     fixture.session, &readback.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_render_fixture_finish_operation(&fixture, &readback)
  );
  mln_texture_readback_result result = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&readback, &result, sizeof(result))
  );
  TEST_ASSERT_EQUAL_UINT32(48, result.info.width);
  TEST_ASSERT_EQUAL_UINT32(24, result.info.height);
  mln_test_completion_destroy(&readback);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP { RUN_TEST(a_resize_retires_every_old_size_slot); }
