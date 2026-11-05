// Sessions that present to an OpenHarmony window with a graphics context that
// the library creates. A consumer surface's window stands in for an app's, so a
// case reads back what the session presents. Each case destroys its consumers
// even after a failed assertion, once its session no longer uses their windows.

// clang-format off
// The SDK's poll.h uses size_t without declaring it.
#include <stddef.h>
// clang-format on

#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <poll.h>
#include <unistd.h>

#include "support/frames.h"
#include "support/test_support.h"

enum { window_size = 64 };

typedef struct window_consumer {
  OH_NativeImage* image;
  // The consumer owns its window.
  OHNativeWindow* window;
  atomic_bool frame_available;
} window_consumer;

static void on_frame_available(void* context) {
  window_consumer* state = context;
  mln_test_flag_set(&state->frame_available);
}

static void consumer_create(window_consumer* out) {
  out->image = OH_ConsumerSurface_Create();
  TEST_ASSERT_NOT_NULL(out->image);
  TEST_ASSERT_EQUAL_INT(
    0, OH_ConsumerSurface_SetDefaultSize(out->image, window_size, window_size)
  );
  TEST_ASSERT_EQUAL_INT(
    0, OH_ConsumerSurface_SetDefaultUsage(
         out->image, NATIVEBUFFER_USAGE_CPU_READ | NATIVEBUFFER_USAGE_HW_RENDER
       )
  );
  const OH_OnFrameAvailableListener listener = {
    .context = out, .onFrameAvailable = on_frame_available
  };
  TEST_ASSERT_EQUAL_INT(
    0, OH_NativeImage_SetOnFrameAvailableListener(out->image, listener)
  );
  out->window = OH_NativeImage_AcquireNativeWindow(out->image);
  TEST_ASSERT_NOT_NULL(out->window);
}

static void consumer_destroy(window_consumer* consumer) {
  if (consumer->image != NULL) OH_NativeImage_Destroy(&consumer->image);
  consumer->image = NULL;
  consumer->window = NULL;
}

// Submits an attach to `window`, or to the surface `surface_id` names, without
// waiting for it. The fixture holds the session once the call succeeds.
static mln_status start_attach(
  mln_map map, OHNativeWindow* window, uint64_t surface_id,
  mln_vulkan_device device, mln_render_driver_kind driver,
  mln_test_completion* completion, mln_test_render_fixture* fixture
) {
  const mln_ohos_surface_descriptor descriptor = {
    .size = sizeof(mln_ohos_surface_descriptor),
    .logical_size = {.width = window_size, .height = window_size},
    .window = window,
    .surface_id = surface_id,
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options), .driver = driver
  };
  options.frame_wake = mln_test_pulse_wake();
  fixture->driver = driver;
  const mln_status status = mln_map_attach_ohos_surface(
    map, &descriptor, device, &options, &fixture->session,
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

// Presenting through Vulkan needs VK_OHOS_surface, and no Vulkan driver that
// the OpenHarmony emulator can load offers it, so the cases that present run on
// OpenGL builds. The Android window cases cover the presentation code that both
// platforms share.
#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
static uint64_t consumer_surface_id(const window_consumer* consumer) {
  uint64_t surface_id = 0;
  TEST_ASSERT_EQUAL_INT(
    0, OH_NativeImage_GetSurfaceId(consumer->image, &surface_id)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(0, surface_id);
  return surface_id;
}

// Waits for the window's next frame, and expects the style's red at its center.
static void expect_red_frame(window_consumer* consumer) {
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&consumer->frame_available));
  atomic_store(&consumer->frame_available, false);
  OHNativeWindowBuffer* buffer = NULL;
  int fence = -1;
  TEST_ASSERT_EQUAL_INT(
    0,
    OH_NativeImage_AcquireNativeWindowBuffer(consumer->image, &buffer, &fence)
  );
  if (fence >= 0) {
    struct pollfd wait = {.fd = fence, .events = POLLIN};
    (void)poll(&wait, 1, -1);
    close(fence);
  }
  OH_NativeBuffer* native = NULL;
  int32_t status = OH_NativeBuffer_FromNativeWindowBuffer(buffer, &native);
  void* data = NULL;
  if (status == 0) status = OH_NativeBuffer_Map(native, &data);
  uint8_t pixel[4] = {0};
  if (status == 0) {
    OH_NativeBuffer_Config config = {0};
    OH_NativeBuffer_GetConfig(native, &config);
    const size_t offset =
      (size_t)(window_size / 2) * (size_t)config.stride + (window_size / 2) * 4;
    memcpy(pixel, (const uint8_t*)data + offset, sizeof(pixel));
    (void)OH_NativeBuffer_Unmap(native);
  }
  (void)OH_NativeImage_ReleaseNativeWindowBuffer(consumer->image, buffer, -1);
  TEST_ASSERT_EQUAL_INT(0, status);
  TEST_ASSERT_EQUAL_HEX8(0xff, pixel[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, pixel[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, pixel[2]);
  TEST_ASSERT_EQUAL_HEX8(0xff, pixel[3]);
}

static void finish_attach(
  const mln_test_render_fixture* fixture, mln_test_completion* completion
) {
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, completion));
  mln_test_completion_destroy(completion);
}

static void attach(
  mln_map map, OHNativeWindow* window, mln_test_render_fixture* fixture
) {
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(start_attach(
    map, window, 0, MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CORE_WORKER, &completion,
    fixture
  ));
  finish_attach(fixture, &completion);
}

