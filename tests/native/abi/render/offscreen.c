// Readback-only offscreen sessions, on devices that the library creates. A
// browser WebGPU build creates a device only for a session that attaches
// without one.

#include <stdatomic.h>
#include <string.h>

#include "support/frames.h"
#include "support/map.h"
#include "support/test_support.h"

#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
#define DEVICE_SUPPORTED 1
#endif

static mln_render_target offscreen_target(void) {
  mln_render_target target = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_OFFSCREEN,
  };
  target.data.offscreen.logical_size =
    (mln_logical_size){.width = 64, .height = 64};
  return target;
}

static mln_render_session_attach_options core_worker_options(void) {
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  return options;
}

// Attaches an offscreen session on `device` and waits until the driver owns it.
static mln_test_render_fixture attach_offscreen_session(
  mln_map map, mln_gpu_device device
) {
  mln_test_render_fixture fixture = {.driver = MLN_RENDER_DRIVER_CORE_WORKER};
  const mln_render_target target = offscreen_target();
  const mln_render_session_attach_options options = core_worker_options();
  mln_test_completion attach = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_attach(
    map, &target, device, &options, &fixture.session, &attach.descriptor,
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
  map_options.initial_extent.width =
    offscreen_target().data.offscreen.logical_size.width;
  map_options.initial_extent.height =
    offscreen_target().data.offscreen.logical_size.height;
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

// Reads back the session's last frame, and expects red pixels at `width` by
// `height`.
static void expect_red_pixels_at(
  const mln_test_render_fixture* fixture, uint32_t width, uint32_t height
) {
  enum { capacity = 128 * 128 * 4 };
  static uint8_t pixels[capacity];
  mln_texture_image_info info = {0};
  MLN_TEST_OK(mln_test_render_read_back(fixture, &info, pixels, capacity));
  TEST_ASSERT_EQUAL_UINT32(width, info.width);
  TEST_ASSERT_EQUAL_UINT32(height, info.height);
  const size_t center =
    (size_t)(height / 2) * info.stride + (size_t)(width / 2) * 4;
  static const uint8_t red[4] = {255, 0, 0, 255};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(red, &pixels[center], 4);
}

static mln_render_session_snapshot snapshot_of(
  const mln_test_render_fixture* fixture
) {
  mln_render_session_snapshot snapshot = {.size = sizeof(snapshot)};
  MLN_TEST_OK(
    mln_render_session_get_snapshot(fixture->session, &snapshot, NULL)
  );
  return snapshot;
}

// A window that moves to a display with another scale factor keeps its
// session, which renders the next frame at the new scale factor.
static void a_resize_changes_the_scale_factor_and_keeps_the_tiles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture =
    attach_offscreen_session(map, MLN_HANDLE_NULL);
  render_red_frame(map, &fixture, 1);
  expect_red_pixels_at(&fixture, 64, 64);

  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session,
      (mln_logical_extent){.width = 64, .height = 64, .scale_factor = 2.0},
      &completion.descriptor, NULL
    )
  );
  mln_render_session_snapshot snapshot = snapshot_of(&fixture);
  TEST_ASSERT_EQUAL_UINT32(64, snapshot.extent.width);
  TEST_ASSERT_EQUAL_UINT32(64, snapshot.extent.height);
  TEST_ASSERT_EQUAL_DOUBLE(2.0, snapshot.extent.scale_factor);
  mln_test_render_demand_forced(&fixture, 2);
  mln_render_frame_batch batch = mln_test_render_wait_for_results(&fixture);
  mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  TEST_ASSERT_EQUAL_UINT32(MLN_RENDER_RESULT_RENDERED, result.disposition);
  TEST_ASSERT_EQUAL_DOUBLE(2.0, result.view.extent.scale_factor);
  expect_red_pixels_at(&fixture, 128, 128);

  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, &fixture,
    mln_render_session_resize(
      fixture.session,
      (mln_logical_extent){.width = 32, .height = 16, .scale_factor = 1.5},
      &completion.descriptor, NULL
    )
  );
  snapshot = snapshot_of(&fixture);
  TEST_ASSERT_EQUAL_DOUBLE(1.5, snapshot.extent.scale_factor);
  render_red_frame(map, &fixture, 3);
  expect_red_pixels_at(&fixture, 48, 24);

  mln_completion rejected = mln_test_discard_completion();
  MLN_TEST_INVALID(mln_render_session_resize(
    fixture.session,
    (mln_logical_extent){.width = 32, .height = 16, .scale_factor = -1.0},
    &rejected, NULL
  ));

  detach_offscreen_session(&fixture);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if defined(DEVICE_SUPPORTED)
