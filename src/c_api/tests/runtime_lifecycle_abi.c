// Raw C ABI coverage for operation-based runtime lifecycle and barriers.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static void wait_for_success(mln_operation operation) {
  bool completed = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_wait(operation, -1, &completed)
  );
  TEST_ASSERT_TRUE(completed);
  mln_status result = MLN_STATUS_NATIVE_ERROR;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_operation_get_status(operation, &result)
  );
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, result);
}

static void runtime_creation_returns_an_operation_result(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_notification_source_create(&source));
  mln_runtime_options options = mln_runtime_options_default();
  options.notification_source = source;

  mln_operation creation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create_start(&options, &creation)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, creation);
  wait_for_success(creation);

  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create_take_result(creation, &runtime)
  );
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE,
    mln_runtime_create_take_result(creation, &(mln_runtime){MLN_HANDLE_NULL})
  );
  mln_operation_release(creation);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_runtime_release(runtime));
  mln_notification_source_release(source);
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

static void a_barrier_waits_for_a_preceding_pending_operation(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_operation pending = MLN_HANDLE_NULL;
  mln_test_operation_control* control = NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_test_runtime_pending_operation_create(runtime, &pending, &control)
  );

  mln_operation barrier = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_barrier_start(runtime, &barrier)
  );
  bool completed = true;
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_operation_poll(barrier, &completed));
  TEST_ASSERT_FALSE(completed);

  mln_test_operation_complete(control, MLN_STATUS_OK, NULL);
  wait_for_success(barrier);
  mln_operation_release(barrier);
  mln_operation_release(pending);
  mln_test_operation_control_destroy(control);
  mln_test_destroy_runtime(runtime);
}

typedef struct close_probe {
  mln_runtime runtime;
  atomic_int status;
} close_probe;

static mln_runtime create_untracked_runtime(
  mln_notification_source* out_source
) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_notification_source_create(out_source)
  );
  mln_runtime_options options = mln_runtime_options_default();
  options.notification_source = *out_source;
  mln_operation creation = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create_start(&options, &creation)
  );
  wait_for_success(creation);
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_runtime_create_take_result(creation, &runtime)
  );
  mln_operation_release(creation);
  return runtime;
}

static void close_from_foreign_thread(void* argument) {
  close_probe* probe = argument;
  atomic_store(&probe->status, mln_runtime_release(probe->runtime));
}

static void accepted_close_is_any_thread_and_retires_the_handle(void) {
  mln_notification_source source = MLN_HANDLE_NULL;
  mln_runtime runtime = create_untracked_runtime(&source);
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
  mln_notification_source_release(source);
}

void run_runtime_lifecycle_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(runtime_creation_returns_an_operation_result);
  RUN_TEST(close_preflight_leaves_a_runtime_with_a_live_child_open);
  RUN_TEST(a_barrier_waits_for_a_preceding_pending_operation);
  RUN_TEST(accepted_close_is_any_thread_and_retires_the_handle);
}
