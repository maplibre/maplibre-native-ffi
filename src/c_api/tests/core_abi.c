// Raw C ABI coverage: core runtime/map/style/event/diagnostic tests for unsafe
// inputs, stale handles, and thread-local diagnostics hidden by bindings.

#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

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
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_release(MLN_HANDLE_NULL)
  );
}

static void runtime_rejects_stale_handles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_release(runtime)
  );
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_barrier(runtime, &completion)
  );
}

static void runtime_barrier_rejects_null_runtime(void) {
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_barrier(MLN_HANDLE_NULL, &completion)
  );
}

static void map_create_rejects_invalid_arguments(void) {
  const mln_map_options options = mln_map_options_default();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_create(MLN_HANDLE_NULL, &options, &completion)
  );

  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_create(runtime, &options, NULL)
  );
  mln_map_options small_options = mln_map_options_default();
  small_options.size = sizeof(mln_map_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_create(runtime, &small_options, &completion)
  );

  mln_map_options invalid_options = mln_map_options_default();
  invalid_options.map_mode = (mln_map_mode)999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_create(runtime, &invalid_options, &completion)
  );
  mln_test_destroy_runtime(runtime);
}

static void map_lifecycle_rejects_invalid_state_and_stale_handles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_ARGUMENT, mln_test_map_close(map));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_test_map_set_style_json(map, MLN_BUFFER_LITERAL("{}"))
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_map_request_repaint(map)
  );
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_request_still_image(map, &completion)
  );
  mln_camera_options camera = mln_camera_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_map_get_camera(map, &camera)
  );
  mln_test_destroy_runtime(runtime);
}

static void style_functions_reject_null_inputs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_test_map_set_style_json(map, (mln_buffer_view){0})
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_map_set_style_url(map, NULL)
  );

  // The copy entry points treat a null buffer as a probe only at zero capacity,
  // and always need somewhere to report the required size.
  size_t size = 0;
  char buffer[8] = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_test_map_copy_loaded_style_json(map, NULL, sizeof(buffer), &size)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_test_map_copy_loaded_style_json(map, buffer, sizeof(buffer), NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_test_map_copy_style_url(map, NULL, sizeof(buffer), &size)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_test_map_copy_style_url(map, buffer, sizeof(buffer), NULL)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static void failing_status_sets_and_successful_status_clears_diagnostics(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_runtime_close(MLN_HANDLE_NULL)
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
  result->status = mln_test_runtime_close(MLN_HANDLE_NULL);
  result->message_length = strlen(mln_thread_last_error_message());
#if defined(_WIN32)
  return 0;
#else
  return NULL;
#endif
}

static void diagnostics_are_thread_local(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_runtime_close(MLN_HANDLE_NULL)
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

static uint32_t ignore_log_record(
  void* user_data, uint32_t severity, uint32_t event, int64_t code,
  const char* message
) {
  (void)user_data;
  (void)severity;
  (void)event;
  (void)code;
  (void)message;
  return 0;
}

static void count_log_callback_release(void* user_data) { ++*(int*)user_data; }

static void log_callback_releases_owned_user_data(void) {
  int first_releases = 0;
  int second_releases = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_log_set_callback(
      ignore_log_record, &first_releases, count_log_callback_release
    )
  );
  TEST_ASSERT_EQUAL_INT(0, first_releases);

  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK,
    mln_log_set_callback(
      ignore_log_record, &second_releases, count_log_callback_release
    )
  );
  TEST_ASSERT_EQUAL_INT(1, first_releases);
  TEST_ASSERT_EQUAL_INT(0, second_releases);

  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_log_clear_callback());
  TEST_ASSERT_EQUAL_INT(1, second_releases);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_log_clear_callback());
  TEST_ASSERT_EQUAL_INT(1, second_releases);
}

void run_core_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(runtime_rejects_invalid_arguments);
  RUN_TEST(runtime_rejects_stale_handles);
  RUN_TEST(runtime_barrier_rejects_null_runtime);
  RUN_TEST(map_create_rejects_invalid_arguments);
  RUN_TEST(map_lifecycle_rejects_invalid_state_and_stale_handles);
  RUN_TEST(style_functions_reject_null_inputs);
  RUN_TEST(failing_status_sets_and_successful_status_clears_diagnostics);
  RUN_TEST(diagnostics_are_thread_local);
  RUN_TEST(log_callback_releases_owned_user_data);
}
