// Sessions that render into an exported ring of dmabufs, on a device of the
// build's backend. A case maps a frame's linear buffer and reads it on the
// CPU, as a host can. Mesa's software drivers export dmabufs only through the
// kernel's udmabuf module, which CI's Linux runners lack: where the device
// reports no dmabuf export, the cases check that the ring is rejected instead.

#include <drm/drm_fourcc.h>
#include <linux/dma-buf.h>
#include <poll.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "support/frames.h"
#include "support/test_support.h"

enum { ring_width = 64, ring_height = 48, buffer_count = 3 };

static const mln_dma_buf_format linear_formats[] = {
  {.fourcc = DRM_FORMAT_ABGR8888, .modifier = DRM_FORMAT_MOD_LINEAR},
};

static mln_render_target ring_target(
  const mln_dma_buf_format* formats, size_t format_count
) {
  return (mln_render_target){
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_EXPORTED_RING,
    .data.exported_ring = {
      .logical_size = {.width = ring_width, .height = ring_height},
      .texture_count = buffer_count,
      .export_kinds = MLN_GPU_EXPORT_KIND_FLAG_DMA_BUF,
      .dma_buf_formats = formats,
      .dma_buf_format_count = format_count,
    },
  };
}

// The library's default device, and whether it exports dmabufs.
static mln_gpu_device create_device(bool* out_exports) {
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_gpu_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC));
  mln_gpu_device_capabilities capabilities = {
    .size = sizeof(mln_gpu_device_capabilities)
  };
  MLN_TEST_OK(
    mln_gpu_device_get_capabilities(device, &capabilities, MLN_TEST_DIAGNOSTIC)
  );
  *out_exports =
    (capabilities.export_kinds & MLN_GPU_EXPORT_KIND_FLAG_DMA_BUF) != 0;
  return device;
}