static mln_render_result demand_frame(const mln_test_render_fixture* fixture) {
  mln_test_render_demand_forced(fixture, 0);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  return result.disposition;
}

static void render_frame(const mln_test_render_fixture* fixture) {
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, demand_frame(fixture));
}

static void set_window(
  const mln_test_render_fixture* fixture, OHNativeWindow* window,
  uint64_t surface_id
) {
  const mln_ohos_surface_replacement replacement = {
    .size = sizeof(mln_ohos_surface_replacement),
    .logical_size = {.width = window_size, .height = window_size},
    .window = window,
    .surface_id = surface_id,
  };
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_set_ohos_surface_target(
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

static mln_map create_red_map(mln_runtime runtime) {
  const mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  return map;
}

// The session keeps a reference of its own, so the host can drop its
// reference as soon as the attach call returns.
static void a_session_presents_to_a_window_that_the_host_let_go_of(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  window_consumer consumer = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    consumer_create(&consumer);
    TEST_ASSERT_EQUAL_INT(
      0, OH_NativeWindow_NativeObjectReference(consumer.window)
    );
    mln_test_completion completion = mln_test_completion_default(0);
    const mln_status attached = start_attach(
      map, consumer.window, 0, MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CORE_WORKER,
      &completion, &fixture
    );
    (void)OH_NativeWindow_NativeObjectUnreference(consumer.window);
    MLN_TEST_OK(attached);
    finish_attach(&fixture, &completion);

    render_frame(&fixture);
    expect_red_frame(&consumer);
  }
  release_session(&fixture);
  consumer_destroy(&consumer);
  destroy_map_and_runtime(map, runtime);
}

// A session keeps its graphics context without a window, and presents to the
// next one, whether it gets that after a release or in place of a window, and
// whether the host names it by pointer or by surface ID.
static void a_session_presents_to_each_window_that_it_gets(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  window_consumer first = {0};
  window_consumer second = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    consumer_create(&first);
    consumer_create(&second);
    attach(map, first.window, &fixture);
    render_frame(&fixture);
    expect_red_frame(&first);

    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_release_target(
        fixture.session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
    set_window(&fixture, NULL, consumer_surface_id(&second));
    render_frame(&fixture);
    expect_red_frame(&second);

    set_window(&fixture, first.window, 0);
    render_frame(&fixture);
    expect_red_frame(&first);
  }
  release_session(&fixture);
  consumer_destroy(&second);
  consumer_destroy(&first);
  destroy_map_and_runtime(map, runtime);
}

// A host that can't hold the consumer's teardown for the release loses the
// window first. Each frame to the lost window ends not ready, and the session
// survives, so the host can release the window and attach the next one.
static void a_session_survives_a_window_whose_consumer_went_away(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  window_consumer first = {0};
  window_consumer second = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    consumer_create(&first);
    attach(map, first.window, &fixture);
    render_frame(&fixture);
    expect_red_frame(&first);

    consumer_destroy(&first);
    for (int frame = 0; frame < 2; frame += 1) {
      TEST_ASSERT_EQUAL_UINT32(
        MLN_RENDER_RESULT_TARGET_NOT_READY, demand_frame(&fixture)
      );
    }
    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_release_target(
        fixture.session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
    consumer_create(&second);
    set_window(&fixture, second.window, 0);
    render_frame(&fixture);
    expect_red_frame(&second);
  }
  release_session(&fixture);
  consumer_destroy(&second);
  consumer_destroy(&first);
  destroy_map_and_runtime(map, runtime);
}

#endif

// The library owns the graphics context, so it drives the session itself.
static void an_ohos_surface_rejects_the_callers_graphics_thread(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  window_consumer consumer = {0};
  if (TEST_PROTECT()) {
    consumer_create(&consumer);
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      start_attach(
        map, consumer.window, 0, MLN_HANDLE_NULL,
        MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, &completion, &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  consumer_destroy(&consumer);
  destroy_map_and_runtime(map, runtime);
}

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
static mln_vulkan_device create_device(bool presentation) {
  mln_vulkan_device_options options = {
    .size = sizeof(mln_vulkan_device_options)
  };
  options.fields = MLN_VULKAN_DEVICE_OPTION_PRESENTATION;
  options.presentation = presentation;
  mln_vulkan_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_vulkan_device_create(&options, &device, MLN_TEST_DIAGNOSTIC));
  return device;
}

static void a_device_without_presentation_cannot_present(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  window_consumer consumer = {0};
  mln_vulkan_device device = MLN_HANDLE_NULL;
  if (TEST_PROTECT()) {
    consumer_create(&consumer);
    device = create_device(false);
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      start_attach(
        map, consumer.window, 0, device, MLN_RENDER_DRIVER_CORE_WORKER,
        &completion, &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  mln_vulkan_device_release(device);
  consumer_destroy(&consumer);
  destroy_map_and_runtime(map, runtime);
}
#endif

MLN_TEST_GROUP {
#if !defined(MLN_FFI_TEST_BACKEND_VULKAN)
  RUN_TEST(a_session_presents_to_a_window_that_the_host_let_go_of);
  RUN_TEST(a_session_presents_to_each_window_that_it_gets);
  RUN_TEST(a_session_survives_a_window_whose_consumer_went_away);
#endif
  RUN_TEST(an_ohos_surface_rejects_the_callers_graphics_thread);
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  RUN_TEST(a_device_without_presentation_cannot_present);
#endif
}