static void expect_attach_status(
  mln_status expected, mln_map map, mln_gpu_device device,
  const mln_render_target* target,
  const mln_render_session_attach_options* options
) {
  mln_render_session session = MLN_HANDLE_NULL;
  mln_test_completion attach = mln_test_completion_default(0);
  MLN_TEST_STATUS(
    expected, mln_map_attach(
                map, target, device, options, &session, &attach.descriptor,
                MLN_TEST_DIAGNOSTIC
              )
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, session);
  mln_test_completion_reject(&attach);
  mln_test_completion_destroy(&attach);
}

static mln_gpu_device create_device(void) {
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_gpu_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, device);
  return device;
}

static void a_session_reads_back_after_its_device_is_released(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_gpu_device device = create_device();
  mln_test_render_fixture fixture = attach_offscreen_session(map, device);
  mln_gpu_device_release(device);

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
  mln_gpu_device device = create_device();
  mln_test_render_fixture first = attach_offscreen_session(first_map, device);
  render_red_frame(first_map, &first, 21);
  expect_red_pixels(&first);
  detach_offscreen_session(&first);
  mln_test_render_fixture second = attach_offscreen_session(second_map, device);
  render_red_frame(second_map, &second, 22);
  expect_red_pixels(&second);
  detach_offscreen_session(&second);
  mln_gpu_device_release(device);
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
  mln_gpu_device device = create_device();
  mln_test_render_fixture first = attach_offscreen_session(first_map, device);
  mln_test_render_fixture second = attach_offscreen_session(second_map, device);

  render_red_frame(first_map, &first, 11);
  render_red_frame(second_map, &second, 12);
  expect_red_pixels(&first);
  expect_red_pixels(&second);

  detach_offscreen_session(&first);
  detach_offscreen_session(&second);
  mln_gpu_device_release(device);
  mln_test_destroy_map(second_map);
  mln_test_destroy_map(first_map);
  mln_test_destroy_runtime(runtime);
}

static void queue_lock_callback(void* user_data) { (void)user_data; }

static void attach_rejects_what_an_offscreen_session_cannot_take(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_gpu_device device = create_device();
  const mln_render_target descriptor = offscreen_target();
  const mln_render_session_attach_options options = core_worker_options();

  mln_render_target undersized = descriptor;
  undersized.size -= 1;
  expect_attach_status(
    MLN_STATUS_INVALID_ARGUMENT, map, device, &undersized, &options
  );
  mln_render_target empty = descriptor;
  empty.data.offscreen.logical_size.width = 0;
  expect_attach_status(
    MLN_STATUS_INVALID_ARGUMENT, map, device, &empty, &options
  );
  mln_render_target unnamed = descriptor;
  unnamed.kind = (mln_render_target_kind)0;
  expect_attach_status(
    MLN_STATUS_INVALID_ARGUMENT, map, device, &unnamed, &options
  );
  mln_render_target unknown = descriptor;
  unknown.kind = (mln_render_target_kind)UINT32_MAX;
  expect_attach_status(
    MLN_STATUS_INVALID_ARGUMENT, map, device, &unknown, &options
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

  mln_gpu_device_release(device);
  expect_attach_status(
    MLN_STATUS_INVALID_STATE, map, device, &descriptor, &options
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void the_first_named_adapter_renders(void) {
  mln_gpu_device_options options = {.size = sizeof(mln_gpu_device_options)};
  options.fields = MLN_GPU_DEVICE_OPTION_ADAPTER_INDEX;
  options.adapter_index = 0;
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_gpu_device_create(&options, &device, MLN_TEST_DIAGNOSTIC));
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_render_fixture fixture = attach_offscreen_session(map, device);

  render_red_frame(map, &fixture, 13);
  expect_red_pixels(&fixture);

  detach_offscreen_session(&fixture);
  mln_gpu_device_release(device);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(a_session_without_a_device_renders_a_still_image);
  RUN_TEST(a_session_without_a_device_renders_frames_in_turn);
  RUN_TEST(a_session_without_a_device_renders_still_images_in_turn);
  RUN_TEST(a_session_without_a_device_renders_a_demand_after_a_still_image);
  RUN_TEST(disposing_a_session_without_a_device_ends_it);
  RUN_TEST(work_behind_a_readback_runs_after_it);
  RUN_TEST(a_resize_changes_the_scale_factor_and_keeps_the_tiles);
#if defined(DEVICE_SUPPORTED)
  RUN_TEST(a_session_reads_back_after_its_device_is_released);
  RUN_TEST(sessions_on_two_maps_take_turns_on_one_device);
  RUN_TEST(sessions_on_two_maps_share_one_device);
  RUN_TEST(attach_rejects_what_an_offscreen_session_cannot_take);
  RUN_TEST(the_first_named_adapter_renders);
#endif
}
