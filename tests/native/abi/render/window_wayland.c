// Sessions that present to a Wayland surface with a graphics context that the
// library creates. A compositor in the test process stands in for a desktop's,
// so a case reads back what the session presents. Each case destroys its
// surfaces and stops the compositor even after a failed assertion, once its
// session no longer uses them.

#include <string.h>

#include "support/frames.h"
#include "support/test_support.h"
#include "support/wayland_compositor.h"

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
#include <vulkan/vulkan_core.h>
#endif

enum { window_size = 64 };

// Waits for the surface's next frame, and expects the style's red at its
// center, at `width` by `height` pixels.
static void expect_red_frame(
  mln_test_wayland* wayland, void* surface, int32_t width, int32_t height
) {
  mln_test_wayland_frame frame = {0};
  TEST_ASSERT_TRUE(mln_test_wayland_wait_for_frame(wayland, surface, &frame));
  TEST_ASSERT_EQUAL_INT32(width, frame.width);
  TEST_ASSERT_EQUAL_INT32(height, frame.height);
  TEST_ASSERT_EQUAL_HEX8(0xff, frame.center[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, frame.center[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, frame.center[2]);
  TEST_ASSERT_EQUAL_HEX8(0xff, frame.center[3]);
}

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
static mln_vulkan_device create_device(
  bool presentation, const uint8_t* device_uuid
) {
  mln_vulkan_device_options options = {
    .size = sizeof(mln_vulkan_device_options)
  };
  options.fields = MLN_VULKAN_DEVICE_OPTION_PRESENTATION;
  options.presentation = presentation;
  if (device_uuid != NULL) {
    options.fields |= MLN_VULKAN_DEVICE_OPTION_DEVICE_UUID;
    options.device_uuid =
      (mln_buffer_view){.data = device_uuid, .size = VK_UUID_SIZE};
  }
  mln_vulkan_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_vulkan_device_create(&options, &device, MLN_TEST_DIAGNOSTIC));
  return device;
}

// Finds the CPU's Vulkan device, such as lavapipe, through the instance of a
// device that the library creates.
static void find_cpu_device(uint8_t out_uuid[VK_UUID_SIZE]) {
  const mln_vulkan_device probe = create_device(false, NULL);
  mln_vulkan_device_context_result result = {.size = sizeof(result)};
  MLN_TEST_OK(
    mln_vulkan_device_get_context(probe, &result, MLN_TEST_DIAGNOSTIC)
  );
  const PFN_vkGetInstanceProcAddr get_instance_proc_addr =
    (PFN_vkGetInstanceProcAddr)(uintptr_t)result.context.get_instance_proc_addr;
  VkInstance instance = (VkInstance)result.context.instance;
  const PFN_vkEnumeratePhysicalDevices enumerate =
    (PFN_vkEnumeratePhysicalDevices)get_instance_proc_addr(
      instance, "vkEnumeratePhysicalDevices"
    );
  const PFN_vkGetPhysicalDeviceProperties2 get_properties =
    (PFN_vkGetPhysicalDeviceProperties2)get_instance_proc_addr(
      instance, "vkGetPhysicalDeviceProperties2"
    );
  VkPhysicalDevice devices[16];
  uint32_t count = 16;
  const VkResult listed = enumerate(instance, &count, devices);
  bool found = false;
  for (uint32_t index = 0; index < count && !found; ++index) {
    VkPhysicalDeviceIDProperties ids = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES,
    };
    VkPhysicalDeviceProperties2 properties = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      .pNext = &ids,
    };
    get_properties(devices[index], &properties);
    if (properties.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU) {
      memcpy(out_uuid, ids.deviceUUID, VK_UUID_SIZE);
      found = true;
    }
  }
  mln_vulkan_device_release(probe);
  TEST_ASSERT_TRUE(listed == VK_SUCCESS || listed == VK_INCOMPLETE);
  TEST_ASSERT_TRUE_MESSAGE(found, "no CPU Vulkan device, such as lavapipe");
}
#endif

// The device that a case presents with. A GPU's driver presents through
// dmabufs, whose memory the test compositor can't map, so a Vulkan build
// presents with the CPU's device, which uses shared memory, as Mesa's EGL does
// when a compositor offers no dmabufs.
static mln_vulkan_device create_presenting_device(void) {
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  uint8_t uuid[VK_UUID_SIZE] = {0};
  find_cpu_device(uuid);
  return create_device(true, uuid);
#else
  return MLN_HANDLE_NULL;
#endif
}

// Submits an attach to `surface` without waiting for it. The fixture holds the
// session once the call succeeds.
static mln_status start_attach(
  mln_map map, mln_test_wayland* wayland, void* surface,
  mln_vulkan_device device, mln_render_driver_kind driver,
  mln_test_completion* completion, mln_test_render_fixture* fixture
) {
  const mln_wayland_surface_descriptor descriptor = {
    .size = sizeof(mln_wayland_surface_descriptor),
    .logical_size = {.width = window_size, .height = window_size},
    .display = mln_test_wayland_display(wayland),
    .surface = surface,
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options), .driver = driver
  };
  options.frame_wake = mln_test_pulse_wake();
  fixture->driver = driver;
  const mln_status status = mln_map_attach_wayland_surface(
    map, &descriptor, device, &options, &fixture->session,
    &completion->descriptor, MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) mln_test_completion_reject(completion);
  return status;
}

// The session holds the device, so the host releases it once the attach call
// returns.
static void attach(
  mln_map map, mln_test_wayland* wayland, void* surface,
  mln_test_render_fixture* fixture
) {
  const mln_vulkan_device device = create_presenting_device();
  mln_test_completion completion = mln_test_completion_default(0);
  const mln_status attached = start_attach(
    map, wayland, surface, device, MLN_RENDER_DRIVER_CORE_WORKER, &completion,
    fixture
  );
  mln_vulkan_device_release(device);
  MLN_TEST_OK(attached);
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &completion));
  mln_test_completion_destroy(&completion);
}

