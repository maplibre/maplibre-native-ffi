// A WebGPU readback that the browser main thread services, as a page that
// drives its own session does. The main thread cannot wait for the readback's
// buffer map, which its event loop delivers only after the service returns, so
// the readback finishes on a later service and the session's later work waits
// behind it.
//
// The suite runs main() on a pthread, so the browser main thread is free to
// run a call proxied to it.

#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <string.h>
#include <webgpu/webgpu.h>

#include "support/frames.h"
#include "support/test_support.h"

enum { texture_size = 64 };

static const uint8_t red[4] = {255, 0, 0, 255};

typedef struct main_thread_case {
  mln_runtime runtime;
  mln_map map;
  mln_test_webgpu_main_thread_device device;
  mln_test_render_fixture fixture;
} main_thread_case;

// Attaches a session on a main-thread device and renders a red frame into it.
static void start_case(main_thread_case* run) {
  run->runtime = mln_test_create_runtime();
  run->map = mln_test_create_map(run->runtime);
  TEST_ASSERT_TRUE(mln_test_webgpu_main_thread_device_create(&run->device));
  TEST_ASSERT_TRUE(mln_test_webgpu_main_thread_fixture_create(
    run->map, &run->device, &run->fixture
  ));
  mln_test_load_style_and_wait(run->map, mln_test_red_background_style_json);
  mln_acquired_frame frame = mln_test_render_and_acquire(&run->fixture, 1);
  mln_test_render_release_frame(&frame);
}

static void finish_case(main_thread_case* run) {
  mln_test_render_fixture_destroy(&run->fixture);
  mln_test_webgpu_main_thread_device_destroy(&run->device);
  mln_test_destroy_map(run->map);
  mln_test_destroy_runtime(run->runtime);
}

static void run_on_main_thread(void (*function)(void*), void* context) {
  TEST_ASSERT_EQUAL_INT(
    1, emscripten_proxy_sync(
         emscripten_proxy_get_system_queue(),
         emscripten_main_runtime_thread_id(), function, context
       )
  );
}

static mln_status service_here(const mln_test_render_fixture* fixture) {
  return mln_render_session_service_driver_work(fixture->session, NULL);
}

static void assert_center_is_red(const mln_texture_readback_result* result) {
  TEST_ASSERT_EQUAL_UINT32(texture_size, result->info.width);
  TEST_ASSERT_EQUAL_UINT32(texture_size, result->info.height);
  TEST_ASSERT_EQUAL_size_t(result->info.byte_length, result->data.size);
  const size_t center =
    (size_t)(texture_size / 2) * result->info.stride + (texture_size / 2) * 4;
  TEST_ASSERT_EQUAL_HEX8_ARRAY(
    red, (const uint8_t*)result->data.data + center, 4
  );
}

// One completion's delivery, numbered among the case's deliveries in the order
// the session made them.
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
    const size_t center = (size_t)(texture_size / 2) * readback->info.stride +
                          (texture_size / 2) * 4;
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

static bool was_delivered(void* context) {
  return atomic_load(&((ordered_delivery*)context)->delivered);
}

static void service_until_delivered(
  const mln_test_render_fixture* fixture, ordered_delivery* delivery
) {
  MLN_TEST_OK(mln_test_render_step_until(
    fixture, was_delivered, delivery, mln_test_deadline_default(),
    "a delivery behind a readback"
  ));
}

// The first service submits the copy and its map, then returns with the
// readback still pending: the map cannot finish inside a call on the main
// thread. Its callback wakes the driver, and the next service finishes the
// readback.
static void a_readback_serviced_on_the_main_thread_finishes_on_a_later_service(
  void
) {
  main_thread_case run = {0};
  start_case(&run);

  mln_test_completion readback = mln_test_completion_readback();
  MLN_TEST_OK(mln_render_session_read_texture(
    run.fixture.session, &readback.descriptor, NULL
  ));
  const unsigned int wakes = atomic_load(&run.fixture.driver_wakes);
  MLN_TEST_OK(mln_test_render_fixture_service(&run.fixture));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&readback));

  MLN_TEST_OK(
    mln_test_render_fixture_finish_operation(&run.fixture, &readback)
  );
  TEST_ASSERT_GREATER_THAN_UINT(wakes, atomic_load(&run.fixture.driver_wakes));
  mln_texture_readback_result result = {0};
  TEST_ASSERT_TRUE(
    mln_test_completion_copy_value(&readback, &result, sizeof(result))
  );
  assert_center_is_red(&result);
  mln_test_completion_destroy(&readback);

  finish_case(&run);
}

