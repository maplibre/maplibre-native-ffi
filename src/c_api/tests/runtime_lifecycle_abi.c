// Raw C ABI coverage for synchronous runtime lifecycle and ordered barriers.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static void runtime_creation_returns_a_runtime(void) {
  const mln_runtime_options options = mln_runtime_options_default();

  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_create(&options, &runtime));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_create(&options, &runtime)
  );

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_close(runtime));
}

static void close_preflight_leaves_a_runtime_with_a_live_child_open(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  const mln_completion discard = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_runtime_release(runtime, &discard)
  );

  uint64_t mask = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_get_event_mask(runtime, &mask)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static bool wait_for_entry(const atomic_bool* entered) {
  const uint64_t deadline = mln_test_monotonic_milliseconds() + 5000;
  while (!atomic_load(entered) &&
         mln_test_monotonic_milliseconds() < deadline) {
    mln_test_sleep_milliseconds(1);
  }
  return atomic_load(entered);
}

static void a_barrier_completes_after_preceding_work(void) {
  mln_runtime runtime = mln_test_create_runtime();
  atomic_bool entered;
  void* pending_operation = NULL;
  atomic_init(&entered, false);
  mln_test_completion operation = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_pending_runtime_operation(
      runtime, &entered, &pending_operation, &operation.descriptor
    )
  );
  const bool operation_entered = wait_for_entry(&entered);
  mln_test_completion barrier = mln_test_completion_default(0);
  const mln_status accepted = mln_runtime_barrier(runtime, &barrier.descriptor);
  const bool completed_early = mln_test_completion_wait(&barrier, 100);
  mln_test_complete_runtime_operation(pending_operation);
  const mln_status operation_terminal = mln_test_completion_settle(&operation);
  const mln_status terminal = mln_test_completion_settle(&barrier);
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_TRUE(operation_entered);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, accepted);
  TEST_ASSERT_FALSE(completed_early);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, terminal);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, operation_terminal);
}

static void runtime_release_waits_for_retired_map_cleanup(void) {
  const mln_runtime_options options = mln_runtime_options_default();
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_create(&options, &runtime));
  mln_test_completion create_map = mln_test_completion_default(sizeof(mln_map));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_create(runtime, NULL, &create_map.descriptor)
  );
  mln_map map = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_completion_finish_value(&create_map, &map, sizeof(map))
  );
  atomic_bool entered;
  atomic_bool release;
  atomic_init(&entered, false);
  atomic_init(&release, false);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_test_block_map_cleanup(map, &entered, &release)
  );
  mln_test_completion map_close = mln_test_completion_default(0);
  const mln_status map_accepted = mln_map_release(map, &map_close.descriptor);
  const bool cleanup_entered = wait_for_entry(&entered);
  const bool map_completed = mln_test_completion_wait(&map_close, 100);
  mln_test_completion runtime_close = mln_test_completion_default(0);
  const mln_status runtime_accepted =
    mln_runtime_release(runtime, &runtime_close.descriptor);
  const bool runtime_completed_early =
    mln_test_completion_wait(&runtime_close, 100);
  atomic_store(&release, true);
  const mln_status map_terminal = mln_test_completion_settle(&map_close);
  const mln_status runtime_terminal =
    mln_test_completion_settle(&runtime_close);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, map_accepted);
  TEST_ASSERT_TRUE(cleanup_entered);
  TEST_ASSERT_TRUE(map_completed);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, runtime_accepted);
  TEST_ASSERT_FALSE(runtime_completed_early);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, map_terminal);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, runtime_terminal);
}

typedef struct close_probe {
  mln_runtime runtime;
  atomic_int status;
} close_probe;

static mln_runtime create_untracked_runtime(void) {
  const mln_runtime_options options = mln_runtime_options_default();
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_create(&options, &runtime));
  return runtime;
}

static void close_from_foreign_thread(void* argument) {
  close_probe* probe = argument;
  const mln_completion discard = mln_test_discard_completion();
  atomic_store(&probe->status, mln_runtime_release(probe->runtime, &discard));
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
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_get_event_mask(runtime, &mask)
  );
}

void run_runtime_lifecycle_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(runtime_creation_returns_a_runtime);
  RUN_TEST(close_preflight_leaves_a_runtime_with_a_live_child_open);
  RUN_TEST(a_barrier_completes_after_preceding_work);
  RUN_TEST(runtime_release_waits_for_retired_map_cleanup);
  RUN_TEST(accepted_close_is_any_thread_and_retires_the_handle);
}
