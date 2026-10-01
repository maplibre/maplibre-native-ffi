// Targets that the host creates: a session renders into a borrowed texture and
// presents to a surface, both made by tests/graphics for the preset's backend.
// The browser presets have no such fixtures, because JavaScript owns their
// canvases.

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "support/harness.h"
#include "support/host_graphics.h"
#include "support/test_support.h"
#include "unity.h"

#if !defined(__EMSCRIPTEN__)
static void finish_render_barrier(const mln_test_render_fixture* fixture) {
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(
    mln_render_session_barrier(fixture->session, &completion.descriptor, NULL)
  );
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &completion));
  mln_test_completion_destroy(&completion);
}

// Loads the red background style, renders one frame of it, and returns that
// frame's disposition.
static uint32_t render_red_frame(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture,
  uint32_t flags
) {
  MLN_TEST_OK(
    mln_test_map_set_style_json(map, mln_test_red_background_style_json)
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = flags;
  demand.token = 7;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
  finish_render_barrier(fixture);
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_render_session_drain_frame_results(fixture->session, &batch, NULL)
  );
  mln_render_frame_result result = {.size = sizeof(mln_render_frame_result)};
  const mln_status status = mln_render_frame_batch_get(batch, 0, &result, NULL);
  mln_render_frame_batch_release(batch);
  MLN_TEST_OK(status);
  TEST_ASSERT_EQUAL_UINT64(demand.token, result.token);
  return result.disposition;
}

static void a_borrowed_texture_holds_the_rendered_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(
    mln_test_render_fixture_create_borrowed_texture(map, &fixture)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_red_frame(runtime, map, &fixture, 0)
  );

  enum { pixel_count = MLN_TEST_HOST_TARGET_SIZE * MLN_TEST_HOST_TARGET_SIZE };
  static uint8_t pixels[pixel_count * 4];
  TEST_ASSERT_TRUE(
    mln_test_render_fixture_read_texture(&fixture, pixels, sizeof(pixels))
  );
  size_t red_pixels = 0;
  for (size_t index = 0; index < pixel_count; index += 1) {
    const uint8_t* pixel = &pixels[index * 4];
    red_pixels +=
      pixel[0] == 255 && pixel[1] == 0 && pixel[2] == 0 && pixel[3] == 255;
  }
  TEST_ASSERT_EQUAL_size_t(pixel_count, red_pixels);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_surface_presents_the_rendered_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_render_fixture_create_surface(map, &fixture));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED,
    render_red_frame(runtime, map, &fixture, MLN_FRAME_DEMAND_PRESENT)
  );
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Reads back the fixture's texture, and returns the status of the submission
// when it is refused or of the completion when it is accepted.
static mln_status read_back(const mln_test_render_fixture* fixture) {
  mln_test_completion readback = mln_test_completion_readback();
  const mln_status submitted = mln_texture_read_premultiplied_rgba8(
    fixture->session, &readback.descriptor, NULL
  );
  if (submitted != MLN_STATUS_OK) {
    mln_test_completion_reject(&readback);
    mln_test_completion_destroy(&readback);
    return submitted;
  }
  const mln_status status =
    mln_test_render_fixture_finish_operation(fixture, &readback);
  mln_test_completion_destroy(&readback);
  return status;
}

// Only a session-owned texture ring reads back, and a borrowed texture takes
// its size from its owner, so the session refuses a resize for it.
static void host_targets_refuse_readback_and_a_borrowed_texture_resize(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(
    mln_test_render_fixture_create_borrowed_texture(map, &fixture)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_red_frame(runtime, map, &fixture, 0)
  );
  MLN_TEST_STATUS(MLN_STATUS_UNSUPPORTED, read_back(&fixture));
  const mln_render_target_extent smaller = {
    .size = sizeof(mln_render_target_extent),
    .width = MLN_TEST_HOST_TARGET_SIZE / 2,
    .height = MLN_TEST_HOST_TARGET_SIZE / 2,
    .scale_factor = 1.0,
  };
  mln_test_completion resize = mln_test_completion_default(0);
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_render_session_resize(
      fixture.session, &smaller, &resize.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "sized by its owner"));
  mln_test_completion_reject(&resize);
  mln_test_completion_destroy(&resize);
  mln_test_render_fixture_destroy(&fixture);

  TEST_ASSERT_TRUE(mln_test_render_fixture_create_surface(map, &fixture));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED,
    render_red_frame(runtime, map, &fixture, MLN_FRAME_DEMAND_PRESENT)
  );
  MLN_TEST_STATUS(MLN_STATUS_UNSUPPORTED, read_back(&fixture));
  mln_test_render_fixture_destroy(&fixture);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

MLN_TEST_GROUP {
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(a_borrowed_texture_holds_the_rendered_frame);
  RUN_TEST(a_surface_presents_the_rendered_frame);
  RUN_TEST(host_targets_refuse_readback_and_a_borrowed_texture_resize);
#endif
}
