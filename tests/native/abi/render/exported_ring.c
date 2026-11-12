// How an exported ring picks the way it shares its textures: the first of the
// kinds that the host imports that the device exports, in the order that
// mln_exported_ring_target gives, or none, which fails the attach.

#include "support/frames.h"
#include "support/test_support.h"

enum { ring_width = 64, ring_height = 48, buffer_count = 3 };

// DRM_FORMAT_ABGR8888 and DRM_FORMAT_MOD_LINEAR, which every device that
// exports dmabufs renders.
static const mln_dma_buf_format linear_formats[] = {
  {.fourcc = 0x34324241U, .modifier = 0},
};

static const mln_gpu_export_kind_flags every_kind =
  MLN_GPU_EXPORT_KIND_FLAG_IO_SURFACE | MLN_GPU_EXPORT_KIND_FLAG_DMA_BUF |
  MLN_GPU_EXPORT_KIND_FLAG_ANDROID_HARDWARE_BUFFER |
  MLN_GPU_EXPORT_KIND_FLAG_DXGI_SHARED_HANDLE |
  MLN_GPU_EXPORT_KIND_FLAG_OPAQUE_FD;

static mln_render_target ring_target(mln_gpu_export_kind_flags kinds) {
  return (mln_render_target){
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_EXPORTED_RING,
    .data.exported_ring = {
      .logical_size = {.width = ring_width, .height = ring_height},
      .texture_count = buffer_count,
      .export_kinds = kinds,
      .dma_buf_formats = linear_formats,
      .dma_buf_format_count =
        (kinds & MLN_GPU_EXPORT_KIND_FLAG_DMA_BUF) != 0 ? 1 : 0,
    },
  };
}

