// Targets that the host creates: a session renders into a borrowed ring and
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

enum { pixel_count = MLN_TEST_HOST_TARGET_SIZE * MLN_TEST_HOST_TARGET_SIZE };

static const uint8_t red[4] = {255, 0, 0, 255};
static const uint8_t blue[4] = {0, 0, 255, 255};

// Every pixel of `texture` holds `color`. Reading another graphics object's
// texture leaves the fixture's OpenGL context as it was.
static void expect_texture_color(
  mln_test_graphics_texture* texture, const uint8_t color[4], const char* what
) {
  static uint8_t pixels[pixel_count * 4];
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_graphics_texture_read_rgba8(texture, pixels, sizeof(pixels)),
    mln_test_graphics_last_error()
  );
  size_t matching = 0;
  for (size_t index = 0; index < pixel_count; index += 1) {
    matching += memcmp(&pixels[index * 4], color, 4) == 0;
  }
  TEST_ASSERT_EQUAL_size_t_MESSAGE(pixel_count, matching, what);
}

// Repaints the red style's background blue, with no transition, so the next
// frame shows the change.
static void paint_background_blue(mln_runtime runtime, mln_map map) {
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("bg"),
    MLN_BUFFER_LITERAL("background-color-transition"),
    MLN_BUFFER_LITERAL("{\"duration\":0}"), &completion.descriptor, NULL
  ));
  MLN_TEST_AWAIT_OK(mln_map_set_style_layer_property(
    map, MLN_BUFFER_LITERAL("bg"), MLN_BUFFER_LITERAL("background-color"),
    MLN_BUFFER_LITERAL("\"#0000ff\""), &completion.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
}

// Acquires the frame that a demand just rendered, and checks that it names the
// ring texture of its slot and needs no producer synchronization object, so
// the case reads that texture with no fence of its own. Returns that slot.
static uint32_t acquire_ring_frame(
  const mln_test_render_fixture* fixture, mln_acquired_frame* out_frame
) {
  MLN_TEST_OK(
    mln_render_session_acquire_frame(fixture->session, out_frame, NULL)
  );
  mln_gpu_sync producer = {.size = sizeof(mln_gpu_sync)};
  MLN_TEST_OK(
    mln_acquired_frame_get_producer_sync(*out_frame, &producer, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_GPU_SYNC_CPU_COMPLETE, producer.kind);
  uint32_t slot = UINT32_MAX;
  const uint64_t texture = mln_test_frame_texture_handle(*out_frame, &slot);
  TEST_ASSERT_LESS_THAN_UINT32(2, slot);
  TEST_ASSERT_EQUAL_UINT64(
    mln_test_texture_handle(mln_test_render_fixture_texture(fixture, slot)),
    texture
  );
  return slot;
}

// A borrowed ring hands each rendered frame to the host through acquisition,
// naming the host's texture for the frame's slot, and never renders into a
// texture whose frame the host holds: with every frame held, a demand waits
// for a release. The ring belongs to its owner, so the session reads nothing
// back and refuses a resize.
static void a_borrowed_ring_hands_each_frame_to_the_host(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(
    runtime, map, mln_test_red_background_style_json
  );
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_borrowed_ring(
      map, &fixture, 2, MLN_TEST_PRESET_DRIVER
    ),
    mln_test_graphics_last_error()
  );
  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(2, capabilities.texture_ring_depth);
  TEST_ASSERT_EQUAL_HEX32(
    MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION |
      MLN_RENDER_SESSION_CAPABILITY_CONSUMER_SYNC,
    capabilities.flags
  );

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 1, 0)
  );
  mln_acquired_frame first = MLN_HANDLE_NULL;
  const uint32_t first_slot = acquire_ring_frame(&fixture, &first);
  expect_texture_color(
    mln_test_render_fixture_texture(&fixture, first_slot), red,
    "the first frame's texture"
  );

  paint_background_blue(runtime, map);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 2, 0)
  );
  mln_acquired_frame second = MLN_HANDLE_NULL;
  const uint32_t second_slot = acquire_ring_frame(&fixture, &second);
  TEST_ASSERT_NOT_EQUAL_UINT32(first_slot, second_slot);
  expect_texture_color(
    mln_test_render_fixture_texture(&fixture, second_slot), blue,
    "the second frame's texture"
  );
  expect_texture_color(
    mln_test_render_fixture_texture(&fixture, first_slot), red,
    "the held first frame's texture"
  );

  // Both textures are held, so the next demand parks. The maintenance command
  // runs after it on the driver, so once that completes the demand has had its
  // chance to render.
  mln_test_render_request_forced(&fixture, 3);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_reduce_memory_use(
      fixture.session, &completion.descriptor, NULL
    )
  );
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_NOT_READY,
    mln_render_session_drain_frame_results(fixture.session, &batch, NULL)
  );
  mln_test_render_release_frame(&first);
  batch = mln_test_render_wait_for_results(&fixture, 1);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT64(3, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_acquired_frame third = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_UINT32(first_slot, acquire_ring_frame(&fixture, &third));
  expect_texture_color(
    mln_test_render_fixture_texture(&fixture, first_slot), blue,
    "the released texture after the parked demand"
  );
  mln_test_render_release_frame(&second);
  mln_test_render_release_frame(&third);

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
  RUN_TEST(a_borrowed_ring_hands_each_frame_to_the_host);
  RUN_TEST(a_surface_presents_frames_and_takes_a_resize);
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  RUN_TEST(a_surface_replacement_can_change_the_scale_factor);
#endif
#endif
}
