// Sessions that render into an exported ring of Vulkan memory, shared as
// opaque file descriptors for an OpenGL context of the same driver, such as one
// of GLX. Where the device exports no opaque file descriptor, such as one of
// OpenGL, the cases check that the ring is rejected instead.

#include <fcntl.h>
#include <sys/stat.h>

#include "support/frames.h"
#include "support/test_support.h"

enum {
  ring_width = 64,
  ring_height = 48,
  buffer_count = 3,
  // VK_FORMAT_R8G8B8A8_UNORM.
  vulkan_rgba8 = 37,
};

// The library's default device, and whether it exports opaque file
// descriptors.
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
    (capabilities.export_kinds & MLN_GPU_EXPORT_KIND_FLAG_OPAQUE_FD) != 0;
  return device;
}

// Set only while attach_keeping_depth() attaches.
static bool attach_keep_depth = false;

static mln_status start_attach(
  mln_map map, mln_gpu_device device, mln_test_completion* completion,
  mln_test_render_fixture* fixture
) {
  const mln_render_target target = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_EXPORTED_RING,
    .data.exported_ring = {
      .logical_size = {.width = ring_width, .height = ring_height},
      .texture_count = buffer_count,
      .export_kinds = MLN_GPU_EXPORT_KIND_FLAG_OPAQUE_FD,
    },
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
    .keep_depth = attach_keep_depth,
  };
  options.frame_wake = mln_test_pulse_wake();
  fixture->driver = MLN_RENDER_DRIVER_CORE_WORKER;
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

static void attach(
  mln_map map, mln_gpu_device device, mln_test_render_fixture* fixture
) {
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(start_attach(map, device, &completion, fixture));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &completion));
  mln_test_completion_destroy(&completion);
}