static mln_status start_attach(
  mln_map map, mln_gpu_device device, mln_render_driver_kind driver,
  const mln_dma_buf_format* formats, size_t format_count,
  mln_test_completion* completion, mln_test_render_fixture* fixture
) {
  const mln_render_target target = ring_target(formats, format_count);
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = driver,
    .producer_sync_kinds =
      MLN_GPU_SYNC_KIND_FLAG_SYNC_FD | MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE,
  };
  options.frame_wake = mln_test_pulse_wake();
  fixture->driver = driver;
  const mln_status status = mln_map_attach(
    map, &target, device, &options, &fixture->session, &completion->descriptor,
    MLN_TEST_DIAGNOSTIC
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

static void expect_attach_status(
  mln_status expected, mln_map map, mln_gpu_device device,
  mln_render_driver_kind driver, const mln_dma_buf_format* formats,
  size_t format_count
) {
  mln_test_render_fixture fixture = {0};
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_STATUS(
    expected,
    start_attach(
      map, device, driver, formats, format_count, &completion, &fixture
    )
  );
  mln_test_completion_destroy(&completion);
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
}

// Attaches a ring of linear buffers on `device`.
static void attach(
  mln_map map, mln_gpu_device device, mln_test_render_fixture* fixture
) {
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(start_attach(
    map, device, MLN_RENDER_DRIVER_CORE_WORKER, linear_formats,
    sizeof(linear_formats) / sizeof(linear_formats[0]), &completion, fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &completion));
  mln_test_completion_destroy(&completion);
}

static mln_dma_buf_frame frame_buffer(mln_acquired_frame frame) {
  mln_dma_buf_frame buffer = {.size = sizeof(mln_dma_buf_frame)};
  MLN_TEST_OK(
    mln_acquired_frame_get_dma_buf(frame, &buffer, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_HEX32(DRM_FORMAT_ABGR8888, buffer.fourcc);
  TEST_ASSERT_EQUAL_HEX64(DRM_FORMAT_MOD_LINEAR, buffer.modifier);
  TEST_ASSERT_EQUAL_size_t(1, buffer.plane_count);
  TEST_ASSERT_NOT_NULL(buffer.planes);
  TEST_ASSERT_TRUE(buffer.planes[0].fd >= 0);
  TEST_ASSERT_TRUE(buffer.planes[0].stride >= buffer.width * 4);
  return buffer;
}

// Waits for the frame's rendering through its producer sync, which the
// session keeps.
static void wait_for_producer(mln_acquired_frame frame) {
  mln_gpu_sync sync = {.size = sizeof(mln_gpu_sync)};
  MLN_TEST_OK(
    mln_acquired_frame_get_producer_sync(frame, &sync, MLN_TEST_DIAGNOSTIC)
  );
  if (sync.kind == MLN_GPU_SYNC_CPU_COMPLETE) return;
  TEST_ASSERT_EQUAL_UINT32(MLN_GPU_SYNC_SYNC_FD, sync.kind);
  const int fd = (int)(int64_t)sync.handle;
  if (fd < 0) return;
  struct pollfd waiting = {.fd = fd, .events = POLLIN};
  const mln_test_deadline deadline = mln_test_deadline_default();
  while (poll(&waiting, 1, 100) == 0) {
    TEST_ASSERT_FALSE_MESSAGE(
      mln_test_deadline_passed(deadline), "the frame's sync file never signaled"
    );
  }
  TEST_ASSERT_TRUE((waiting.revents & POLLIN) != 0);
}

// Expects the style's red at the center of the frame's buffer, which a host
// maps and reads between DMA_BUF_IOCTL_SYNC calls.
static void expect_red_buffer(mln_acquired_frame frame) {
  wait_for_producer(frame);
  const mln_dma_buf_frame buffer = frame_buffer(frame);
  const mln_dma_buf_plane plane = buffer.planes[0];
  const size_t length =
    (size_t)plane.offset + ((size_t)plane.stride * buffer.height);
  uint8_t* data = mmap(NULL, length, PROT_READ, MAP_SHARED, plane.fd, 0);
  TEST_ASSERT_TRUE_MESSAGE(data != MAP_FAILED, "the dmabuf doesn't map");
  struct dma_buf_sync sync = {.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_READ};
  (void)ioctl(plane.fd, DMA_BUF_IOCTL_SYNC, &sync);
  uint8_t pixel[4] = {0};
  const size_t offset = plane.offset +
                        ((size_t)(buffer.height / 2) * plane.stride) +
                        ((size_t)(buffer.width / 2) * 4);
  memcpy(pixel, data + offset, sizeof(pixel));
  sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_READ;
  (void)ioctl(plane.fd, DMA_BUF_IOCTL_SYNC, &sync);
  munmap(data, length);
  // ABGR8888 keeps red in the first byte.
  TEST_ASSERT_EQUAL_HEX8(0xff, pixel[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, pixel[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, pixel[2]);
  TEST_ASSERT_EQUAL_HEX8(0xff, pixel[3]);
}

// Expects the frame's buffer to have the size in pixels of `logical_width`
// by `logical_height`.
static void expect_buffer_size(
  mln_acquired_frame frame, uint32_t logical_width, uint32_t logical_height
) {
  const mln_dma_buf_frame buffer = frame_buffer(frame);
  uint32_t width = 0;
  uint32_t height = 0;
  MLN_TEST_OK(mln_logical_extent_physical_size(
    (mln_logical_extent){
      .width = logical_width,
      .height = logical_height,
      .scale_factor = buffer.scale_factor,
    },
    &width, &height, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_UINT32(width, buffer.width);
  TEST_ASSERT_EQUAL_UINT32(height, buffer.height);
}

static void release_frame(mln_acquired_frame* frame) {
  if (*frame != MLN_HANDLE_NULL) mln_test_render_release_frame(frame);
}

static void release_session(mln_test_render_fixture* fixture) {
  if (fixture->session != MLN_HANDLE_NULL) {
    (void)mln_test_render_session_release(fixture->session);
  }
  fixture->session = MLN_HANDLE_NULL;
}

static mln_map create_red_map(mln_runtime runtime) {
  const mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  return map;
}

static void a_frame_renders_into_a_dma_buf_that_the_host_reads(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  bool exports = false;
  const mln_gpu_device device = create_device(&exports);
  if (TEST_PROTECT()) {
    if (!exports) {
      expect_attach_status(
        MLN_STATUS_UNSUPPORTED, map, device, MLN_RENDER_DRIVER_CORE_WORKER,
        linear_formats, sizeof(linear_formats) / sizeof(linear_formats[0])
      );
    } else {
      attach(map, device, &fixture);
      mln_render_session_capabilities capabilities = {
        .size = sizeof(mln_render_session_capabilities)
      };
      MLN_TEST_OK(mln_render_session_get_capabilities(
        fixture.session, &capabilities, MLN_TEST_DIAGNOSTIC
      ));
      TEST_ASSERT_EQUAL_UINT32(buffer_count, capabilities.texture_ring_depth);
      TEST_ASSERT_TRUE(
        (capabilities.flags &
         MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION) != 0
      );
      TEST_ASSERT_EQUAL_UINT32(
        MLN_GPU_EXPORT_KIND_DMA_BUF, capabilities.export_kind
      );
      TEST_ASSERT_TRUE(
        capabilities.producer_sync_kind == MLN_GPU_SYNC_SYNC_FD ||
        capabilities.producer_sync_kind == MLN_GPU_SYNC_CPU_COMPLETE
      );
      TEST_ASSERT_TRUE(
        (capabilities.consumer_sync_kinds &
         MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE) != 0
      );

      frame = mln_test_render_and_acquire(&fixture, 1);
      expect_buffer_size(frame, ring_width, ring_height);
      expect_red_buffer(frame);
      release_frame(&frame);
    }
  }
  release_frame(&frame);
  release_session(&fixture);
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

// A host keeps a frame acquired while it shows the frame, so the session
// renders the next frames into the ring's other buffers.
static void a_held_frame_keeps_its_buffer(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame held = MLN_HANDLE_NULL;
  mln_acquired_frame next = MLN_HANDLE_NULL;
  bool exports = false;
  const mln_gpu_device device = create_device(&exports);
  if (TEST_PROTECT()) {
    if (exports) {
      attach(map, device, &fixture);
      held = mln_test_render_and_acquire(&fixture, 1);
      const mln_dma_buf_frame first = frame_buffer(held);
      const int first_fd = first.planes[0].fd;

      for (uint64_t token = 2; token < 2 + buffer_count; ++token) {
        next = mln_test_render_and_acquire(&fixture, token);
        const mln_dma_buf_frame other = frame_buffer(next);
        TEST_ASSERT_NOT_EQUAL_UINT32(first.slot, other.slot);
        TEST_ASSERT_NOT_EQUAL_INT(first_fd, other.planes[0].fd);
        expect_red_buffer(next);
        release_frame(&next);
      }

      const mln_dma_buf_frame again = frame_buffer(held);
      TEST_ASSERT_EQUAL_UINT32(first.slot, again.slot);
      TEST_ASSERT_EQUAL_INT(first_fd, again.planes[0].fd);
      expect_red_buffer(held);
      release_frame(&held);
    }
  }
  release_frame(&next);
  release_frame(&held);
  release_session(&fixture);
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

// A compositor holds the buffer it shows while its window resizes: the held
// frame keeps its buffer of the old size, and new frames get the new size.
static void a_resize_keeps_the_buffer_of_a_held_frame(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame held = MLN_HANDLE_NULL;
  mln_acquired_frame next = MLN_HANDLE_NULL;
  bool exports = false;
  const mln_gpu_device device = create_device(&exports);
  if (TEST_PROTECT()) {
    if (exports) {
      attach(map, device, &fixture);
      held = mln_test_render_and_acquire(&fixture, 1);
      expect_buffer_size(held, ring_width, ring_height);

      MLN_TEST_RENDER_AWAIT(
        MLN_STATUS_OK, &fixture,
        mln_render_session_resize(
          fixture.session,
          mln_test_session_extent(
            fixture.session,
            (mln_logical_size){.width = ring_height, .height = ring_width}
          ),
          &completion.descriptor, MLN_TEST_DIAGNOSTIC
        )
      );
      next = mln_test_render_and_acquire(&fixture, 2);
      expect_buffer_size(next, ring_height, ring_width);
      expect_red_buffer(next);
      release_frame(&next);

      expect_buffer_size(held, ring_width, ring_height);
      expect_red_buffer(held);
      release_frame(&held);

      next = mln_test_render_and_acquire(&fixture, 3);
      expect_buffer_size(next, ring_height, ring_width);
      release_frame(&next);
    }
  }
  release_frame(&next);
  release_frame(&held);
  release_session(&fixture);
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

// A sync file of -1 has signaled already, and a release that takes one
// succeeds wherever the ring takes sync files.
static void a_release_takes_the_hosts_sync_file(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  bool exports = false;
  const mln_gpu_device device = create_device(&exports);
  if (TEST_PROTECT()) {
    if (exports) {
      attach(map, device, &fixture);
      mln_render_session_capabilities capabilities = {
        .size = sizeof(mln_render_session_capabilities)
      };
      MLN_TEST_OK(mln_render_session_get_capabilities(
        fixture.session, &capabilities, MLN_TEST_DIAGNOSTIC
      ));
      for (uint64_t token = 1; token <= 2 * buffer_count; ++token) {
        frame = mln_test_render_and_acquire(&fixture, token);
        expect_red_buffer(frame);
        const bool sync_file = (capabilities.consumer_sync_kinds &
                                MLN_GPU_SYNC_KIND_FLAG_SYNC_FD) != 0;
        const mln_gpu_sync consumer = {
          .size = sizeof(mln_gpu_sync),
          .kind = sync_file ? MLN_GPU_SYNC_SYNC_FD : MLN_GPU_SYNC_CPU_COMPLETE,
          .handle = sync_file ? (uint64_t)(int64_t)-1 : 0,
        };
        MLN_TEST_OK(
          mln_acquired_frame_release(frame, &consumer, MLN_TEST_DIAGNOSTIC)
        );
        frame = MLN_HANDLE_NULL;
      }
    }
  }
  release_frame(&frame);
  release_session(&fixture);
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

// The library owns the graphics context, so it drives the session itself.
static void a_dma_buf_ring_rejects_the_callers_graphics_thread(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  if (TEST_PROTECT()) {
    expect_attach_status(
      MLN_STATUS_UNSUPPORTED, map, MLN_HANDLE_NULL,
      MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, linear_formats,
      sizeof(linear_formats) / sizeof(linear_formats[0])
    );
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
    expect_attach_status(
      MLN_STATUS_UNSUPPORTED, map, MLN_HANDLE_NULL,
      MLN_RENDER_DRIVER_CORE_WORKER, formats,
      sizeof(formats) / sizeof(formats[0])
    );
  }
  destroy_map_and_runtime(map, runtime);
}

static void a_dma_buf_ring_needs_formats(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  if (TEST_PROTECT()) {
    expect_attach_status(
      MLN_STATUS_INVALID_ARGUMENT, map, MLN_HANDLE_NULL,
      MLN_RENDER_DRIVER_CORE_WORKER, NULL, 0
    );
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
  RUN_TEST(a_frame_renders_into_a_dma_buf_that_the_host_reads);
  RUN_TEST(a_held_frame_keeps_its_buffer);
  RUN_TEST(a_resize_keeps_the_buffer_of_a_held_frame);
  RUN_TEST(a_release_takes_the_hosts_sync_file);
  RUN_TEST(a_dma_buf_ring_rejects_the_callers_graphics_thread);
  RUN_TEST(a_dma_buf_ring_rejects_formats_that_it_cannot_render);
  RUN_TEST(a_dma_buf_ring_needs_formats);
  RUN_TEST(getting_a_dma_buf_needs_a_live_frame);
}
