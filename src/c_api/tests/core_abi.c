// Raw C ABI coverage: core runtime/map/style/event/diagnostic tests for unsafe
// inputs, stale handles, and thread-local diagnostics hidden by bindings.

#include <stdint.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

static mln_runtime_event empty_event(void) {
  return (mln_runtime_event){
    .size = sizeof(mln_runtime_event),
    .source_type = MLN_RUNTIME_EVENT_SOURCE_RUNTIME,
    .payload_type = MLN_RUNTIME_EVENT_PAYLOAD_NONE,
  };
}

// This verifies null inputs, undersized options, preinitialized outputs, and
// null destroy calls hidden by safe bindings.
static void runtime_rejects_invalid_arguments(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_create(NULL, NULL)
  );

  mln_runtime_options small_options = mln_runtime_options_default();
  small_options.size = sizeof(mln_runtime_options) - 1;
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_create(&small_options, &runtime)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);

  runtime = 1;
  const mln_runtime_options options = mln_runtime_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_create(&options, &runtime)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_destroy(MLN_HANDLE_NULL)
  );
}

// This verifies rejection of unknown raw flag bits that typed binding option
// sets cannot represent.
static void runtime_rejects_unknown_flags(void) {
  mln_runtime_options options = mln_runtime_options_default();
  options.flags = UINT32_C(1) << 31;
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_create(&options, &runtime)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);
}

// This verifies the raw handle registry rejects use-after-destroy calls that
// binding-owned handle state prevents.
static void runtime_rejects_stale_handles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_destroy(runtime)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_pump(runtime, 0)
  );
}

// This verifies the public null-handle contract for a raw entry point that
// bindings do not call with null.
static void runtime_pump_rejects_null_runtime(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_pump(MLN_HANDLE_NULL, 0)
  );
}

// This verifies raw event polling rejects a null runtime handle that binding
// handle-state checks prevent.
static void runtime_event_polling_rejects_null_runtime(void) {
  mln_runtime_event event = empty_event();
  bool has_event = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_poll_event(MLN_HANDLE_NULL, &event, &has_event)
  );
}

// This verifies raw runtime, output-handle, struct-size, and enum validation
// hidden by binding constructors.
static void map_create_rejects_invalid_arguments(void) {
  mln_map map = MLN_HANDLE_NULL;
  const mln_map_options options = mln_map_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_create(MLN_HANDLE_NULL, &options, &map)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, map);

  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_create(runtime, &options, NULL)
  );
  map = 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_create(runtime, &options, &map)
  );

  map = MLN_HANDLE_NULL;
  mln_map_options small_options = mln_map_options_default();
  small_options.size = sizeof(mln_map_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_create(runtime, &small_options, &map)
  );

  mln_map_options invalid_options = mln_map_options_default();
  invalid_options.map_mode = (mln_map_mode)999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_create(runtime, &invalid_options, &map)
  );
  mln_test_destroy_runtime(runtime);
}

// This verifies destroyed raw map pointers remain invalid across multiple C
// entry points.
static void map_lifecycle_rejects_invalid_state_and_stale_handles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_ARGUMENT, mln_map_destroy(map));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_style_json(map, "{}")
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_request_repaint(map)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_request_still_image(map)
  );
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_get_camera(map, &camera)
  );
  mln_test_destroy_runtime(runtime);
}

// This verifies null C-string validation hidden by binding strings.
static void style_functions_reject_null_inputs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_style_json(map, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_style_url(map, NULL)
  );
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// This verifies null and undersized raw output storage is rejected without
// writes through invalid pointers.
static void runtime_event_polling_rejects_invalid_outputs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  bool has_event = false;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_poll_event(runtime, NULL, &has_event)
  );
  mln_runtime_event event = empty_event();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_poll_event(runtime, &event, NULL)
  );
  event.size = sizeof(mln_runtime_event) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_poll_event(runtime, &event, &has_event)
  );
  mln_test_destroy_runtime(runtime);
}

// This verifies the C boundary's per-call diagnostic set-and-clear contract
// independently of binding error copying.
static void failing_status_sets_and_successful_status_clears_diagnostics(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_destroy(MLN_HANDLE_NULL)
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(mln_thread_last_error_message()));
  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_size_t(0, strlen(mln_thread_last_error_message()));
  mln_test_destroy_runtime(runtime);
}

typedef struct worker_diagnostic {
  mln_status status;
  size_t message_length;
} worker_diagnostic;

#if defined(_WIN32)
static DWORD WINAPI fail_on_thread(void* opaque_result) {
#else
static void* fail_on_thread(void* opaque_result) {
#endif
  worker_diagnostic* result = opaque_result;
  result->status = mln_runtime_destroy(MLN_HANDLE_NULL);
  result->message_length = strlen(mln_thread_last_error_message());
#if defined(_WIN32)
  return 0;
#else
  return NULL;
#endif
}

// This verifies native diagnostic storage isolation across host threads, below
// binding-owned thread checks.
static void diagnostics_are_thread_local(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_destroy(MLN_HANDLE_NULL)
  );
  char main_message[512] = {0};
  strncpy(
    main_message, mln_thread_last_error_message(), sizeof(main_message) - 1
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(main_message));

  worker_diagnostic worker_result = {0};
#if defined(_WIN32)
  HANDLE worker =
    CreateThread(NULL, 0, fail_on_thread, &worker_result, 0, NULL);
  TEST_ASSERT_NOT_NULL(worker);
  TEST_ASSERT_EQUAL_UINT32(
    WAIT_OBJECT_0, WaitForSingleObject(worker, INFINITE)
  );
  TEST_ASSERT_TRUE(CloseHandle(worker));
#else
  pthread_t worker;
  TEST_ASSERT_EQUAL_INT(
    0, pthread_create(&worker, NULL, fail_on_thread, &worker_result)
  );
  TEST_ASSERT_EQUAL_INT(0, pthread_join(worker, NULL));
#endif
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_ARGUMENT, worker_result.status);
  TEST_ASSERT_TRUE(worker_result.message_length > 0);
  TEST_ASSERT_EQUAL_STRING(main_message, mln_thread_last_error_message());

  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_size_t(0, strlen(mln_thread_last_error_message()));
  mln_test_destroy_runtime(runtime);
}

void run_core_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(runtime_rejects_invalid_arguments);
  RUN_TEST(runtime_rejects_unknown_flags);
  RUN_TEST(runtime_rejects_stale_handles);
  RUN_TEST(runtime_pump_rejects_null_runtime);
  RUN_TEST(runtime_event_polling_rejects_null_runtime);
  RUN_TEST(map_create_rejects_invalid_arguments);
  RUN_TEST(map_lifecycle_rejects_invalid_state_and_stale_handles);
  RUN_TEST(style_functions_reject_null_inputs);
  RUN_TEST(runtime_event_polling_rejects_invalid_outputs);
  RUN_TEST(failing_status_sets_and_successful_status_clears_diagnostics);
  RUN_TEST(diagnostics_are_thread_local);
}
