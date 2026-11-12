// Sessions that render into a ring of Android hardware buffers that the
// library allocates. A case reads a frame's buffer on the CPU, as a host that
// asks for AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN can.
//
// The executable targets API level 24, and the hardware buffer functions came
// in API level 26, so the cases look them up as the library does.

#include <android/hardware_buffer.h>
#include <dlfcn.h>
#include <poll.h>

#include "support/frames.h"
#include "support/test_support.h"

enum { ring_width = 64, ring_height = 48, buffer_count = 3 };

typedef struct hardware_buffer_functions {
  int (*lock)(AHardwareBuffer*, uint64_t, int32_t, const ARect*, void**);
  int (*unlock)(AHardwareBuffer*, int32_t*);
  void (*describe)(const AHardwareBuffer*, AHardwareBuffer_Desc*);
} hardware_buffer_functions;

static hardware_buffer_functions load_functions(void) {
  void* library = dlopen("libnativewindow.so", RTLD_NOW | RTLD_LOCAL);
  TEST_ASSERT_NOT_NULL(library);
  hardware_buffer_functions functions = {
    .lock = (int (*)(AHardwareBuffer*, uint64_t, int32_t, const ARect*, void**))
      dlsym(library, "AHardwareBuffer_lock"),
    .unlock = (int (*)(AHardwareBuffer*, int32_t*))dlsym(
      library, "AHardwareBuffer_unlock"
    ),
    .describe = (void (*)(const AHardwareBuffer*, AHardwareBuffer_Desc*))dlsym(
      library, "AHardwareBuffer_describe"
    ),
  };
  TEST_ASSERT_NOT_NULL(functions.lock);
  TEST_ASSERT_NOT_NULL(functions.unlock);
  TEST_ASSERT_NOT_NULL(functions.describe);
  return functions;
}

static mln_status start_attach(
  mln_map map, mln_render_driver_kind driver, const mln_queue_lock* queue_lock,
  mln_test_completion* completion, mln_test_render_fixture* fixture
) {
  const mln_render_target target = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_EXPORTED_RING,
    .data.exported_ring = {
      .logical_size = {.width = ring_width, .height = ring_height},
      .texture_count = buffer_count,
      .export_kinds = MLN_GPU_EXPORT_KIND_FLAG_ANDROID_HARDWARE_BUFFER,
      .android_hardware_buffer_usage = AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN,
    },
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options),
    .driver = driver,
    .producer_sync_kinds =
      MLN_GPU_SYNC_KIND_FLAG_SYNC_FD | MLN_GPU_SYNC_KIND_FLAG_CPU_COMPLETE,
  };
  options.frame_wake = mln_test_pulse_wake();
  // A host's queue lock is its device's.
  mln_gpu_device device = MLN_HANDLE_NULL;
  if (queue_lock != NULL) {
    mln_gpu_device_options device_options = {
      .size = sizeof(mln_gpu_device_options),
      .fields = MLN_GPU_DEVICE_OPTION_BACKEND,
      .backend = MLN_RENDER_BACKEND_VULKAN,
      .queue_lock = *queue_lock,
    };
    MLN_TEST_OK(mln_gpu_device_create(&device_options, &device, NULL));
  }
  fixture->driver = driver;
  const mln_status status = mln_map_attach(
    map, &target, device, &options, &fixture->session, &completion->descriptor,
    MLN_TEST_DIAGNOSTIC
  );
  mln_gpu_device_release(device);
  if (status != MLN_STATUS_OK) mln_test_completion_reject(completion);
  return status;
}

static void attach_with_lock(
  mln_map map, const mln_queue_lock* queue_lock,
  mln_test_render_fixture* fixture
) {
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(start_attach(
    map, MLN_RENDER_DRIVER_CORE_WORKER, queue_lock, &completion, fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &completion));
  mln_test_completion_destroy(&completion);
}

static void attach(mln_map map, mln_test_render_fixture* fixture) {
  attach_with_lock(map, NULL, fixture);
}

