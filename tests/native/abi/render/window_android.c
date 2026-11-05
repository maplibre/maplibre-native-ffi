// Sessions that present to an Android window with a graphics context that the
// library creates. An AImageReader's window stands in for an app's, so a case
// reads back what the session presents. Each case deletes its readers even
// after a failed assertion, once its session no longer uses their windows.

#include <android/native_window.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>

#include "support/frames.h"
#include "support/test_support.h"

enum { window_size = 64 };

typedef struct window_reader {
  AImageReader* reader;
  // The reader owns its window.
  ANativeWindow* window;
  mln_logical_size window_size;
  atomic_bool image_available;
} window_reader;

static void on_image_available(void* context, AImageReader* reader) {
  (void)reader;
  window_reader* state = context;
  mln_test_flag_set(&state->image_available);
}

static void reader_create_sized(window_reader* out, mln_logical_size size) {
  out->window_size = size;
  TEST_ASSERT_EQUAL_INT(
    AMEDIA_OK, AImageReader_new(
                 (int32_t)size.width, (int32_t)size.height,
                 AIMAGE_FORMAT_RGBA_8888, 2, &out->reader
               )
  );
  AImageReader_ImageListener listener = {
    .context = out, .onImageAvailable = on_image_available
  };
  TEST_ASSERT_EQUAL_INT(
    AMEDIA_OK, AImageReader_setImageListener(out->reader, &listener)
  );
  TEST_ASSERT_EQUAL_INT(
    AMEDIA_OK, AImageReader_getWindow(out->reader, &out->window)
  );
}

static void reader_create(window_reader* out) {
  reader_create_sized(
    out, (mln_logical_size){.width = window_size, .height = window_size}
  );
}

static void reader_destroy(window_reader* reader) {
  if (reader->reader != NULL) AImageReader_delete(reader->reader);
  reader->reader = NULL;
  reader->window = NULL;
}

// Waits for the window's next frame, and expects the style's red at its center.
static void expect_red_frame(window_reader* reader) {
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&reader->image_available));
  atomic_store(&reader->image_available, false);
  AImage* image = NULL;
  TEST_ASSERT_EQUAL_INT(
    AMEDIA_OK, AImageReader_acquireLatestImage(reader->reader, &image)
  );
  uint8_t* data = NULL;
  int length = 0;
  int32_t row_stride = 0;
  int32_t pixel_stride = 0;
  media_status_t status = AImage_getPlaneData(image, 0, &data, &length);
  if (status == AMEDIA_OK) {
    status = AImage_getPlaneRowStride(image, 0, &row_stride);
  }
  if (status == AMEDIA_OK) {
    status = AImage_getPlanePixelStride(image, 0, &pixel_stride);
  }
  uint8_t pixel[4] = {0};
  const int offset = (int)(reader->window_size.height / 2) * row_stride +
                     (int)(reader->window_size.width / 2) * pixel_stride;
  if (status == AMEDIA_OK && offset + 4 <= length) {
    memcpy(pixel, data + offset, sizeof(pixel));
  }
  AImage_delete(image);
  TEST_ASSERT_EQUAL_INT(AMEDIA_OK, status);
  TEST_ASSERT_EQUAL_HEX8(0xff, pixel[0]);
  TEST_ASSERT_EQUAL_HEX8(0x00, pixel[1]);
  TEST_ASSERT_EQUAL_HEX8(0x00, pixel[2]);
  TEST_ASSERT_EQUAL_HEX8(0xff, pixel[3]);
}

// Submits an attach to `window` without waiting for it. The fixture holds the
// session once the call succeeds.
static mln_status start_attach(
  mln_map map, ANativeWindow* window, mln_vulkan_device device,
  mln_render_driver_kind driver, mln_test_completion* completion,
  mln_test_render_fixture* fixture
) {
  const mln_android_surface_descriptor descriptor = {
    .size = sizeof(mln_android_surface_descriptor),
    .logical_size = {.width = window_size, .height = window_size},
    .window = window,
  };
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options), .driver = driver
  };
  options.frame_wake = mln_test_pulse_wake();
  fixture->driver = driver;
  const mln_status status = mln_map_attach_android_surface(
    map, &descriptor, device, &options, &fixture->session,
    &completion->descriptor, MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) mln_test_completion_reject(completion);
  return status;
}

static void finish_attach(
  const mln_test_render_fixture* fixture, mln_test_completion* completion
) {
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, completion));
  mln_test_completion_destroy(completion);
}

static void attach(
  mln_map map, ANativeWindow* window, mln_test_render_fixture* fixture
) {
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(start_attach(
    map, window, MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CORE_WORKER, &completion,
    fixture
  ));
  finish_attach(fixture, &completion);
}

static mln_render_frame_result demand_frame(
  const mln_test_render_fixture* fixture
) {
  mln_test_render_demand_forced(fixture, 0);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  return result;
}

static mln_render_frame_result render_frame(
  const mln_test_render_fixture* fixture
) {
  const mln_render_frame_result result = demand_frame(fixture);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  return result;
}

