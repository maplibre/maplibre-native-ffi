// Sessions that own their EGL context: the dedicated ownership mode, which
// only an EGL provider offers.

#include "support/frames.h"
#include "support/test_support.h"

static void finish_render_barrier(const mln_test_render_fixture* fixture) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_barrier(fixture->session, &completion.descriptor, NULL)
  );
}

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
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = MLN_FRAME_DEMAND_PRESENT;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  finish_render_barrier(&fixture);
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_render_session_drain_frame_results(fixture.session, &batch, NULL)
  );
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

// A private EGL owned texture needs no host graphics thread: its core worker
// creates the context, renders, reads pixels, and destroys the context.
static void dedicated_egl_texture_uses_a_readback_only_core_worker(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_dedicated_egl_texture_create(map, &fixture));

  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_DRIVER_CORE_WORKER, capabilities.driver);
  TEST_ASSERT_EQUAL_UINT32(1, capabilities.texture_ring_depth);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_CAPABILITY_READBACK, capabilities.flags
  );
  size_t serviced = 0;
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_service_driver_work(fixture.session, 0, &serviced, NULL)
  );
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_render_session_acquire_frame(fixture.session, &frame, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, frame);

  MLN_TEST_OK(
    mln_test_map_set_style_json(map, mln_test_red_background_style_json)
  );
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = 0;
  demand.token = 41;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  finish_render_barrier(&fixture);

  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_render_session_drain_frame_results(fixture.session, &batch, NULL)
  );
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(demand.token, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);

  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(mln_texture_read_premultiplied_rgba8(
    fixture.session, &readback.descriptor, NULL
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &readback));
  mln_texture_readback_result readback_result = {0};
  TEST_ASSERT_TRUE(mln_test_completion_copy_value(
    &readback, &readback_result, sizeof(readback_result)
  ));
  mln_buffer_view view = readback_result.data;
  mln_texture_image_info info = readback_result.info;
  TEST_ASSERT_EQUAL_UINT32(64, info.width);
  TEST_ASSERT_EQUAL_UINT32(64, info.height);
  TEST_ASSERT_EQUAL_UINT32(64 * 4, info.stride);
  TEST_ASSERT_EQUAL_size_t(64 * 64 * 4, info.byte_length);
  TEST_ASSERT_EQUAL_size_t(info.byte_length, view.size);
  const uint8_t* rgba = view.data;
  TEST_ASSERT_EQUAL_UINT8(255, rgba[0]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[1]);
  TEST_ASSERT_EQUAL_UINT8(0, rgba[2]);
  TEST_ASSERT_EQUAL_UINT8(255, rgba[3]);
  mln_test_completion_destroy(&readback);

  mln_test_dedicated_egl_texture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(dedicated_egl_surface_renders_and_keeps_its_context_current);
  RUN_TEST(dedicated_egl_texture_uses_a_readback_only_core_worker);
}
