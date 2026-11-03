// Offscreen devices that native code creates, and the readback-only offscreen
// sessions that render on them. Metal, Vulkan, EGL, and WGL builds create a
// device; browser builds report that they cannot, after they validate the
// call.

#include "support/frames.h"
#include "support/map.h"
#include "support/test_support.h"

#if defined(MLN_FFI_TEST_BACKEND_METAL) ||                                    \
  defined(MLN_FFI_TEST_BACKEND_VULKAN) || defined(MLN_FFI_TEST_OPENGL_EGL) || \
  defined(MLN_FFI_TEST_OPENGL_WGL)
#define DEVICE_SUPPORTED 1
#endif

static void create_rejects_malformed_arguments(void) {
  mln_offscreen_device device = MLN_HANDLE_NULL;
  mln_offscreen_device_options options = {
    .size = sizeof(mln_offscreen_device_options) - 1
  };
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_offscreen_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  options.size = sizeof(mln_offscreen_device_options);
  options.fields = 1U << 31U;
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_offscreen_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_offscreen_device_create(NULL, NULL, MLN_TEST_DIAGNOSTIC)
  );
  device = UINT64_C(1);
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_offscreen_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC)
  );
  mln_offscreen_device_release(MLN_HANDLE_NULL);
}

static mln_offscreen_descriptor texture_descriptor(void) {
  mln_offscreen_descriptor descriptor = {
    .size = sizeof(mln_offscreen_descriptor)
  };
  descriptor.extent =
    (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 1.0};
  return descriptor;
}

static mln_render_session_attach_options core_worker_options(void) {
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  return options;
}

