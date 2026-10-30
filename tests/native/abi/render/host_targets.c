// Targets that the host creates: a session renders into a borrowed ring and
// presents to a surface, both made by tests/graphics for the preset's backend.
// The browser presets have no such fixtures, because JavaScript owns their
// canvases.

#if defined(MLN_FFI_TEST_OPENGL_EGL)
#include <EGL/egl.h>
#endif
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
#include <vulkan/vulkan_core.h>
#endif

#include "support/frames.h"
#include "support/host_graphics.h"
#include "support/test_support.h"

#if !defined(__EMSCRIPTEN__)
// Renders one frame of the map's style with `flags`, and returns that frame's
// disposition.
static uint32_t render_frame(
  const mln_test_render_fixture* fixture, uint64_t token, uint32_t flags
) {
  mln_frame_demand demand = {.size = sizeof(mln_frame_demand)};
  demand.flags = MLN_FRAME_DEMAND_FORCE | flags;
  demand.token = token;
  MLN_TEST_OK(
    mln_render_session_request_frame(fixture->session, &demand, NULL)
  );
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
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
  MLN_TEST_OK(mln_test_map_fence(map));
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
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
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
    MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION, capabilities.flags
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_GPU_SYNC_CPU_COMPLETE, capabilities.producer_sync_kind
  );
  TEST_ASSERT_BITS_HIGH(
    MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE, capabilities.consumer_sync_kinds
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
    mln_test_drain_frame_results(fixture.session, &batch, NULL)
  );
  mln_test_render_release_frame(&first);
  batch = mln_test_render_wait_for_results(&fixture);
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

#if defined(MLN_FFI_TEST_BACKEND_VULKAN) || defined(MLN_FFI_TEST_BACKEND_METAL)
// A queue-ordered frame is acquirable once submitted. The host reads its
// texture on the queue that it shares with the session, with no wait of its
// own, and releases the frame in queue order, after which the session renders
// into the slot again.
static void queue_ordered_frames_share_the_hosts_queue(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_synced(
      map, &fixture, true,
      MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE | MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER
    ),
    mln_test_graphics_last_error()
  );
  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_GPU_SYNC_QUEUE_ORDER, capabilities.producer_sync_kind
  );
  TEST_ASSERT_BITS_HIGH(
    MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER, capabilities.consumer_sync_kinds
  );

  const uint8_t* const colors[] = {red, blue};
  for (uint64_t index = 0; index < 2; index += 1) {
    if (index == 1) paint_background_blue(runtime, map);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, index + 1, 0)
    );
    mln_acquired_frame frame = MLN_HANDLE_NULL;
    MLN_TEST_OK(
      mln_render_session_acquire_frame(fixture.session, &frame, NULL)
    );
    mln_gpu_sync producer = {.size = sizeof(mln_gpu_sync)};
    MLN_TEST_OK(mln_acquired_frame_get_producer_sync(frame, &producer, NULL));
    TEST_ASSERT_EQUAL_UINT32(MLN_GPU_SYNC_QUEUE_ORDER, producer.kind);
    TEST_ASSERT_EQUAL_UINT64(0, producer.handle);
    uint32_t slot = UINT32_MAX;
    TEST_ASSERT_NOT_EQUAL_UINT64(
      0, mln_test_frame_texture_handle(frame, &slot)
    );
    TEST_ASSERT_EQUAL_UINT32(0, slot);
    expect_texture_color(
      mln_test_render_fixture_texture(&fixture, slot), colors[index],
      "the queue-ordered frame's texture"
    );
    const mln_gpu_sync consumer = {
      .size = sizeof(mln_gpu_sync), .kind = MLN_GPU_SYNC_QUEUE_ORDER
    };
    MLN_TEST_OK(mln_acquired_frame_release(frame, &consumer, NULL));
  }
  // Detach right after a queue-ordered frame and its release, which may both
  // still run on the queue, waits for them before it destroys the ring.
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 3, 0)
  );
  mln_acquired_frame last = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_render_session_acquire_frame(fixture.session, &last, NULL));
  const mln_gpu_sync queued = {
    .size = sizeof(mln_gpu_sync), .kind = MLN_GPU_SYNC_QUEUE_ORDER
  };
  MLN_TEST_OK(mln_acquired_frame_release(last, &queued, NULL));
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A queue-ordered owned ring rebuilds its textures at a new size once the
// queue it shares with the host has finished with the old ones.
static void a_queue_ordered_ring_resizes_after_the_hosts_reads(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_synced(
      map, &fixture, false, MLN_GPU_SYNC_KIND_FLAG_QUEUE_ORDER
    ),
    mln_test_graphics_last_error()
  );
  const mln_gpu_sync consumer = {
    .size = sizeof(mln_gpu_sync), .kind = MLN_GPU_SYNC_QUEUE_ORDER
  };
  const mln_logical_extent smaller = {
    .width = MLN_TEST_HOST_TARGET_SIZE / 2,
    .height = MLN_TEST_HOST_TARGET_SIZE / 2,
    .scale_factor = 1.0,
  };
  for (uint64_t index = 0; index < 2; index += 1) {
    if (index == 1) {
      MLN_TEST_RENDER_AWAIT(
        MLN_STATUS_OK, &fixture,
        mln_render_session_resize(
          fixture.session, smaller, &completion.descriptor, NULL
        )
      );
    }
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, index + 1, 0)
    );
    mln_acquired_frame frame = MLN_HANDLE_NULL;
    MLN_TEST_OK(
      mln_render_session_acquire_frame(fixture.session, &frame, NULL)
    );
    MLN_TEST_OK(mln_acquired_frame_release(frame, &consumer, NULL));
  }
  MLN_TEST_STATUS(MLN_STATUS_OK, read_back(&fixture));
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// The Android emulator's Vulkan device has no timeline semaphores; every other
// CI driver does.
#if defined(__ANDROID__)
#define TIMELINES_REQUIRED 0
#else
#define TIMELINES_REQUIRED 1
#endif