// The frame's memory, which `logical_width` by `logical_height` sizes.
static mln_opaque_fd_frame frame_memory(
  mln_acquired_frame frame, uint32_t logical_width, uint32_t logical_height
) {
  mln_opaque_fd_frame memory = {.size = sizeof(mln_opaque_fd_frame)};
  MLN_TEST_OK(
    mln_acquired_frame_get_opaque_fd(frame, &memory, MLN_TEST_DIAGNOSTIC)
  );
  uint32_t width = 0;
  uint32_t height = 0;
  MLN_TEST_OK(mln_logical_extent_physical_size(
    (mln_logical_extent){
      .width = logical_width,
      .height = logical_height,
      .scale_factor = memory.scale_factor,
    },
    &width, &height, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_EQUAL_UINT32(width, memory.width);
  TEST_ASSERT_EQUAL_UINT32(height, memory.height);
  TEST_ASSERT_EQUAL_UINT32(vulkan_rgba8, memory.format);
  TEST_ASSERT_TRUE(
    memory.allocation_size >= (uint64_t)memory.width * memory.height * 4
  );
  // The session keeps the descriptor open.
  TEST_ASSERT_TRUE(memory.fd >= 0);
  TEST_ASSERT_NOT_EQUAL_INT(-1, fcntl(memory.fd, F_GETFD));
  return memory;
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

static void a_frame_renders_into_memory_that_the_host_imports(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  bool exports = false;
  const mln_gpu_device device = create_device(&exports);
  if (TEST_PROTECT()) {
    if (!exports) {
      mln_test_completion completion = mln_test_completion_default(0);
      MLN_TEST_STATUS(
        MLN_STATUS_UNSUPPORTED, start_attach(map, device, &completion, &fixture)
      );
      mln_test_completion_destroy(&completion);
      TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
    } else {
      attach(map, device, &fixture);
      mln_render_session_capabilities capabilities = {
        .size = sizeof(mln_render_session_capabilities)
      };
      MLN_TEST_OK(mln_render_session_get_capabilities(
        fixture.session, &capabilities, MLN_TEST_DIAGNOSTIC
      ));
      TEST_ASSERT_EQUAL_UINT32(
        MLN_GPU_EXPORT_KIND_OPAQUE_FD, capabilities.export_kind
      );
      TEST_ASSERT_EQUAL_UINT32(
        MLN_GPU_SYNC_CPU_COMPLETE, capabilities.producer_sync_kind
      );

      frame = mln_test_render_and_acquire(&fixture, 1);
      const mln_opaque_fd_frame memory =
        frame_memory(frame, ring_width, ring_height);
      // The ring's other getters report another kind of texture.
      mln_dma_buf_frame buffer = {.size = sizeof(mln_dma_buf_frame)};
      MLN_TEST_STATUS(
        MLN_STATUS_UNSUPPORTED,
        mln_acquired_frame_get_dma_buf(frame, &buffer, MLN_TEST_DIAGNOSTIC)
      );
      // On the device, the ring's image is the one whose memory it shares.
      // The ring keeps no depth unless the attach asks it to.
      mln_vulkan_texture_frame texture = {
        .size = sizeof(mln_vulkan_texture_frame)
      };
      MLN_TEST_OK(mln_acquired_frame_get_vulkan_texture(
        frame, &texture, MLN_TEST_DIAGNOSTIC
      ));
      TEST_ASSERT_NOT_EQUAL_UINT64(0, texture.image);
      TEST_ASSERT_EQUAL_UINT32(memory.format, texture.format);
      TEST_ASSERT_EQUAL_UINT32(memory.width, texture.width);
      TEST_ASSERT_EQUAL_UINT64(0, texture.depth_image);
      TEST_ASSERT_EQUAL_INT32(-1, memory.depth_fd);
      struct stat status = {0};
      TEST_ASSERT_EQUAL_INT(0, fstat(memory.fd, &status));
      release_frame(&frame);
    }
  }
  release_frame(&frame);
  release_session(&fixture);
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

static void a_resize_keeps_the_memory_of_a_held_frame(void) {
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
      const mln_opaque_fd_frame first =
        frame_memory(held, ring_width, ring_height);

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
      (void)frame_memory(next, ring_height, ring_width);
      release_frame(&next);

      const mln_opaque_fd_frame again =
        frame_memory(held, ring_width, ring_height);
      TEST_ASSERT_EQUAL_INT(first.fd, again.fd);
      TEST_ASSERT_EQUAL_UINT32(first.slot, again.slot);
      release_frame(&held);
    }
  }
  release_frame(&next);
  release_frame(&held);
  release_session(&fixture);
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

// A ring that keeps depth shares each frame's depth memory beside its color
// memory, where the device exports depth, and names the depth image on the
// device in any case.
static void a_ring_that_keeps_depth_shares_its_frames_depth(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  bool exports = false;
  const mln_gpu_device device = create_device(&exports);
  mln_gpu_device_capabilities capabilities = {
    .size = sizeof(mln_gpu_device_capabilities)
  };
  MLN_TEST_OK(mln_gpu_device_get_capabilities(device, &capabilities, NULL));
  const bool exports_depth =
    (capabilities.depth_export_kinds & MLN_GPU_EXPORT_KIND_FLAG_OPAQUE_FD) != 0;
  if (TEST_PROTECT() && exports) {
    attach_keep_depth = true;
    attach(map, device, &fixture);
    attach_keep_depth = false;
    frame = mln_test_render_and_acquire(&fixture, 1);
    const mln_opaque_fd_frame memory =
      frame_memory(frame, ring_width, ring_height);
    mln_vulkan_texture_frame texture = {
      .size = sizeof(mln_vulkan_texture_frame)
    };
    MLN_TEST_OK(mln_acquired_frame_get_vulkan_texture(
      frame, &texture, MLN_TEST_DIAGNOSTIC
    ));
    TEST_ASSERT_NOT_EQUAL_UINT64(0, texture.depth_image);
    TEST_ASSERT_NOT_EQUAL_UINT64(0, texture.depth_image_view);
    // VK_IMAGE_LAYOUT_GENERAL.
    TEST_ASSERT_EQUAL_UINT32(1, texture.depth_layout);
    if (exports_depth) {
      TEST_ASSERT_TRUE(memory.depth_fd >= 0);
      TEST_ASSERT_NOT_EQUAL_INT(-1, fcntl(memory.depth_fd, F_GETFD));
      TEST_ASSERT_NOT_EQUAL_INT(memory.fd, memory.depth_fd);
      TEST_ASSERT_TRUE(memory.depth_allocation_size > 0);
      TEST_ASSERT_EQUAL_UINT32(texture.depth_format, memory.depth_format);
    } else {
      TEST_ASSERT_EQUAL_INT32(-1, memory.depth_fd);
    }
    release_frame(&frame);
  }
  attach_keep_depth = false;
  release_frame(&frame);
  release_session(&fixture);
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

static void getting_an_opaque_fd_needs_a_live_frame(void) {
  mln_opaque_fd_frame memory = {.size = sizeof(mln_opaque_fd_frame)};
  MLN_TEST_INVALID(
    mln_acquired_frame_get_opaque_fd(MLN_HANDLE_NULL, &memory, NULL)
  );
}

MLN_TEST_GROUP {
  RUN_TEST(a_frame_renders_into_memory_that_the_host_imports);
  RUN_TEST(a_resize_keeps_the_memory_of_a_held_frame);
  RUN_TEST(a_ring_that_keeps_depth_shares_its_frames_depth);
  RUN_TEST(getting_an_opaque_fd_needs_a_live_frame);
}
