// Owned textures that keep their EGL or WGL context on a core worker. A
// browser build's dedicated texture transfers a canvas instead.

#include "support/frames.h"
#include "support/test_support.h"

#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
// A private owned texture needs no host graphics thread: its core worker
// creates the context, renders, reads pixels, and destroys the context.
static void dedicated_texture_uses_a_readback_only_core_worker(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE(mln_test_dedicated_texture_create(map, &fixture));

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

  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_STATE,
    mln_render_session_service_driver_work(fixture.session, NULL)
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
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
  demand.flags = MLN_FRAME_DEMAND_FORCE;
  demand.token = 41;
  MLN_TEST_OK(mln_render_session_request_frame(fixture.session, &demand, NULL));
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(&fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(demand.token, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);

  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(
    mln_render_session_read_texture(fixture.session, &readback.descriptor, NULL)
  );
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

  mln_test_dedicated_texture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

MLN_TEST_GROUP {
#if !defined(MLN_FFI_TEST_OPENGL_WEBGL)
  RUN_TEST(dedicated_texture_uses_a_readback_only_core_worker);
#endif
}