// A timeline frame is acquirable once submitted: the session signals its
// semaphore after the frame, and the host waits for the frame's value before
// it reads. The host releases the frame with its own timeline, and the
// session's next frame waits until the host signals it. A device without
// timelines refuses them.
static void vulkan_timelines_order_frame_reads_and_slot_reuse(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_synced(
      map, &fixture, true,
      MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE |
        MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE
    ),
    mln_test_graphics_last_error()
  );
  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  if (
    capabilities.producer_sync_kind != MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE
  ) {
    TEST_ASSERT_FALSE_MESSAGE(
      TIMELINES_REQUIRED, "this target's driver has no timeline semaphores"
    );
    TEST_ASSERT_EQUAL_UINT32(
      MLN_GPU_SYNC_CPU_COMPLETE, capabilities.producer_sync_kind
    );
    TEST_ASSERT_BITS_LOW(
      MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE,
      capabilities.consumer_sync_kinds
    );
    mln_test_render_fixture_destroy(&fixture);
    mln_test_render_fixture fallback = {0};
    TEST_ASSERT_FALSE(mln_test_render_fixture_create_synced(
      map, &fallback, true, MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE
    ));
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
    return;
  }
  TEST_ASSERT_BITS_HIGH(
    MLN_GPU_SYNC_KIND_FLAG_VULKAN_TIMELINE_SEMAPHORE,
    capabilities.consumer_sync_kinds
  );
  mln_test_graphics* graphics = mln_test_render_fixture_graphics(&fixture);

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 1, 0)
  );
  mln_acquired_frame first = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_render_session_acquire_frame(fixture.session, &first, NULL));
  mln_gpu_sync producer = {.size = sizeof(mln_gpu_sync)};
  MLN_TEST_OK(mln_acquired_frame_get_producer_sync(first, &producer, NULL));
  TEST_ASSERT_EQUAL_UINT32(
    MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE, producer.kind
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(0, producer.handle);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, producer.value);
  const uint64_t produced = producer.handle;
  const uint64_t first_value = producer.value;
  TEST_ASSERT_EQUAL_INT_MESSAGE(
    1,
    mln_test_graphics_vulkan_timeline_wait(
      graphics, produced, first_value, UINT64_MAX
    ),
    mln_test_graphics_last_error()
  );
  expect_texture_color(
    mln_test_render_fixture_texture(&fixture, 0), red,
    "the first timeline frame's texture"
  );

  // The host's timeline is still at zero when it releases the frame.
  const uint64_t consumed = mln_test_graphics_vulkan_timeline_create(graphics);
  TEST_ASSERT_NOT_EQUAL_UINT64_MESSAGE(
    0, consumed, mln_test_graphics_last_error()
  );
  const mln_gpu_sync consumer = {
    .size = sizeof(mln_gpu_sync),
    .kind = MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE,
    .handle = consumed,
    .value = 1,
  };
  MLN_TEST_OK(mln_acquired_frame_release(first, &consumer, NULL));

  paint_background_blue(runtime, map);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 2, 0)
  );
  mln_acquired_frame second = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_render_session_acquire_frame(fixture.session, &second, NULL));
  MLN_TEST_OK(mln_acquired_frame_get_producer_sync(second, &producer, NULL));
  TEST_ASSERT_EQUAL_UINT64(produced, producer.handle);
  TEST_ASSERT_GREATER_THAN_UINT64(first_value, producer.value);
  // The second frame waits for the host's semaphore.
  TEST_ASSERT_EQUAL_INT(
    0, mln_test_graphics_vulkan_timeline_wait(
         graphics, produced, producer.value, 0
       )
  );
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_graphics_vulkan_timeline_signal(graphics, consumed, 1),
    mln_test_graphics_last_error()
  );
  TEST_ASSERT_EQUAL_INT(
    1, mln_test_graphics_vulkan_timeline_wait(
         graphics, produced, producer.value, UINT64_MAX
       )
  );
  expect_texture_color(
    mln_test_render_fixture_texture(&fixture, 0), blue,
    "the reused texture after the host's timeline"
  );
  mln_test_render_release_frame(&second);
  // The host's timeline stays valid until detach completes.
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  mln_test_graphics_vulkan_timeline_destroy(graphics, consumed);
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// A ring whose image goes to another owner between frames hands each frame
// over, and the session takes the image back to render into it again. Only an
// image without contents to keep can have another owner.
static void vulkan_images_go_to_their_owner_between_frames(void) {
  const uint32_t layouts[] = {
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED
  };
  for (size_t layout = 0; layout < 2; layout += 1) {
    mln_runtime runtime = mln_test_create_runtime();
    mln_map map = mln_test_create_map(runtime);
    mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
    mln_test_render_fixture fixture = {0};
    const bool attached = mln_test_render_fixture_create_vulkan_handoff_ring(
      map, &fixture, MLN_VULKAN_IMAGE_OWNER_EXTERNAL, layouts[layout]
    );
    if (layouts[layout] != VK_IMAGE_LAYOUT_UNDEFINED) {
      TEST_ASSERT_FALSE(attached);
      TEST_ASSERT_NOT_NULL(strstr(mln_test_last_error(), "initial_layout"));
      mln_test_destroy_map(map);
      mln_test_destroy_runtime(runtime);
      continue;
    }
    TEST_ASSERT_TRUE_MESSAGE(attached, mln_test_graphics_last_error());
    mln_test_graphics* graphics = mln_test_render_fixture_graphics(&fixture);
    for (uint64_t index = 0; index < 2; index += 1) {
      TEST_ASSERT_EQUAL_UINT32(
        MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, index + 1, 0)
      );
      mln_acquired_frame frame = MLN_HANDLE_NULL;
      MLN_TEST_OK(
        mln_render_session_acquire_frame(fixture.session, &frame, NULL)
      );
      mln_gpu_sync producer = {.size = sizeof(mln_gpu_sync)};
      MLN_TEST_OK(mln_acquired_frame_get_producer_sync(frame, &producer, NULL));
      // A frame without a timeline completed with its release.
      if (producer.kind == MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE) {
        TEST_ASSERT_EQUAL_INT(
          1, mln_test_graphics_vulkan_timeline_wait(
               graphics, producer.handle, producer.value, UINT64_MAX
             )
        );
      }
      mln_test_render_release_frame(&frame);
    }
    mln_test_render_fixture_destroy(&fixture);
    mln_test_destroy_map(map);
    mln_test_destroy_runtime(runtime);
  }
}
#endif

