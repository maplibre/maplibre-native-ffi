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

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_release(runtime));
}

static void close_preflight_leaves_a_runtime_with_a_live_child_open(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_STATE, mln_runtime_release(runtime));

  uint64_t mask = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_get_event_mask(runtime, &mask)
  );
  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_reserve_child(runtime));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_STATE, mln_runtime_release(runtime));
  mln_test_runtime_abandon_child(runtime);
  mln_test_destroy_runtime(runtime);
}

static void a_barrier_completes_after_preceding_work(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_completion barrier = mln_test_completion_default(0);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_barrier(runtime, &barrier.descriptor)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_completion_finish(&barrier));
  mln_test_completion_destroy(&barrier);
  mln_test_destroy_runtime(runtime);
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
  atomic_store(&probe->status, mln_runtime_release(probe->runtime));
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
  RUN_TEST(accepted_close_is_any_thread_and_retires_the_handle);
}