// The library's default device, or none where a build makes none, and the
// kinds that it exports.
static mln_gpu_device create_device(mln_gpu_export_kind_flags* out_kinds) {
  mln_gpu_device device = MLN_HANDLE_NULL;
  *out_kinds = 0;
  if (mln_gpu_device_create(NULL, &device, NULL) != MLN_STATUS_OK) {
    return MLN_HANDLE_NULL;
  }
  mln_gpu_device_capabilities capabilities = {
    .size = sizeof(mln_gpu_device_capabilities)
  };
  MLN_TEST_OK(
    mln_gpu_device_get_capabilities(device, &capabilities, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_HEX32(0, capabilities.export_kinds & ~every_kind);
  if (capabilities.export_kinds != 0) {
    TEST_ASSERT_TRUE(
      (capabilities.target_kinds & MLN_RENDER_TARGET_KIND_FLAG_EXPORTED_RING) !=
      0
    );
  }
  *out_kinds = capabilities.export_kinds;
  return device;
}

// The kind of `kinds` that a ring prefers.
static mln_gpu_export_kind preferred_kind(mln_gpu_export_kind_flags kinds) {
  static const struct {
    mln_gpu_export_kind_flag flag;
    mln_gpu_export_kind kind;
  } order[] = {
    {MLN_GPU_EXPORT_KIND_FLAG_IO_SURFACE, MLN_GPU_EXPORT_KIND_IO_SURFACE},
    {MLN_GPU_EXPORT_KIND_FLAG_ANDROID_HARDWARE_BUFFER,
     MLN_GPU_EXPORT_KIND_ANDROID_HARDWARE_BUFFER},
    {MLN_GPU_EXPORT_KIND_FLAG_DXGI_SHARED_HANDLE,
     MLN_GPU_EXPORT_KIND_DXGI_SHARED_HANDLE},
    {MLN_GPU_EXPORT_KIND_FLAG_DMA_BUF, MLN_GPU_EXPORT_KIND_DMA_BUF},
    {MLN_GPU_EXPORT_KIND_FLAG_OPAQUE_FD, MLN_GPU_EXPORT_KIND_OPAQUE_FD},
  };
  for (size_t index = 0; index < sizeof(order) / sizeof(order[0]); ++index) {
    if ((kinds & order[index].flag) != 0) return order[index].kind;
  }
  return MLN_GPU_EXPORT_KIND_NONE;
}

static mln_status start_attach(
  mln_map map, mln_gpu_device device, const mln_render_target* target,
  mln_test_completion* completion, mln_test_render_fixture* fixture
) {
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = MLN_RENDER_DRIVER_CORE_WORKER,
  };
  options.frame_wake = mln_test_pulse_wake();
  fixture->driver = MLN_RENDER_DRIVER_CORE_WORKER;
  const mln_status status = mln_map_attach(
    map, target, device, &options, &fixture->session, &completion->descriptor,
    MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) mln_test_completion_reject(completion);
  return status;
}

static void expect_attach_status(
  mln_status expected, mln_map map, mln_gpu_device device,
  const mln_render_target* target
) {
  mln_test_render_fixture fixture = {0};
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_STATUS(
    expected, start_attach(map, device, target, &completion, &fixture)
  );
  mln_test_completion_destroy(&completion);
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
}

// Unity aborts at every assertion after a failed one, back into the case's
// TEST_PROTECT, so a teardown that asserts would run again without end. After
// a failure, the harness reclaims the map and the runtime instead.
static void destroy_map_and_runtime(mln_map map, mln_runtime runtime) {
  if (Unity.CurrentTestFailed) return;
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void release_session(mln_test_render_fixture* fixture) {
  if (fixture->session != MLN_HANDLE_NULL) {
    (void)mln_test_render_session_release(fixture->session);
  }
  fixture->session = MLN_HANDLE_NULL;
}

// A host that imports every kind gets the one that its device prefers.
static void a_ring_takes_the_first_kind_that_its_device_exports(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_gpu_export_kind_flags exported = 0;
  const mln_gpu_device device = create_device(&exported);
  if (TEST_PROTECT()) {
    const mln_render_target target = ring_target(every_kind);
    if (exported == 0) {
      expect_attach_status(MLN_STATUS_UNSUPPORTED, map, device, &target);
    } else {
      mln_test_completion completion = mln_test_completion_default(0);
      MLN_TEST_OK(start_attach(map, device, &target, &completion, &fixture));
      MLN_TEST_OK(
        mln_test_render_fixture_finish_operation(&fixture, &completion)
      );
      mln_test_completion_destroy(&completion);
      mln_render_session_capabilities capabilities = {
        .size = sizeof(mln_render_session_capabilities)
      };
      MLN_TEST_OK(mln_render_session_get_capabilities(
        fixture.session, &capabilities, MLN_TEST_DIAGNOSTIC
      ));
      TEST_ASSERT_EQUAL_UINT32(
        preferred_kind(exported), capabilities.export_kind
      );
      TEST_ASSERT_EQUAL_UINT32(buffer_count, capabilities.texture_ring_depth);
      TEST_ASSERT_EQUAL_HEX32(
        MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION, capabilities.flags
      );

      // The host chooses among fewer kinds: each one that the device exports
      // alone, and each pair of them, in either order of the bits.
      release_session(&fixture);
      for (uint32_t first = 1; first <= every_kind; first <<= 1U) {
        for (uint32_t second = first; second <= every_kind; second <<= 1U) {
          const mln_gpu_export_kind_flags kinds = (first | second) & exported;
          if (kinds == 0) continue;
          const mln_render_target subset = ring_target(kinds);
          completion = mln_test_completion_default(0);
          MLN_TEST_OK(
            start_attach(map, device, &subset, &completion, &fixture)
          );
          MLN_TEST_OK(
            mln_test_render_fixture_finish_operation(&fixture, &completion)
          );
          mln_test_completion_destroy(&completion);
          MLN_TEST_OK(mln_render_session_get_capabilities(
            fixture.session, &capabilities, MLN_TEST_DIAGNOSTIC
          ));
          TEST_ASSERT_EQUAL_UINT32(
            preferred_kind(kinds), capabilities.export_kind
          );
          release_session(&fixture);
        }
      }
    }
  }
  release_session(&fixture);
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

static void a_ring_with_no_kind_that_its_device_exports_is_unsupported(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  mln_gpu_export_kind_flags exported = 0;
  const mln_gpu_device device = create_device(&exported);
  if (TEST_PROTECT()) {
    // No platform has every kind.
    const mln_gpu_export_kind_flags others = every_kind & ~exported;
    TEST_ASSERT_NOT_EQUAL_HEX32(0, others);
    const mln_render_target target = ring_target(others);
    expect_attach_status(MLN_STATUS_UNSUPPORTED, map, device, &target);
  }
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

static void an_exported_ring_rejects_invalid_descriptors(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  if (TEST_PROTECT()) {
    mln_render_target target = ring_target(every_kind);
    target.data.exported_ring.texture_count = 0;
    expect_attach_status(
      MLN_STATUS_INVALID_ARGUMENT, map, MLN_HANDLE_NULL, &target
    );
    target.data.exported_ring.texture_count = MLN_TEXTURE_RING_MAX_DEPTH + 1;
    expect_attach_status(
      MLN_STATUS_INVALID_ARGUMENT, map, MLN_HANDLE_NULL, &target
    );

    target = ring_target(0);
    expect_attach_status(
      MLN_STATUS_INVALID_ARGUMENT, map, MLN_HANDLE_NULL, &target
    );
    target = ring_target(every_kind);
    target.data.exported_ring.export_kinds = every_kind + 1U;
    expect_attach_status(
      MLN_STATUS_INVALID_ARGUMENT, map, MLN_HANDLE_NULL, &target
    );

    target = ring_target(every_kind);
    target.data.exported_ring.logical_size.width = 0;
    expect_attach_status(
      MLN_STATUS_INVALID_ARGUMENT, map, MLN_HANDLE_NULL, &target
    );

    // A dmabuf ring needs the formats that the host imports.
    target = ring_target(MLN_GPU_EXPORT_KIND_FLAG_DMA_BUF);
    target.data.exported_ring.dma_buf_formats = NULL;
    target.data.exported_ring.dma_buf_format_count = 0;
    expect_attach_status(
      MLN_STATUS_INVALID_ARGUMENT, map, MLN_HANDLE_NULL, &target
    );
  }
  destroy_map_and_runtime(map, runtime);
}

// A ring that the library allocates resizes rather than taking a new target,
// and keeps its textures until the session ends.
static void an_exported_ring_keeps_its_target(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_gpu_export_kind_flags exported = 0;
  const mln_gpu_device device = create_device(&exported);
  if (TEST_PROTECT()) {
    if (exported != 0) {
      const mln_render_target target = ring_target(exported);
      mln_test_completion completion = mln_test_completion_default(0);
      MLN_TEST_OK(start_attach(map, device, &target, &completion, &fixture));
      MLN_TEST_OK(
        mln_test_render_fixture_finish_operation(&fixture, &completion)
      );
      mln_test_completion_destroy(&completion);
      completion = mln_test_completion_default(0);
      MLN_TEST_STATUS(
        MLN_STATUS_UNSUPPORTED,
        mln_render_session_set_target(
          fixture.session, &target, &completion.descriptor, MLN_TEST_DIAGNOSTIC
        )
      );
      mln_test_completion_reject(&completion);
      mln_test_completion_destroy(&completion);
      completion = mln_test_completion_default(0);
      MLN_TEST_STATUS(
        MLN_STATUS_UNSUPPORTED,
        mln_render_session_release_target(
          fixture.session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
        )
      );
      mln_test_completion_reject(&completion);
      mln_test_completion_destroy(&completion);
    }
  }
  release_session(&fixture);
  mln_gpu_device_release(device);
  destroy_map_and_runtime(map, runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_ring_takes_the_first_kind_that_its_device_exports);
  RUN_TEST(a_ring_with_no_kind_that_its_device_exports_is_unsupported);
  RUN_TEST(an_exported_ring_rejects_invalid_descriptors);
  RUN_TEST(an_exported_ring_keeps_its_target);
}
