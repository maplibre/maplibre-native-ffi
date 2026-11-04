// Attaching a ring of dmabufs that the library allocates. CI's Linux runners
// have no GPU, and their kernels lack the udmabuf module that Mesa's software
// drivers need to export dmabufs, so these cases check only what a session
// rejects.

#include <drm/drm_fourcc.h>

#include "support/frames.h"
#include "support/test_support.h"

enum { ring_width = 64, ring_height = 48, buffer_count = 3 };

static const mln_dma_buf_format linear_formats[] = {
  {.fourcc = DRM_FORMAT_ABGR8888, .modifier = DRM_FORMAT_MOD_LINEAR},
};

static mln_status start_attach(
  mln_map map, mln_render_driver_kind driver, const mln_dma_buf_format* formats,
  size_t format_count, mln_test_completion* completion,
  mln_test_render_fixture* fixture
) {
  const mln_dma_buf_ring_descriptor descriptor = {
    .size = sizeof(mln_dma_buf_ring_descriptor),
    .logical_size = {.width = ring_width, .height = ring_height},
    .buffer_count = buffer_count,
    .formats = formats,
    .format_count = format_count,
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = driver,
    .producer_sync_kinds =
      MLN_GPU_SYNC_KIND_FLAG_SYNC_FD | MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE,
  };
  options.frame_wake = mln_test_pulse_wake();
  fixture->driver = driver;
  const mln_status status = mln_map_attach_dma_buf_ring(
    map, &descriptor, MLN_HANDLE_NULL, &options, &fixture->session,
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

static void a_dma_buf_ring_rejects_the_callers_graphics_thread(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  if (TEST_PROTECT()) {
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      start_attach(
        map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, linear_formats,
        sizeof(linear_formats) / sizeof(linear_formats[0]), &completion,
        &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  destroy_map_and_runtime(map, runtime);
}

// The library renders no YUV format, and no device takes an invalid modifier.
static void a_dma_buf_ring_rejects_formats_that_it_cannot_render(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  if (TEST_PROTECT()) {
    const mln_dma_buf_format formats[] = {
      {.fourcc = DRM_FORMAT_NV12, .modifier = DRM_FORMAT_MOD_LINEAR},
      {.fourcc = DRM_FORMAT_ABGR8888, .modifier = DRM_FORMAT_MOD_INVALID},
    };
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      start_attach(
        map, MLN_RENDER_DRIVER_CORE_WORKER, formats,
        sizeof(formats) / sizeof(formats[0]), &completion, &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  destroy_map_and_runtime(map, runtime);
}

static void getting_a_dma_buf_needs_a_live_frame(void) {
  mln_dma_buf_frame buffer = {.size = sizeof(mln_dma_buf_frame)};
  MLN_TEST_INVALID(
    mln_acquired_frame_get_dma_buf(MLN_HANDLE_NULL, &buffer, NULL)
  );
}

MLN_TEST_GROUP {
  RUN_TEST(a_dma_buf_ring_rejects_the_callers_graphics_thread);
  RUN_TEST(a_dma_buf_ring_rejects_formats_that_it_cannot_render);
  RUN_TEST(getting_a_dma_buf_needs_a_live_frame);
}