static void set_window(
  const mln_test_render_fixture* fixture, const window_reader* reader
) {
  const mln_android_surface_replacement replacement = {
    .size = sizeof(mln_android_surface_replacement),
    .logical_size = reader->window_size,
    .window = reader->window,
  };
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_set_android_surface_target(
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
  window_reader reader = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    reader_create(&reader);
    ANativeWindow_acquire(reader.window);
    mln_test_completion completion = mln_test_completion_default(0);
    const mln_status attached = start_attach(
      map, reader.window, MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CORE_WORKER,
      &completion, &fixture
    );
    ANativeWindow_release(reader.window);
    MLN_TEST_OK(attached);
    finish_attach(&fixture, &completion);

    render_frame(&fixture);
    expect_red_frame(&reader);
  }
  release_session(&fixture);
  reader_destroy(&reader);
  destroy_map_and_runtime(map, runtime);
}

// A session keeps its graphics context without a window, and presents to the
// next one, whether it gets that after a release or in place of a window.
static void a_session_presents_to_each_window_that_it_gets(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  window_reader first = {0};
  window_reader second = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    reader_create(&first);
    reader_create(&second);
    attach(map, first.window, &fixture);
    render_frame(&fixture);
    expect_red_frame(&first);

    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_release_target(
        fixture.session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
    set_window(&fixture, &second);
    render_frame(&fixture);
    expect_red_frame(&second);

    set_window(&fixture, &first);
    render_frame(&fixture);
    expect_red_frame(&first);
  }
  release_session(&fixture);
  reader_destroy(&second);
  reader_destroy(&first);
  destroy_map_and_runtime(map, runtime);
}

// A host that can't hold the reader's teardown for the release loses the
// window first, and a resize queued before the release, as for a rotation,
// reaches the lost window. The session takes the size, each frame to the lost
// window ends not ready, and the host can release the window and attach the
// next one at the new size.
static void a_session_survives_a_window_whose_reader_went_away(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  const mln_logical_size rotated = {.width = 96, .height = 48};
  window_reader first = {0};
  window_reader second = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    reader_create(&first);
    attach(map, first.window, &fixture);
    render_frame(&fixture);
    expect_red_frame(&first);

    reader_destroy(&first);
    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_resize(
        fixture.session, rotated, &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
    for (int frame = 0; frame < 2; frame += 1) {
      TEST_ASSERT_EQUAL_UINT32(
        MLN_RENDER_RESULT_TARGET_NOT_READY, demand_frame(&fixture).disposition
      );
    }
    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_release_target(
        fixture.session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
    reader_create_sized(&second, rotated);
    set_window(&fixture, &second);
    const mln_render_frame_result rendered = render_frame(&fixture);
    TEST_ASSERT_EQUAL_UINT32(rotated.width, rendered.view.extent.width);
    TEST_ASSERT_EQUAL_UINT32(rotated.height, rendered.view.extent.height);
    expect_red_frame(&second);
  }
  release_session(&fixture);
  reader_destroy(&second);
  reader_destroy(&first);
  destroy_map_and_runtime(map, runtime);
}

// The library owns the graphics context, so it drives the session itself.
static void an_android_surface_rejects_the_callers_graphics_thread(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  window_reader reader = {0};
  if (TEST_PROTECT()) {
    reader_create(&reader);
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      start_attach(
        map, reader.window, MLN_HANDLE_NULL,
        MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, &completion, &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  reader_destroy(&reader);
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

// The session holds the device, so the host can release it once the attach
// call returns.
static void a_session_presents_with_the_hosts_device(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  window_reader reader = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    reader_create(&reader);
    const mln_vulkan_device device = create_device(true);
    mln_test_completion completion = mln_test_completion_default(0);
    const mln_status attached = start_attach(
      map, reader.window, device, MLN_RENDER_DRIVER_CORE_WORKER, &completion,
      &fixture
    );
    mln_vulkan_device_release(device);
    MLN_TEST_OK(attached);
    finish_attach(&fixture, &completion);

    render_frame(&fixture);
    expect_red_frame(&reader);
  }
  release_session(&fixture);
  reader_destroy(&reader);
  destroy_map_and_runtime(map, runtime);
}

static void a_device_without_presentation_cannot_present(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  window_reader reader = {0};
  mln_vulkan_device device = MLN_HANDLE_NULL;
  if (TEST_PROTECT()) {
    reader_create(&reader);
    device = create_device(false);
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      start_attach(
        map, reader.window, device, MLN_RENDER_DRIVER_CORE_WORKER, &completion,
        &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  mln_vulkan_device_release(device);
  reader_destroy(&reader);
  destroy_map_and_runtime(map, runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(a_session_presents_to_a_window_that_the_host_let_go_of);
  RUN_TEST(a_session_presents_to_each_window_that_it_gets);
  RUN_TEST(a_session_survives_a_window_whose_reader_went_away);
  RUN_TEST(an_android_surface_rejects_the_callers_graphics_thread);
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  RUN_TEST(a_session_presents_with_the_hosts_device);
  RUN_TEST(a_device_without_presentation_cannot_present);
#endif
}
