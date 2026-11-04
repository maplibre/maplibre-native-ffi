// Offscreen devices that native code creates, and the readback-only offscreen
// sessions that render on them. A browser WebGPU build creates a device only
// for a session that attaches without one, and reports that it cannot create
// one for the caller, after it validates the call.

#include <stdatomic.h>
#include <string.h>

#include "support/frames.h"
#include "support/map.h"
#include "support/test_support.h"

#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
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
  options.fields = MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API;
  options.client_api = MLN_OPEN_GL_CLIENT_API_UNSPECIFIED;
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
  descriptor.logical_size = (mln_logical_size){.width = 64, .height = 64};
  return descriptor;
}

static mln_render_session_attach_options core_worker_options(void) {
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  return options;
}

// Only an EGL device renders with either OpenGL API. A WGL device renders
// with desktop OpenGL alone, a WebGL device with OpenGL ES alone, and other
// devices with no OpenGL API.
#if !defined(MLN_FFI_TEST_OPENGL_EGL)
static void create_reports_an_unusable_client_api(void) {
  mln_offscreen_device_options options = {
    .size = sizeof(mln_offscreen_device_options)
  };
  options.fields = MLN_OFFSCREEN_DEVICE_OPTION_CLIENT_API;
#if defined(MLN_FFI_TEST_OPENGL_WGL)
  options.client_api = MLN_OPEN_GL_CLIENT_API_GLES;
#else
  options.client_api = MLN_OPEN_GL_CLIENT_API_GL;
#endif
  mln_offscreen_device device = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_offscreen_device_create(&options, &device, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
}
#endif

#if !defined(DEVICE_SUPPORTED)
static void create_reports_an_unsupported_backend(void) {
  mln_offscreen_device device = MLN_HANDLE_NULL;
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED,
    mln_offscreen_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
}
#endif

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
  MLN_TEST_OK(mln_test_render_session_release(fixture->session));
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
  mln_test_render_demand_forced(fixture, token);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  TEST_ASSERT_EQUAL_UINT64(token, result.token);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  mln_render_frame_batch_release(batch);
}

static mln_map create_static_red_map(mln_runtime runtime) {
  mln_map_options map_options = mln_test_map_options();
  map_options.initial_extent.width = texture_descriptor().logical_size.width;
  map_options.initial_extent.height = texture_descriptor().logical_size.height;
  map_options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &map_options);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  return map;
}

