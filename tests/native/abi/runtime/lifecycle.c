// Raw C ABI coverage for synchronous runtime lifecycle and ordered barriers.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void runtime_creation_returns_a_runtime(void) {
  const mln_runtime_options options = mln_runtime_options_default();

  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create(&options, &runtime, NULL)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_create(&options, &runtime, NULL)
  );

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_close(runtime));
}

static void close_preflight_leaves_a_runtime_with_a_live_child_open(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_completion discard = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_runtime_release(runtime, &discard, NULL)
  );

  uint64_t mask = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_get_event_mask(runtime, &mask, NULL)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A barrier waits for earlier work that is still pending. A static map with no
// render session holds a still image until the map closes, and the resize
// after the barrier is the fence: once it completes, the barrier has been
// admitted and would have completed already if it ignored the still image.
static void a_barrier_completes_after_preceding_work(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map_options options = mln_map_options_default();
  options.map_mode = MLN_MAP_MODE_STATIC;
  mln_map map = mln_test_create_map_with_options(runtime, &options);
  mln_test_completion still = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_request_still_image(map, &still.descriptor, NULL)
  );
  mln_test_completion barrier = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_barrier(runtime, &barrier.descriptor, NULL)
  );
  MLN_TEST_AWAIT_COMMAND(
    MLN_STATUS_OK,
    mln_map_resize(
      map, (mln_logical_extent){128, 128, 1.0}, &completion.descriptor, NULL
    )
  );
  TEST_ASSERT_FALSE(mln_test_completion_poll(&barrier));

  // Closing the map cancels the still image, which lets the barrier finish.
  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&barrier));
  TEST_ASSERT_TRUE(mln_test_completion_poll(&still));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_CANCELLED, mln_test_completion_status(&still)
  );
  mln_test_completion_destroy(&barrier);
  mln_test_completion_destroy(&still);
  mln_test_destroy_runtime(runtime);
}

typedef struct creator_thread_probe {
  mln_runtime runtime;
  mln_map map;
  mln_status status;
} creator_thread_probe;

static void create_on_temporary_thread(void* argument) {
  creator_thread_probe* probe = argument;
  const mln_runtime_options options = mln_runtime_options_default();
  probe->status = mln_runtime_create(&options, &probe->runtime, NULL);
  if (probe->status != MLN_STATUS_OK) return;

  mln_test_completion create_map = mln_test_completion_default(sizeof(mln_map));
  probe->status =
    mln_map_create(probe->runtime, NULL, &create_map.descriptor, NULL);
  if (probe->status == MLN_STATUS_OK) {
    probe->status = mln_test_completion_finish_value(
      &create_map, &probe->map, sizeof(probe->map)
    );
  } else {
    mln_test_completion_reject(&create_map);
    mln_test_completion_destroy(&create_map);
  }
  if (probe->status != MLN_STATUS_OK) {
    (void)mln_test_runtime_close(probe->runtime);
  }
}

static void runtime_and_map_outlive_the_creating_host_thread(void) {
  creator_thread_probe probe = {.status = MLN_STATUS_NATIVE_ERROR};
  mln_test_thread_join(
    mln_test_thread_start(create_on_temporary_thread, &probe)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probe.status);

  const mln_logical_extent extent = {320, 240, 1.0};
  mln_test_completion resize = mln_test_completion_default(0);
  mln_status resize_status =
    mln_map_resize(probe.map, extent, &resize.descriptor, NULL);
  if (resize_status == MLN_STATUS_OK) {
    resize_status = mln_test_completion_settle(&resize);
  } else {
    mln_test_completion_reject(&resize);
    mln_test_completion_destroy(&resize);
  }
  const mln_status barrier_status = mln_test_runtime_barrier(probe.runtime);
  mln_map_snapshot snapshot = {.size = sizeof(mln_map_snapshot)};
  const mln_status snapshot_status =
    mln_map_snapshot_get(probe.map, &snapshot, NULL);
  const mln_status map_close_status = mln_test_map_close(probe.map);
  const mln_status runtime_close_status = mln_test_runtime_close(probe.runtime);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, resize_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, barrier_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, snapshot_status);
  TEST_ASSERT_EQUAL_UINT32(extent.width, snapshot.logical_extent.width);
  TEST_ASSERT_EQUAL_UINT32(extent.height, snapshot.logical_extent.height);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, map_close_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, runtime_close_status);
}

typedef struct close_probe {
  mln_runtime runtime;
  atomic_int status;
} close_probe;

static mln_runtime create_untracked_runtime(void) {
  const mln_runtime_options options = mln_runtime_options_default();
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create(&options, &runtime, NULL)
  );
  return runtime;
}

static void close_from_foreign_thread(void* argument) {
  close_probe* probe = argument;
  const mln_completion discard = mln_test_discard_completion();
  atomic_store(
    &probe->status, mln_runtime_release(probe->runtime, &discard, NULL)
  );
}

static void accepted_close_is_any_thread_and_retires_the_handle(void) {
  mln_runtime runtime = create_untracked_runtime();
  close_probe probe = {
    .runtime = runtime,
  };
  atomic_init(&probe.status, MLN_STATUS_NATIVE_ERROR);
  mln_test_thread* thread =
    mln_test_thread_start(close_from_foreign_thread, &probe);
  mln_test_thread_join(thread);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, atomic_load(&probe.status));

  uint64_t mask = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_get_event_mask(runtime, &mask, NULL)
  );
}

static void disposal_wake(void* user_data) { (void)user_data; }

static void disposal_release(void* user_data) {
  atomic_store((atomic_bool*)user_data, true);
}

static void disposal_waits_for_children_and_retires_callback_state(void) {
  atomic_bool released;
  atomic_init(&released, false);
  mln_runtime_options options = mln_runtime_options_default();
  options.event_wake.callback = disposal_wake;
  options.event_wake.user_data = &released;
  options.event_wake.release_user_data = disposal_release;
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create(&options, &runtime, NULL)
  );
  mln_map map = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_map_create_status(runtime, NULL, &map)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_dispose(runtime, NULL));
  uint64_t mask = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_get_event_mask(runtime, &mask, NULL)
  );
  TEST_ASSERT_FALSE(atomic_load(&released));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_map_dispose(map, NULL));
  TEST_ASSERT_TRUE(mln_test_wait_for_flag(&released));
}

MLN_TEST_GROUP {
  RUN_TEST(runtime_creation_returns_a_runtime);
  RUN_TEST(disposal_waits_for_children_and_retires_callback_state);
  RUN_TEST(close_preflight_leaves_a_runtime_with_a_live_child_open);
  RUN_TEST(a_barrier_completes_after_preceding_work);
  RUN_TEST(accepted_close_is_any_thread_and_retires_the_handle);
  RUN_TEST(runtime_and_map_outlive_the_creating_host_thread);
}