// Calls that the main thread makes in one turn, so no map can finish between
// them. Unity cannot fail a case from the main thread, so each call records
// its status for the case to check.
typedef struct queued_behind {
  const mln_test_render_fixture* fixture;
  const mln_test_webgpu_main_thread_device* device;
  ordered_delivery* first;
  ordered_delivery* second;
  ordered_delivery* last;
  mln_status statuses[4];
} queued_behind;

static void read_twice_then_fence_here(void* context) {
  queued_behind* queued = context;
  const mln_render_session session = queued->fixture->session;
  const mln_completion first = ordered_completion(queued->first);
  const mln_completion second = ordered_completion(queued->second);
  const mln_completion last = ordered_completion(queued->last);
  queued->statuses[0] = mln_render_session_read_texture(session, &first, NULL);
  queued->statuses[1] = mln_render_session_read_texture(session, &second, NULL);
  queued->statuses[2] = service_here(queued->fixture);
  queued->statuses[3] =
    mln_render_session_dump_debug_logs(session, &last, NULL);
}

static void assert_queued(const queued_behind* queued, size_t count) {
  for (size_t index = 0; index < count; index += 1) {
    MLN_TEST_OK(queued->statuses[index]);
  }
}

// A second readback and a command queued while a readback is pending run after
// it, in the order they were accepted, and the second readback starts only
// once the first has finished.
static void work_behind_a_pending_readback_runs_after_it(void) {
  main_thread_case run = {0};
  start_case(&run);

  atomic_int sequence = 0;
  ordered_delivery first = {.sequence = &sequence};
  ordered_delivery second = {.sequence = &sequence};
  ordered_delivery fence = {.sequence = &sequence};
  queued_behind queued = {
    .fixture = &run.fixture,
    .first = &first,
    .second = &second,
    .last = &fence,
  };
  run_on_main_thread(read_twice_then_fence_here, &queued);
  assert_queued(&queued, 4);
  TEST_ASSERT_FALSE(atomic_load(&first.delivered));

  service_until_delivered(&run.fixture, &fence);
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

  finish_case(&run);
}

static void read_then_detach_here(void* context) {
  queued_behind* queued = context;
  const mln_render_session session = queued->fixture->session;
  const mln_completion read = ordered_completion(queued->first);
  const mln_completion detach = ordered_completion(queued->last);
  queued->statuses[0] = mln_render_session_read_texture(session, &read, NULL);
  queued->statuses[1] = service_here(queued->fixture);
  queued->statuses[2] = mln_render_session_detach(session, &detach, NULL);
}

// Detach waits for a pending readback instead of ending it: the readback
// copied its frame when it started, and finishes with those pixels before the
// detach releases the texture.
static void a_detach_behind_a_pending_readback_waits_for_it(void) {
  main_thread_case run = {0};
  start_case(&run);

  atomic_int sequence = 0;
  ordered_delivery read = {.sequence = &sequence};
  ordered_delivery detach = {.sequence = &sequence};
  queued_behind queued = {
    .fixture = &run.fixture,
    .first = &read,
    .last = &detach,
  };
  run_on_main_thread(read_then_detach_here, &queued);
  assert_queued(&queued, 3);
  TEST_ASSERT_FALSE(atomic_load(&read.delivered));

  service_until_delivered(&run.fixture, &detach);
  TEST_ASSERT_TRUE(atomic_load(&read.delivered));
  MLN_TEST_OK(read.status);
  MLN_TEST_OK(detach.status);
  TEST_ASSERT_EQUAL_INT(0, read.position);
  TEST_ASSERT_EQUAL_INT(1, detach.position);
  TEST_ASSERT_EQUAL_HEX8_ARRAY(red, read.center, 4);

  finish_case(&run);
}