// The session creates its device on its core worker, before the attach
// completes.
static void a_session_without_a_device_renders_a_still_image(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_static_red_map(runtime);
  mln_test_render_fixture fixture =
    attach_offscreen_session(map, MLN_HANDLE_NULL);

  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  expect_red_pixels(&fixture);

  detach_offscreen_session(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A WebGPU frame's GPU work can still run when the next demand reaches the
// driver, which renders the next frame only once that work ends.
static void a_session_without_a_device_renders_frames_in_turn(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture =
    attach_offscreen_session(map, MLN_HANDLE_NULL);

  for (uint64_t token = 1; token <= 4; token += 1) {
    render_red_frame(map, &fixture, token);
    expect_red_pixels(&fixture);
  }

  detach_offscreen_session(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_session_without_a_device_renders_still_images_in_turn(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_static_red_map(runtime);
  mln_test_render_fixture fixture =
    attach_offscreen_session(map, MLN_HANDLE_NULL);

  for (int image = 0; image < 3; image += 1) {
    MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
    expect_red_pixels(&fixture);
  }

  detach_offscreen_session(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void a_session_without_a_device_renders_a_demand_after_a_still_image(
  void
) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_static_red_map(runtime);
  mln_test_render_fixture fixture =
    attach_offscreen_session(map, MLN_HANDLE_NULL);
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  expect_red_pixels(&fixture);

  render_red_frame(map, &fixture, 1);
  expect_red_pixels(&fixture);

  detach_offscreen_session(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// Disposal detaches the session on its core worker, which then exits with the
// device that the session created gone.
static void disposing_a_session_without_a_device_ends_it(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = create_static_red_map(runtime);
  mln_test_render_fixture fixture =
    attach_offscreen_session(map, MLN_HANDLE_NULL);
  MLN_TEST_OK(mln_test_render_still_image(&fixture, map));
  expect_red_pixels(&fixture);

  MLN_TEST_OK(mln_test_render_session_release(fixture.session));
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// One completion's delivery, numbered among the case's deliveries in the order
// the session made them, with the center pixel of a readback.
typedef struct ordered_delivery {
  atomic_int* sequence;
  int position;
  mln_status status;
  uint8_t center[4];
  atomic_bool delivered;
} ordered_delivery;

static void record_delivery(
  void* user_data, const mln_completion_result* result
) {
  ordered_delivery* delivery = user_data;
  delivery->position = atomic_fetch_add(delivery->sequence, 1);
  delivery->status = (mln_status)result->status;
  const mln_texture_readback_result* readback = result->value;
  if (result->status == MLN_STATUS_OK && readback != NULL) {
    const size_t center = (size_t)32 * readback->info.stride + (size_t)32 * 4;
    if (readback->data.size >= center + 4) {
      memcpy(delivery->center, (const uint8_t*)readback->data.data + center, 4);
    }
  }
  mln_test_flag_set(&delivery->delivered);
}

static mln_completion ordered_completion(ordered_delivery* delivery) {
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = record_delivery,
    .user_data = delivery,
  };
}

// A WebGPU readback finishes only once its buffer map does, so the work
// submitted behind it waits for it.
static void work_behind_a_readback_runs_after_it(void) {
  static const uint8_t red[4] = {255, 0, 0, 255};
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture =
    attach_offscreen_session(map, MLN_HANDLE_NULL);
  render_red_frame(map, &fixture, 1);

  atomic_int sequence = 0;
  ordered_delivery first = {.sequence = &sequence};
  ordered_delivery second = {.sequence = &sequence};
  ordered_delivery fence = {.sequence = &sequence};
  const mln_completion first_completion = ordered_completion(&first);
  const mln_completion second_completion = ordered_completion(&second);
  const mln_completion fence_completion = ordered_completion(&fence);
  MLN_TEST_OK(
    mln_render_session_read_texture(fixture.session, &first_completion, NULL)
  );
  MLN_TEST_OK(
    mln_render_session_read_texture(fixture.session, &second_completion, NULL)
  );
  MLN_TEST_OK(
    mln_render_session_dump_debug_logs(fixture.session, &fence_completion, NULL)
  );
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&fence.delivered));

  TEST_ASSERT_TRUE(atomic_load(&first.delivered));
  TEST_ASSERT_TRUE(atomic_load(&second.delivered));
  MLN_TEST_OK(first.status);
  MLN_TEST_OK(second.status);
  MLN_TEST_OK(fence.status);
  TEST_ASSERT_EQUAL_INT(0, first.position);
  TEST_ASSERT_EQUAL_INT(1, second.position);
  TEST_ASSERT_EQUAL_INT(2, fence.position);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(red, first.center, 4);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(red, second.center, 4);

  detach_offscreen_session(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if defined(DEVICE_SUPPORTED)
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

static mln_offscreen_device create_device(void) {
  mln_offscreen_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_offscreen_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
  return device;
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
  empty.logical_size.width = 0;
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
#if !defined(MLN_FFI_TEST_OPENGL_EGL)
  RUN_TEST(create_reports_an_unusable_client_api);
#endif
  RUN_TEST(a_session_without_a_device_renders_a_still_image);
  RUN_TEST(a_session_without_a_device_renders_frames_in_turn);
  RUN_TEST(a_session_without_a_device_renders_still_images_in_turn);
  RUN_TEST(a_session_without_a_device_renders_a_demand_after_a_still_image);
  RUN_TEST(disposing_a_session_without_a_device_ends_it);
  RUN_TEST(work_behind_a_readback_runs_after_it);
#if defined(DEVICE_SUPPORTED)
  RUN_TEST(a_session_reads_back_after_its_device_is_released);
  RUN_TEST(sessions_on_two_maps_take_turns_on_one_device);
  RUN_TEST(sessions_on_two_maps_share_one_device);
  RUN_TEST(attach_rejects_what_an_offscreen_session_cannot_take);
  RUN_TEST(the_first_named_adapter_renders);
  RUN_TEST(create_reports_a_missing_adapter);
#else
  RUN_TEST(create_reports_an_unsupported_backend);
#endif
}
