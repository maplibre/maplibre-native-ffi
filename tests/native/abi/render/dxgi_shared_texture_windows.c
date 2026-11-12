// Attaching a ring of DXGI shared textures that the library allocates. Windows
// CI renders Vulkan with lavapipe, whose GPU has no LUID that names a DXGI
// adapter, so no D3D11 device there can open the ring's textures. These cases
// check only what a session rejects.

#include "support/frames.h"
#include "support/test_support.h"

enum { ring_width = 64, ring_height = 48, texture_count = 3 };

static mln_status start_attach(
  mln_map map, mln_render_driver_kind driver, mln_test_completion* completion,
  mln_test_render_fixture* fixture
) {
  const mln_render_target target = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_EXPORTED_RING,
    .data.exported_ring = {
      .logical_size = {.width = ring_width, .height = ring_height},
      .texture_count = texture_count,
      .export_kinds = MLN_GPU_EXPORT_KIND_FLAG_DXGI_SHARED_HANDLE,
    },
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = driver,
    .producer_sync_kinds =
      MLN_GPU_SYNC_KIND_FLAG_D3D12_FENCE | MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE,
  };
  options.frame_wake = mln_test_pulse_wake();
  fixture->driver = driver;
  const mln_status status = mln_map_attach(
    map, &target, MLN_HANDLE_NULL, &options, &fixture->session,
    &completion->descriptor, MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) mln_test_completion_reject(completion);
  return status;
}

// Unity aborts at every assertion after a failed one, back into the case's
// TEST_PROTECT, so a teardown that asserts would run again without end. After
// a failure, the harness reclaims the map and the runtime instead.
static void destroy_map_and_runtime(mln_map map, mln_runtime runtime) {
  if (Unity.CurrentTestFailed) return;
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void expect_attach_unsupported(mln_render_driver_kind driver) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  if (TEST_PROTECT()) {
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED, start_attach(map, driver, &completion, &fixture)
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  destroy_map_and_runtime(map, runtime);
}

// The library owns the graphics context, so it drives the session itself.
static void a_dxgi_shared_texture_ring_rejects_the_callers_graphics_thread(
  void
) {
  expect_attach_unsupported(MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD);
}

#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
static void an_opengl_build_renders_into_no_dxgi_shared_textures(void) {
  expect_attach_unsupported(MLN_RENDER_DRIVER_CORE_WORKER);
}
#endif

static void getting_a_dxgi_shared_texture_needs_a_live_frame(void) {
  mln_dxgi_shared_texture_frame texture = {
    .size = sizeof(mln_dxgi_shared_texture_frame)
  };
  MLN_TEST_INVALID(
    mln_acquired_frame_get_dxgi_shared_texture(MLN_HANDLE_NULL, &texture, NULL)
  );
}

MLN_TEST_GROUP {
  RUN_TEST(a_dxgi_shared_texture_ring_rejects_the_callers_graphics_thread);
#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
  RUN_TEST(an_opengl_build_renders_into_no_dxgi_shared_textures);
#endif
  RUN_TEST(getting_a_dxgi_shared_texture_needs_a_live_frame);
}