static void service_then_lose_the_device_here(void* context) {
  queued_behind* queued = context;
  const mln_completion fence = ordered_completion(queued->last);
  queued->statuses[0] = service_here(queued->fixture);
  wgpuDeviceDestroy(queued->device->context.device);
  queued->statuses[1] =
    mln_render_session_dump_debug_logs(queued->fixture->session, &fence, NULL);
}

// Losing the device while a readback's map is pending fails the readback
// instead of leaving it pending. WebGPU aborts the map, its callback wakes the
// driver, and the next service fails the readback and runs the work behind it.
static void losing_the_device_fails_a_pending_readback(void) {
  main_thread_case run = {0};
  start_case(&run);

  atomic_int sequence = 0;
  ordered_delivery read = {.sequence = &sequence};
  ordered_delivery fence = {.sequence = &sequence};
  const mln_completion read_completion = ordered_completion(&read);
  MLN_TEST_OK(
    mln_render_session_read_texture(run.fixture.session, &read_completion, NULL)
  );
  queued_behind queued = {
    .fixture = &run.fixture,
    .device = &run.device,
    .first = &read,
    .last = &fence,
  };
  const unsigned int wakes = atomic_load(&run.fixture.driver_wakes);
  run_on_main_thread(service_then_lose_the_device_here, &queued);
  assert_queued(&queued, 2);
  TEST_ASSERT_FALSE(atomic_load(&read.delivered));

  service_until_delivered(&run.fixture, &fence);
  TEST_ASSERT_GREATER_THAN_UINT(wakes, atomic_load(&run.fixture.driver_wakes));
  TEST_ASSERT_TRUE(atomic_load(&read.delivered));
  MLN_TEST_STATUS(MLN_STATUS_NATIVE_ERROR, read.status);
  MLN_TEST_OK(fence.status);
  TEST_ASSERT_EQUAL_INT(0, read.position);
  TEST_ASSERT_EQUAL_INT(1, fence.position);

  finish_case(&run);
}

typedef struct abandoned_read {
  const mln_test_render_fixture* fixture;
  mln_completion completion;
  mln_status statuses[3];
} abandoned_read;

static void read_then_abandon_here(void* context) {
  abandoned_read* run = context;
  const mln_render_session session = run->fixture->session;
  run->statuses[0] =
    mln_render_session_read_texture(session, &run->completion, NULL);
  run->statuses[1] = service_here(run->fixture);
  mln_render_abandon_result abandoned = {
    .size = sizeof(mln_render_abandon_result)
  };
  run->statuses[2] = mln_render_session_abandon(session, &abandoned, NULL);
}

// Abandoning the session while its readback's map is pending fails the
// readback with MLN_STATUS_TARGET_LOST. The case checks only the completion:
// the map's callback runs on the main thread at some point before or after the
// case destroys the session and the device.
static void abandoning_a_pending_readback_fails_it_with_target_lost(void) {
  main_thread_case run = {0};
  start_case(&run);

  mln_test_completion readback = mln_test_completion_readback();
  abandoned_read abandon = {
    .fixture = &run.fixture,
    .completion = readback.descriptor,
  };
  run_on_main_thread(read_then_abandon_here, &abandon);
  for (size_t index = 0; index < 3; index += 1) {
    MLN_TEST_OK(abandon.statuses[index]);
  }
  MLN_TEST_STATUS(
    MLN_STATUS_TARGET_LOST, mln_test_completion_finish(&readback)
  );
  mln_test_completion_destroy(&readback);

  finish_case(&run);
}

MLN_TEST_GROUP {
  RUN_TEST(a_readback_serviced_on_the_main_thread_finishes_on_a_later_service);
  RUN_TEST(work_behind_a_pending_readback_runs_after_it);
  RUN_TEST(a_detach_behind_a_pending_readback_waits_for_it);
  RUN_TEST(losing_the_device_fails_a_pending_readback);
  RUN_TEST(abandoning_a_pending_readback_fails_it_with_target_lost);
}