static mln_android_hardware_buffer_frame frame_buffer(
  mln_acquired_frame frame
) {
  mln_android_hardware_buffer_frame buffer = {
    .size = sizeof(mln_android_hardware_buffer_frame)
  };
  MLN_TEST_OK(mln_acquired_frame_get_android_hardware_buffer(
    frame, &buffer, MLN_TEST_DIAGNOSTIC
  ));
  TEST_ASSERT_NOT_NULL(buffer.buffer);
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

// Expects the style's red at the center of the frame's buffer.
static void expect_red_buffer(
  const hardware_buffer_functions* functions, mln_acquired_frame frame
) {
  wait_for_producer(frame);
  AHardwareBuffer* buffer = frame_buffer(frame).buffer;
  AHardwareBuffer_Desc description = {0};
  functions->describe(buffer, &description);
  void* data = NULL;
  TEST_ASSERT_EQUAL_INT(
    0, functions->lock(
         buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, &data
       )
  );
  uint8_t pixel[4] = {0};
  const size_t offset = (((size_t)description.height / 2) * description.stride +
                         description.width / 2) *
                        4;
  memcpy(pixel, (const uint8_t*)data + offset, sizeof(pixel));
  TEST_ASSERT_EQUAL_INT(0, functions->unlock(buffer, NULL));
  TEST_ASSERT_EQUAL_HEX8(0xff, pixel[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, pixel[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, pixel[2]);
  TEST_ASSERT_EQUAL_HEX8(0xff, pixel[3]);
}

// A fill over the northern hemisphere, which covers the top half of a view
// centered on 0,0 at zoom 0.
static const char north_fill_style_json[] =
  "{\"version\":8,\"sources\":{\"north\":{\"type\":\"geojson\",\"data\":"
  "{\"type\":\"Feature\",\"geometry\":{\"type\":\"Polygon\","
  "\"coordinates\":[[[-170,0],[170,0],[170,80],[-170,80],[-170,0]]]},"
  "\"properties\":{}}}},"
  "\"layers\":[{\"id\":\"bg\",\"type\":\"background\","
  "\"paint\":{\"background-color\":\"#ff0000\"}},"
  "{\"id\":\"north\",\"type\":\"fill\",\"source\":\"north\","
  "\"paint\":{\"fill-color\":\"#0000ff\"}}]}";

// Expects the fill of north_fill_style_json in the top half of the frame's
// buffer, which a host takes as starting with the top row, and the background
// in its bottom half.
static void expect_north_up_buffer(
  const hardware_buffer_functions* functions, mln_acquired_frame frame
) {
  wait_for_producer(frame);
  AHardwareBuffer* buffer = frame_buffer(frame).buffer;
  AHardwareBuffer_Desc description = {0};
  functions->describe(buffer, &description);
  void* data = NULL;
  TEST_ASSERT_EQUAL_INT(
    0, functions->lock(
         buffer, AHARDWAREBUFFER_USAGE_CPU_READ_OFTEN, -1, NULL, &data
       )
  );
  uint8_t top[4] = {0};
  uint8_t bottom[4] = {0};
  const size_t column = description.width / 2;
  const size_t top_row = description.height / 4;
  const size_t bottom_row = description.height - 1 - top_row;
  memcpy(
    top, (const uint8_t*)data + (top_row * description.stride + column) * 4,
    sizeof(top)
  );
  memcpy(
    bottom,
    (const uint8_t*)data + (bottom_row * description.stride + column) * 4,
    sizeof(bottom)
  );
  TEST_ASSERT_EQUAL_INT(0, functions->unlock(buffer, NULL));
  static const uint8_t blue[4] = {0x00, 0x00, 0xff, 0xff};
  static const uint8_t red[4] = {0xff, 0x00, 0x00, 0xff};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(blue, top, 4);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(red, bottom, 4);
}

// Expects the frame's buffer to have the size in pixels of `logical_width` by
// `logical_height`.
static void expect_buffer_size(
  const hardware_buffer_functions* functions, mln_acquired_frame frame,
  uint32_t logical_width, uint32_t logical_height
) {
  const mln_android_hardware_buffer_frame buffer = frame_buffer(frame);
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
  AHardwareBuffer_Desc description = {0};
  functions->describe(buffer.buffer, &description);
  TEST_ASSERT_EQUAL_UINT32(width, description.width);
  TEST_ASSERT_EQUAL_UINT32(height, description.height);
  TEST_ASSERT_EQUAL_UINT32(
    AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM, description.format
  );
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

// Unity aborts at every assertion after a failed one, back into the case's
// TEST_PROTECT, so a teardown that asserts would run again without end. After
// a failure, the harness reclaims the map and the runtime instead.
static void destroy_map_and_runtime(mln_map map, mln_runtime runtime) {
  if (Unity.CurrentTestFailed) return;
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_map create_red_map(mln_runtime runtime) {
  const mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  return map;
}

static void a_frame_renders_into_a_buffer_that_the_host_reads(void) {
  const hardware_buffer_functions functions = load_functions();
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  if (TEST_PROTECT()) {
    attach(map, &fixture);
    mln_render_session_capabilities capabilities = {
      .size = sizeof(mln_render_session_capabilities)
    };
    MLN_TEST_OK(mln_render_session_get_capabilities(
      fixture.session, &capabilities, MLN_TEST_DIAGNOSTIC
    ));
    TEST_ASSERT_EQUAL_UINT32(buffer_count, capabilities.texture_ring_depth);
    TEST_ASSERT_TRUE(
      (capabilities.flags & MLN_RENDER_SESSION_CAPABILITY_FRAME_ACQUISITION) !=
      0
    );

    frame = mln_test_render_and_acquire(&fixture, 1);
    expect_buffer_size(&functions, frame, ring_width, ring_height);
    expect_red_buffer(&functions, frame);
    release_frame(&frame);
  }
  release_frame(&frame);
  release_session(&fixture);
  destroy_map_and_runtime(map, runtime);
}

static void a_frame_has_the_maps_top_as_its_first_row(void) {
  const hardware_buffer_functions functions = load_functions();
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  if (TEST_PROTECT()) {
    mln_test_load_style_and_wait(
      map, MLN_BUFFER_LITERAL(north_fill_style_json)
    );
    attach(map, &fixture);
    mln_test_render_until_idle(runtime, map, &fixture);
    frame = mln_test_render_and_acquire(&fixture, 1);
    expect_north_up_buffer(&functions, frame);
    release_frame(&frame);
  }
  release_frame(&frame);
  release_session(&fixture);
  destroy_map_and_runtime(map, runtime);
}

// A host keeps a frame acquired while it shows the frame, so the session
// renders the next frames into the ring's other buffers.
static void a_held_frame_keeps_its_buffer(void) {
  const hardware_buffer_functions functions = load_functions();
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame held = MLN_HANDLE_NULL;
  mln_acquired_frame next = MLN_HANDLE_NULL;
  if (TEST_PROTECT()) {
    attach(map, &fixture);
    held = mln_test_render_and_acquire(&fixture, 1);
    const mln_android_hardware_buffer_frame first = frame_buffer(held);

    for (uint64_t token = 2; token < 2 + buffer_count; ++token) {
      next = mln_test_render_and_acquire(&fixture, token);
      const mln_android_hardware_buffer_frame other = frame_buffer(next);
      TEST_ASSERT_NOT_EQUAL_UINT32(first.slot, other.slot);
      TEST_ASSERT_TRUE(first.buffer != other.buffer);
      expect_red_buffer(&functions, next);
      release_frame(&next);
    }

    const mln_android_hardware_buffer_frame again = frame_buffer(held);
    TEST_ASSERT_EQUAL_UINT32(first.slot, again.slot);
    TEST_ASSERT_TRUE(first.buffer == again.buffer);
    expect_red_buffer(&functions, held);
    release_frame(&held);
  }
  release_frame(&next);
  release_frame(&held);
  release_session(&fixture);
  destroy_map_and_runtime(map, runtime);
}

static void a_resize_gives_the_ring_buffers_of_the_new_size(void) {
  const hardware_buffer_functions functions = load_functions();
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  if (TEST_PROTECT()) {
    attach(map, &fixture);
    frame = mln_test_render_and_acquire(&fixture, 1);
    expect_buffer_size(&functions, frame, ring_width, ring_height);
    release_frame(&frame);

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
    frame = mln_test_render_and_acquire(&fixture, 2);
    expect_buffer_size(&functions, frame, ring_height, ring_width);
    expect_red_buffer(&functions, frame);
    release_frame(&frame);
  }
  release_frame(&frame);
  release_session(&fixture);
  destroy_map_and_runtime(map, runtime);
}

// The library owns the graphics context, so it drives the session itself.
static void a_hardware_buffer_ring_rejects_the_callers_graphics_thread(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  if (TEST_PROTECT()) {
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED, start_attach(
                                map, MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD,
                                NULL, &completion, &fixture
                              )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  destroy_map_and_runtime(map, runtime);
}

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
// Counts the session's calls to the host's queue lock. It has file scope,
// because the session can call it after a case that fails partway.
typedef struct counted_queue_lock {
  atomic_uint locks;
  atomic_uint unlocks;
} counted_queue_lock;

static counted_queue_lock host_lock;

static void take_host_lock(void* user_data) {
  atomic_fetch_add(&((counted_queue_lock*)user_data)->locks, 1);
}

static void give_host_lock(void* user_data) {
  atomic_fetch_add(&((counted_queue_lock*)user_data)->unlocks, 1);
}

// A host that submits to the device's queue itself gives the device its lock,
// which the session holds around its own queue calls.
static void a_hardware_buffer_ring_takes_the_host_queue_lock(void) {
  atomic_store(&host_lock.locks, 0);
  atomic_store(&host_lock.unlocks, 0);
  const mln_queue_lock queue_lock = {
    .lock = take_host_lock,
    .unlock = give_host_lock,
    .user_data = &host_lock,
  };
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_render_fixture fixture = {0};
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  if (TEST_PROTECT()) {
    attach_with_lock(map, &queue_lock, &fixture);
    frame = mln_test_render_and_acquire(&fixture, 1);
    release_frame(&frame);
    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_detach(
        fixture.session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
    TEST_ASSERT_GREATER_THAN_UINT(0, atomic_load(&host_lock.locks));
    TEST_ASSERT_EQUAL_UINT(
      atomic_load(&host_lock.locks), atomic_load(&host_lock.unlocks)
    );
  }
  release_frame(&frame);
  release_session(&fixture);
  destroy_map_and_runtime(map, runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(a_frame_renders_into_a_buffer_that_the_host_reads);
  RUN_TEST(a_frame_has_the_maps_top_as_its_first_row);
  RUN_TEST(a_held_frame_keeps_its_buffer);
  RUN_TEST(a_resize_gives_the_ring_buffers_of_the_new_size);
  RUN_TEST(a_hardware_buffer_ring_rejects_the_callers_graphics_thread);
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  RUN_TEST(a_hardware_buffer_ring_takes_the_host_queue_lock);
#endif
}
