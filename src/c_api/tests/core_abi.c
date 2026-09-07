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
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_destroy(MLN_HANDLE_NULL)
  );
}

static void runtime_rejects_stale_handles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_destroy_runtime(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_destroy(runtime)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_pump(runtime, 0, -1)
  );
}

static void runtime_pump_rejects_null_runtime(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_pump(MLN_HANDLE_NULL, 0, -1)
  );
}

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

static void map_lifecycle_rejects_invalid_state_and_stale_handles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_destroy_map(map);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_INVALID_ARGUMENT, mln_map_destroy(map));
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_json(map, MLN_BUFFER_LITERAL("{}"))
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

static void style_functions_reject_null_inputs(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_set_style_json(map, (mln_buffer_view){0})
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_set_style_url(map, NULL)
  );

  // The copy entry points treat a null buffer as a probe only at zero capacity,
  // and always need somewhere to report the required size.
  size_t size = 0;
  char buffer[8] = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_loaded_style_json(map, NULL, sizeof(buffer), &size)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_loaded_style_json(map, buffer, sizeof(buffer), NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_url(map, NULL, sizeof(buffer), &size)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_copy_style_url(map, buffer, sizeof(buffer), NULL)
  );

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

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

// The native identity of the calling thread, which is what the runtime's
// owner-thread check compares.
static uintptr_t current_thread_id(void) {
#if defined(_WIN32)
  return (uintptr_t)GetCurrentThreadId();
#else
  return (uintptr_t)pthread_self();
#endif
}

typedef struct leaked_runtime_probe {
  uintptr_t thread_id;
  mln_runtime runtime;
  mln_status create_status;
} leaked_runtime_probe;

// Exits with its runtime live, which is the leak under test.
static void leak_runtime_entry(void* argument) {
  leaked_runtime_probe* probe = argument;
  probe->thread_id = current_thread_id();
  const mln_runtime_options options = mln_runtime_options_default();
  probe->create_status = mln_runtime_create(&options, &probe->runtime);
}

typedef struct reused_thread_probe {
  uintptr_t wanted_thread_id;
  atomic_bool* release;
  atomic_bool reported;
  bool matched;
  mln_status create_status;
  mln_status destroy_status;
} reused_thread_probe;

// A thread that reuses the exited owner's identity creates a runtime; any other
// thread stays alive until released, so the next thread cannot take its
// identity and the platform hands out the exited owner's in turn.
static void reused_thread_entry(void* argument) {
  reused_thread_probe* probe = argument;
  probe->matched = current_thread_id() == probe->wanted_thread_id;
  if (!probe->matched) {
    atomic_store(&probe->reported, true);
    while (!atomic_load(probe->release)) {
      mln_test_sleep_millisecond();
    }
    return;
  }

  mln_runtime runtime = MLN_HANDLE_NULL;
  const mln_runtime_options options = mln_runtime_options_default();
  probe->create_status = mln_runtime_create(&options, &runtime);
  if (probe->create_status == MLN_STATUS_OK) {
    probe->destroy_status = mln_runtime_destroy(runtime);
  }
  atomic_store(&probe->reported, true);
}

enum { thread_reuse_attempts = 32 };

// Only the owner thread can destroy a runtime, so a runtime whose owner thread
// exited can never leave the C API. Its thread identity must not leave with
// it: platforms reuse thread identities, and a later thread that gets this one
// creates a runtime of its own.
static void a_runtime_that_outlives_its_owner_thread_releases_the_thread(void) {
  leaked_runtime_probe leaked = {0};
  mln_test_thread_join(mln_test_thread_start(leak_runtime_entry, &leaked));
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, leaked.create_status);

  // The orphaned handle reports the leak, on any thread, ahead of the
  // owner-thread check that would otherwise name this thread as the problem.
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_runtime_pump(leaked.runtime, 0, -1)
  );
  TEST_ASSERT_EQUAL_STRING(
    "runtime is orphaned: its owner thread exited without destroying it",
    mln_thread_last_error_message()
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_STATE, mln_runtime_destroy(leaked.runtime)
  );

  atomic_bool release;
  atomic_init(&release, false);
  reused_thread_probe probes[thread_reuse_attempts] = {0};
  mln_test_thread* threads[thread_reuse_attempts] = {0};
  size_t started = 0;
  size_t matched = thread_reuse_attempts;
  while (started < thread_reuse_attempts && matched == thread_reuse_attempts) {
    reused_thread_probe* probe = &probes[started];
    probe->wanted_thread_id = leaked.thread_id;
    probe->release = &release;
    atomic_init(&probe->reported, false);
    threads[started] = mln_test_thread_start(reused_thread_entry, probe);
    started += 1;
    while (!atomic_load(&probe->reported)) {
      mln_test_sleep_millisecond();
    }
    if (probe->matched) {
      matched = started - 1;
    }
  }
  atomic_store(&release, true);
  for (size_t index = 0; index < started; index += 1) {
    mln_test_thread_join(threads[index]);
  }

  if (matched == thread_reuse_attempts) {
    // The platform decides when it reuses a thread identity, and the test has
    // no way to make it do so.
    TEST_IGNORE_MESSAGE("No thread reused the exited owner's identity.");
  }
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probes[matched].create_status);
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, probes[matched].destroy_status);
}

void run_core_abi_tests(void) {
  UnitySetTestFile(__FILE__);
  RUN_TEST(runtime_rejects_invalid_arguments);
  RUN_TEST(runtime_rejects_stale_handles);
  RUN_TEST(runtime_pump_rejects_null_runtime);
  RUN_TEST(map_create_rejects_invalid_arguments);
  RUN_TEST(map_lifecycle_rejects_invalid_state_and_stale_handles);
  RUN_TEST(style_functions_reject_null_inputs);
  RUN_TEST(failing_status_sets_and_successful_status_clears_diagnostics);
  RUN_TEST(diagnostics_are_thread_local);
  RUN_TEST(a_runtime_that_outlives_its_owner_thread_releases_the_thread);
}
