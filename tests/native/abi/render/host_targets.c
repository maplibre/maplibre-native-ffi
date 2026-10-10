// Targets that the host creates: a session renders into a borrowed texture and
// presents to a surface, both made by tests/graphics for the preset's backend.
// The browser presets have no such fixtures, because JavaScript owns their
// canvases.

#include "support/frames.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

#if !defined(__EMSCRIPTEN__)
// Renders one frame of the map's style with `flags`, and returns that frame's
// disposition.
static uint32_t render_frame(
  const mln_test_render_fixture* fixture, uint64_t token, uint32_t flags
) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = flags;
  demand.token = token;
  demand.coalescing_boundary = token;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture, 1);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(token, result.token);
  return result.disposition;
}

// Reads back the fixture's texture, and returns the status of the submission
// when it is refused or of the completion when it is accepted.
static mln_status read_back(const mln_test_render_fixture* fixture) {
  mln_test_completion readback = mln_test_completion_readback();
  const mln_status submitted = mln_render_session_read_texture(
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

static mln_render_session_snapshot read_snapshot(
  const mln_test_render_fixture* fixture
) {
  mln_render_session_snapshot snapshot = {
    .size = sizeof(mln_render_session_snapshot)
  };
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture->session, &snapshot, NULL)
  );
  return snapshot;
}

// A borrowed texture holds every pixel of the frame rendered into it. It has
// no session-owned ring, so it has no frames to acquire or read back, and it
// takes its size from its owner, so the session refuses a resize for it.
static void a_borrowed_texture_holds_the_rendered_frame(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, mln_test_red_background_style_json
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(
    mln_test_render_fixture_create_borrowed_texture(map, &fixture)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 7, 0)
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

  mln_acquired_frame frame = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED, mln_render_session_acquire_frame(
                              fixture.session, &frame, MLN_TEST_DIAGNOSTIC
                            )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);
  MLN_TEST_STATUS(MLN_STATUS_UNSUPPORTED, read_back(&fixture));
  const mln_logical_extent smaller = {
    .width = MLN_TEST_HOST_TARGET_SIZE / 2,
    .height = MLN_TEST_HOST_TARGET_SIZE / 2,
    .scale_factor = 1.0,
  };
  mln_test_completion resize = mln_test_completion_default(0);
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_render_session_resize(
      fixture.session, smaller, &resize.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "sized by its owner"));
  mln_test_completion_reject(&resize);
  mln_test_completion_destroy(&resize);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A surface presents the frames it renders and reads none back. A resize
// reaches the host's surface, so the next frame, which presents nothing,
// renders at the new extent.
static void a_surface_presents_frames_and_takes_a_resize(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, mln_test_red_background_style_json
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_surface(map, &fixture),
    mln_test_graphics_last_error()
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED,
    render_frame(&fixture, 1, MLN_FRAME_DEMAND_PRESENT)
  );
  MLN_TEST_STATUS(MLN_STATUS_UNSUPPORTED, read_back(&fixture));

  const mln_logical_extent extent = {
    .width = 48,
    .height = 32,
    .scale_factor = 1.0,
  };
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session, extent, &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 2, 0)
  );
  const mln_render_session_snapshot snapshot = read_snapshot(&fixture);
  TEST_ASSERT_EQUAL_UINT32(48, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(32, snapshot.extent.height);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if defined(MLN_FFI_TEST_BACKEND_METAL)
// A replacement surface may carry a new scale factor. The renderer bakes the
// pixel ratio into its shaders, so the session rebuilds it and renders at the
// new ratio.
static void a_surface_replacement_can_change_the_scale_factor(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_prepare_map(runtime, map);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_surface(map, &fixture),
    mln_test_graphics_last_error()
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 1, 0)
  );

  mln_test_graphics* graphics = mln_test_render_fixture_graphics(&fixture);
  mln_test_graphics_surface* replacement =
    mln_test_render_fixture_new_surface(&fixture);
  TEST_ASSERT_NOT_NULL_MESSAGE(replacement, mln_test_graphics_last_error());
  mln_test_graphics_context context = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_get_context(graphics, &context));
  mln_test_graphics_surface_info info = {0};
  TEST_ASSERT_TRUE(mln_test_graphics_surface_get_info(replacement, &info));
  mln_metal_surface_descriptor descriptor =
    mln_metal_surface_descriptor_default();
  descriptor.extent = (mln_logical_extent){
    .width = MLN_TEST_HOST_TARGET_SIZE,
    .height = MLN_TEST_HOST_TARGET_SIZE,
    .scale_factor = 2.0,
  };
  descriptor.context.device = context.metal_device;
  descriptor.layer = info.metal_layer;
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_set_metal_surface_target(
      fixture.session, &descriptor, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 2, 0)
  );
  TEST_ASSERT_EQUAL_DOUBLE(2.0, read_snapshot(&fixture).extent.scale_factor);

  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif
#endif

MLN_TEST_GROUP {
#if !defined(__EMSCRIPTEN__)
  RUN_TEST(a_borrowed_texture_holds_the_rendered_frame);
  RUN_TEST(a_surface_presents_frames_and_takes_a_resize);
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  RUN_TEST(a_surface_replacement_can_change_the_scale_factor);
#endif
#endif
}