static void expect_attach_status(
  mln_status expected, mln_map map, mln_offscreen_device device,
  const mln_offscreen_descriptor* descriptor,
  const mln_render_session_attach_options* options
) {
  mln_render_session session = MLN_HANDLE_NULL;
  mln_test_completion attach = mln_test_completion_default(0);
  MLN_TEST_STATUS(
    expected, mln_map_attach_offscreen(
                map, descriptor, options, device, &session, &attach.descriptor,
                MLN_TEST_DIAGNOSTIC
              )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_completion_reject(&attach);
  mln_test_completion_destroy(&attach);
}

#if !defined(DEVICE_SUPPORTED)
static void create_reports_an_unsupported_backend(void) {
  mln_offscreen_device device = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_offscreen_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
}

// Other builds create the device on the session's core worker, but a browser
// build can never create one, so the call itself rejects the attach.
static void attach_without_a_device_reports_an_unsupported_backend(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_offscreen_descriptor descriptor = texture_descriptor();
  const mln_render_session_attach_options options = core_worker_options();
  expect_attach_status(
    MLN_STATUS_UNSUPPORTED, map, MLN_HANDLE_NULL, &descriptor, &options
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

#if defined(DEVICE_SUPPORTED)
static mln_offscreen_device create_device(void) {
  mln_offscreen_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_offscreen_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
  return device;
}

// Attaches an offscreen session on `device` and waits until the driver owns it.
static mln_test_render_fixture attach_offscreen_session(
  mln_map map, mln_offscreen_device device
) {
  mln_test_render_fixture fixture = {.driver = MLN_RENDER_DRIVER_CORE_WORKER};
  const mln_offscreen_descriptor descriptor = texture_descriptor();
  const mln_render_session_attach_options options = core_worker_options();
  mln_test_completion attach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_attach_offscreen(
    map, &descriptor, &options, device, &fixture.session, &attach.descriptor,
    MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(&fixture, &attach));
  mln_test_completion_destroy(&attach);
  return fixture;
}

static void detach_offscreen_session(mln_test_render_fixture* fixture) {
  mln_test_completion detach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_render_session_detach(
    fixture->session, &detach.descriptor, MLN_TEST_DIAGNOSTIC
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &detach));
  mln_test_completion_destroy(&detach);
  MLN_TEST_OK(mln_test_render_session_dispose(fixture->session));
  *fixture = (mln_test_render_fixture){0};
}

static void expect_red_pixels(const mln_test_render_fixture* fixture) {
  enum { pixel_bytes = 64 * 64 * 4 };
  static uint8_t pixels[pixel_bytes];
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(fixture, &info, pixels, pixel_bytes));
  TEST_ASSERT_EQUAL_UINT32(64, info.width);
  TEST_ASSERT_EQUAL_UINT32(64, info.height);
  TEST_ASSERT_EQUAL_size_t(pixel_bytes, info.byte_length);
  TEST_ASSERT_EQUAL_UINT8(255, pixels[0]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels[1]);
  TEST_ASSERT_EQUAL_UINT8(0, pixels[2]);
  TEST_ASSERT_EQUAL_UINT8(255, pixels[3]);
}

static void render_red_frame(
  mln_map map, const mln_test_render_fixture* fixture, uint64_t token
) {
  MLN_TEST_OK(
    mln_test_map_set_style_json(map, mln_test_red_background_style_json)
  );
  MLN_TEST_OK(mln_test_map_fence(map));
  mln_test_render_request_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(token, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
}

static void a_session_reads_back_after_its_device_is_released(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_offscreen_device device = create_device();
  mln_test_render_fixture fixture = attach_offscreen_session(map, device);
  mln_offscreen_device_release(device);

  mln_render_session_capabilities capabilities = {
    .size = sizeof(mln_render_session_capabilities)
  };
  MLN_TEST_OK(
    mln_render_session_get_capabilities(fixture.session, &capabilities, NULL)
  );
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_DRIVER_CORE_WORKER, capabilities.driver);
  TEST_ASSERT_EQUAL_UINT32(1, capabilities.texture_ring_depth);
  TEST_ASSERT_EQUAL_UINT32(
    MLN_RENDER_SESSION_CAPABILITY_READBACK, capabilities.flags
  );
  mln_acquired_frame frame = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_render_session_acquire_frame(fixture.session, &frame, NULL)
  );

  render_red_frame(map, &fixture, 7);
  expect_red_pixels(&fixture);

  detach_offscreen_session(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// The session creates its device on its core worker, before the attach
// completes.
static void a_session_without_a_device_renders_a_still_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options map_options = mln_test_map_options();
  map_options.initial_extent = texture_descriptor().extent;
  map_options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &map_options);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  mln_test_render_fixture fixture =
    attach_offscreen_session(map, MLN_HANDLE_NULL);

  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  expect_red_pixels(&fixture);

  detach_offscreen_session(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void sessions_on_two_maps_take_turns_on_one_device(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map first_map = mln_test_create_map(runtime);
  mln_map second_map = mln_test_create_map(runtime);
  mln_offscreen_device device = create_device();
  mln_test_render_fixture first = attach_offscreen_session(first_map, device);
  render_red_frame(first_map, &first, 21);
  expect_red_pixels(&first);
  detach_offscreen_session(&first);
  mln_test_render_fixture second = attach_offscreen_session(second_map, device);
  render_red_frame(second_map, &second, 22);
  expect_red_pixels(&second);
  detach_offscreen_session(&second);
  mln_offscreen_device_release(device);
  mln_test_destroy_map(second_map);
  mln_test_destroy_map(first_map);
  mln_test_destroy_runtime(runtime);
}

// Each session keeps its context current on its own core worker, so both stay
// attached while the other renders and reads back.
static void sessions_on_two_maps_share_one_device(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map first_map = mln_test_create_map(runtime);
  mln_map second_map = mln_test_create_map(runtime);
  mln_offscreen_device device = create_device();
  mln_test_render_fixture first = attach_offscreen_session(first_map, device);
  mln_test_render_fixture second = attach_offscreen_session(second_map, device);

  render_red_frame(first_map, &first, 11);
  render_red_frame(second_map, &second, 12);
  expect_red_pixels(&first);
  expect_red_pixels(&second);

  detach_offscreen_session(&first);
  detach_offscreen_session(&second);
  mln_offscreen_device_release(device);
  mln_test_destroy_map(second_map);
  mln_test_destroy_map(first_map);
  mln_test_destroy_runtime(runtime);
}

static void queue_lock_callback(void* user_data) { (void)user_data; }

static void attach_rejects_what_an_offscreen_session_cannot_take(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_offscreen_device device = create_device();
  const mln_offscreen_descriptor descriptor = texture_descriptor();
  const mln_render_session_attach_options options = core_worker_options();

  mln_offscreen_descriptor undersized = descriptor;
  undersized.size -= 1;
  expect_attach_status(
    MLN_STATUS_INVALID_ARGUMENT, map, device, &undersized, &options
  );
  mln_offscreen_descriptor empty = descriptor;
  empty.extent.width = 0;
  expect_attach_status(
    MLN_STATUS_INVALID_ARGUMENT, map, device, &empty, &options
  );
  mln_render_session_attach_options caller_thread = options;
  caller_thread.driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  expect_attach_status(
    MLN_STATUS_UNSUPPORTED, map, device, &descriptor, &caller_thread
  );
  mln_render_session_attach_options locked = options;
  locked.queue_lock.lock = queue_lock_callback;
  locked.queue_lock.unlock = queue_lock_callback;
  expect_attach_status(
    MLN_STATUS_UNSUPPORTED, map, device, &descriptor, &locked
  );

  mln_offscreen_device_release(device);
  expect_attach_status(
    MLN_STATUS_INVALID_STATE, map, device, &descriptor, &options
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void the_first_named_adapter_renders(void) {
  mln_offscreen_device_options options = {
    .size = sizeof(mln_offscreen_device_options)
  };
  options.fields = MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX;
  options.adapter_index = 0;
  mln_offscreen_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(
    mln_offscreen_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = attach_offscreen_session(map, device);

  render_red_frame(map, &fixture, 13);
  expect_red_pixels(&fixture);

  detach_offscreen_session(&fixture);
  mln_offscreen_device_release(device);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void create_reports_a_missing_adapter(void) {
  mln_offscreen_device_options options = {
    .size = sizeof(mln_offscreen_device_options)
  };
  options.fields = MLN_OFFSCREEN_DEVICE_OPTION_ADAPTER_INDEX;
  options.adapter_index = UINT32_MAX;
  mln_offscreen_device device = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR,
    mln_offscreen_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(create_rejects_malformed_arguments);
#if defined(DEVICE_SUPPORTED)
  RUN_TEST(a_session_reads_back_after_its_device_is_released);
  RUN_TEST(a_session_without_a_device_renders_a_still_image);
  RUN_TEST(sessions_on_two_maps_take_turns_on_one_device);
  RUN_TEST(sessions_on_two_maps_share_one_device);
  RUN_TEST(attach_rejects_what_an_offscreen_session_cannot_take);
  RUN_TEST(the_first_named_adapter_renders);
  RUN_TEST(create_reports_a_missing_adapter);
#else
  RUN_TEST(create_reports_an_unsupported_backend);
  RUN_TEST(attach_without_a_device_reports_an_unsupported_backend);
#endif
}