static void render_frame(const mln_test_render_fixture* fixture) {
  mln_test_render_demand_forced(fixture, 0);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
}

static void set_surface(const mln_test_render_fixture* fixture, void* surface) {
  const mln_wayland_surface_replacement replacement = {
    .size = sizeof(mln_wayland_surface_replacement),
    .logical_size = {.width = window_size, .height = window_size},
    .surface = surface,
  };
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_set_wayland_surface_target(
      fixture->session, &replacement, &completion.descriptor,
      MLN_TEST_DIAGNOSTIC
    )
  );
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

static void stop_wayland(
  mln_test_wayland* wayland, void* const* surfaces, int count
) {
  if (wayland == NULL) return;
  for (int index = 0; index < count; ++index) {
    mln_test_wayland_destroy_surface(wayland, surfaces[index]);
  }
  mln_test_wayland_stop(wayland);
}

static mln_map create_red_map(mln_runtime runtime) {
  const mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  return map;
}

// A Wayland client sizes the surface's buffers itself, so a resize has to
// reach them.
static void a_session_presents_to_a_surface_at_its_size(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_wayland* wayland = NULL;
  void* surfaces[1] = {NULL};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    mln_test_wayland_start(&wayland);
    surfaces[0] = mln_test_wayland_create_surface(wayland);
    attach(map, wayland, surfaces[0], &fixture);
    render_frame(&fixture);
    expect_red_frame(wayland, surfaces[0], window_size, window_size);

    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_resize(
        fixture.session, (mln_logical_size){.width = 96, .height = 48},
        &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
    render_frame(&fixture);
    expect_red_frame(wayland, surfaces[0], 96, 48);
  }
  release_session(&fixture);
  stop_wayland(wayland, surfaces, 1);
  destroy_map_and_runtime(map, runtime);
}

// A session keeps its graphics context without a surface, and presents to the
// next one, whether it gets that after a release or in place of a surface.
static void a_session_presents_to_each_surface_that_it_gets(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_wayland* wayland = NULL;
  void* surfaces[2] = {NULL, NULL};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    mln_test_wayland_start(&wayland);
    surfaces[0] = mln_test_wayland_create_surface(wayland);
    surfaces[1] = mln_test_wayland_create_surface(wayland);
    attach(map, wayland, surfaces[0], &fixture);
    render_frame(&fixture);
    expect_red_frame(wayland, surfaces[0], window_size, window_size);

    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_release_target(
        fixture.session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
    set_surface(&fixture, surfaces[1]);
    render_frame(&fixture);
    expect_red_frame(wayland, surfaces[1], window_size, window_size);

    set_surface(&fixture, surfaces[0]);
    render_frame(&fixture);
    expect_red_frame(wayland, surfaces[0], window_size, window_size);
  }
  release_session(&fixture);
  stop_wayland(wayland, surfaces, 2);
  destroy_map_and_runtime(map, runtime);
}

// A compositor answers a hidden surface's frame callbacks only once it shows
// again. The host paces the frames, so a present must not wait for them, or a
// detach queued behind it would wait too.
static void a_session_presents_to_a_hidden_surface_and_detaches(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  mln_test_wayland* wayland = NULL;
  void* surfaces[1] = {NULL};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    mln_test_wayland_start(&wayland);
    surfaces[0] = mln_test_wayland_create_surface(wayland);
    mln_test_wayland_hide_surface(wayland, surfaces[0]);
    attach(map, wayland, surfaces[0], &fixture);
    for (int frame = 0; frame < 4; ++frame) {
      render_frame(&fixture);
      expect_red_frame(wayland, surfaces[0], window_size, window_size);
    }
    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_detach(
        fixture.session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
  }
  release_session(&fixture);
  stop_wayland(wayland, surfaces, 1);
  destroy_map_and_runtime(map, runtime);
}

// The library owns the graphics context, so it drives the session itself.
static void a_wayland_surface_rejects_the_callers_graphics_thread(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  mln_test_wayland* wayland = NULL;
  void* surfaces[1] = {NULL};
  if (TEST_PROTECT()) {
    mln_test_wayland_start(&wayland);
    surfaces[0] = mln_test_wayland_create_surface(wayland);
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      start_attach(
        map, wayland, surfaces[0], MLN_HANDLE_NULL,
        MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, &completion, &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  stop_wayland(wayland, surfaces, 1);
  destroy_map_and_runtime(map, runtime);
}

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
static void a_device_without_presentation_cannot_present(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  mln_test_wayland* wayland = NULL;
  void* surfaces[1] = {NULL};
  mln_vulkan_device device = MLN_HANDLE_NULL;
  if (TEST_PROTECT()) {
    mln_test_wayland_start(&wayland);
    surfaces[0] = mln_test_wayland_create_surface(wayland);
    device = create_device(false, NULL);
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      start_attach(
        map, wayland, surfaces[0], device, MLN_RENDER_DRIVER_CORE_WORKER,
        &completion, &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  mln_vulkan_device_release(device);
  stop_wayland(wayland, surfaces, 1);
  destroy_map_and_runtime(map, runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(a_session_presents_to_a_surface_at_its_size);
  RUN_TEST(a_session_presents_to_each_surface_that_it_gets);
  RUN_TEST(a_session_presents_to_a_hidden_surface_and_detaches);
  RUN_TEST(a_wayland_surface_rejects_the_callers_graphics_thread);
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  RUN_TEST(a_device_without_presentation_cannot_present);
#endif
}