#if defined(MLN_FFI_TEST_OPENGL_EGL)
static void shared_opengl_fences_order_frame_reads_and_slot_reuse(
  bool borrowed
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_fixture fixture = {0};
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_render_fixture_create_synced(
      map, &fixture, borrowed, MLN_GPU_SYNC_KIND_FLAG_OPEN_GL_FENCE
    ),
    mln_test_graphics_last_error()
  );
  void* (*fence_sync)(unsigned, unsigned) =
    (void* (*)(unsigned, unsigned))eglGetProcAddress("glFenceSync");
  void (*wait_sync)(void*, unsigned, uint64_t) =
    (void (*)(void*, unsigned, uint64_t))eglGetProcAddress("glWaitSync");
  unsigned char (*is_sync)(void*) =
    (unsigned char (*)(void*))eglGetProcAddress("glIsSync");
  void (*flush)(void) = (void (*)(void))eglGetProcAddress("glFlush");
  void (*finish)(void) = (void (*)(void))eglGetProcAddress("glFinish");
  TEST_ASSERT_NOT_NULL(fence_sync);
  TEST_ASSERT_NOT_NULL(wait_sync);
  TEST_ASSERT_NOT_NULL(is_sync);
  TEST_ASSERT_NOT_NULL(flush);
  TEST_ASSERT_NOT_NULL(finish);

  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 1, 0)
  );
  mln_acquired_frame first = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_render_session_acquire_frame(fixture.session, &first, NULL));
  mln_gpu_sync producer = {.size = sizeof(mln_gpu_sync)};
  MLN_TEST_OK(mln_acquired_frame_get_producer_sync(first, &producer, NULL));
  TEST_ASSERT_EQUAL_UINT32(MLN_GPU_SYNC_OPEN_GL_FENCE, producer.kind);
  TEST_ASSERT_TRUE(is_sync((void*)(uintptr_t)producer.handle));
  wait_sync((void*)(uintptr_t)producer.handle, 0, UINT64_MAX);
  uint32_t first_slot = UINT32_MAX;
  TEST_ASSERT_NOT_EQUAL_UINT64(
    0, mln_test_frame_texture_handle(first, &first_slot)
  );
  if (borrowed) {
    expect_texture_color(
      mln_test_render_fixture_texture(&fixture, first_slot), red,
      "the fence-protected first texture"
    );
  } else {
    MLN_TEST_STATUS(MLN_STATUS_OK, read_back(&fixture));
  }

  // An invalid fence keeps the frame handle available for a valid release.
  mln_gpu_sync consumer = {
    .size = sizeof(consumer), .kind = MLN_GPU_SYNC_OPEN_GL_FENCE
  };
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_acquired_frame_release(first, &consumer, MLN_TEST_DIAGNOSTIC)
  );
  consumer.handle = (uint64_t)(uintptr_t)fence_sync(0x9117, 0);
  TEST_ASSERT_NOT_EQUAL_UINT64(0, consumer.handle);
  flush();
  MLN_TEST_OK(mln_acquired_frame_release(first, &consumer, NULL));
  first = MLN_HANDLE_NULL;
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_barrier(fixture.session, &completion.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(is_sync((void*)(uintptr_t)consumer.handle));

  paint_background_blue(runtime, map);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_RESULT_RENDERED, render_frame(&fixture, 2, 0)
  );
  mln_acquired_frame second = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_render_session_acquire_frame(fixture.session, &second, NULL));
  MLN_TEST_OK(mln_acquired_frame_get_producer_sync(second, &producer, NULL));
  TEST_ASSERT_EQUAL_UINT32(MLN_GPU_SYNC_OPEN_GL_FENCE, producer.kind);
  TEST_ASSERT_TRUE(is_sync((void*)(uintptr_t)producer.handle));
  wait_sync((void*)(uintptr_t)producer.handle, 0, UINT64_MAX);
  uint32_t second_slot = UINT32_MAX;
  TEST_ASSERT_NOT_EQUAL_UINT64(
    0, mln_test_frame_texture_handle(second, &second_slot)
  );
  TEST_ASSERT_EQUAL_UINT32(first_slot, second_slot);
  if (borrowed) {
    expect_texture_color(
      mln_test_render_fixture_texture(&fixture, second_slot), blue,
      "the reused texture after the consumer fence"
    );
  } else {
    MLN_TEST_STATUS(MLN_STATUS_OK, read_back(&fixture));
  }
  finish();
  mln_test_render_release_frame(&second);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_detach(fixture.session, &completion.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(is_sync((void*)(uintptr_t)producer.handle));
  mln_test_render_fixture_destroy(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void borrowed_opengl_frames_support_gpu_fences(void) {
  shared_opengl_fences_order_frame_reads_and_slot_reuse(true);
}

static void owned_opengl_frames_support_gpu_fences(void) {
  shared_opengl_fences_order_frame_reads_and_slot_reuse(false);
}
#endif

// A surface presents the frames it renders and reads none back. A resize
// reaches the host's surface, so the next frame, which presents nothing,
// renders at the new extent.
static void a_surface_presents_frames_and_takes_a_resize(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
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
  mln_test_render_prepare_map(map);
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
  mln_metal_surface_descriptor descriptor = {
    .size = sizeof(mln_metal_surface_descriptor)
  };
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
#if defined(MLN_FFI_TEST_BACKEND_VULKAN) || defined(MLN_FFI_TEST_BACKEND_METAL)
  RUN_TEST(queue_ordered_frames_share_the_hosts_queue);
  RUN_TEST(a_queue_ordered_ring_resizes_after_the_hosts_reads);
#endif
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  RUN_TEST(vulkan_timelines_order_frame_reads_and_slot_reuse);
  RUN_TEST(vulkan_images_go_to_their_owner_between_frames);
#endif
#if defined(MLN_FFI_TEST_OPENGL_EGL)
  RUN_TEST(borrowed_opengl_frames_support_gpu_fences);
  RUN_TEST(owned_opengl_frames_support_gpu_fences);
#endif
  RUN_TEST(a_surface_presents_frames_and_takes_a_resize);
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  RUN_TEST(a_surface_replacement_can_change_the_scale_factor);
#endif
#endif
}
