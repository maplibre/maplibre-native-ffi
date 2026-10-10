// The status and diagnostic contract every call shares, how a handle parameter
// rejects a value that names no live object, and what the library reports
// about its own build.

#include "support/test_support.h"

static void a_failed_call_writes_its_diagnostic_and_a_successful_call_clears_it(
  void
) {
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  mln_event_batch_view view = {.size = sizeof(view)};
  MLN_TEST_INVALID(mln_event_batch_get(MLN_HANDLE_NULL, &view, &diagnostic));
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(diagnostic.message));

  uint32_t network_status = 0;
  MLN_TEST_OK(mln_network_status_get(&network_status, &diagnostic));
  TEST_ASSERT_EQUAL_size_t(0, strlen(diagnostic.message));
}

static void a_diagnostic_is_written_within_its_declared_size(void) {
  mln_diagnostic diagnostic;
  memset(&diagnostic, 'x', sizeof(diagnostic));
  diagnostic.size = (uint32_t)(offsetof(mln_diagnostic, message) + 4);
  mln_event_batch_view view = {.size = sizeof(view)};
  MLN_TEST_INVALID(mln_event_batch_get(MLN_HANDLE_NULL, &view, &diagnostic));
  TEST_ASSERT_EQUAL_size_t(3, strlen(diagnostic.message));
  TEST_ASSERT_EQUAL_INT('x', diagnostic.message[4]);
}

typedef struct batch_rejection {
  const char* label;
  // Builds the handle to pass. The runtime is live for the row's duration.
  mln_event_batch (*handle)(mln_runtime runtime);
  const char* fragment;
} batch_rejection;

static mln_event_batch null_batch(mln_runtime runtime) {
  (void)runtime;
  return MLN_HANDLE_NULL;
}

static mln_event_batch runtime_as_batch(mln_runtime runtime) { return runtime; }

static mln_event_batch malformed_batch(mln_runtime runtime) {
  (void)runtime;
  return (mln_event_batch)0xDEADBEEFDEADBEEFULL;
}

static const batch_rejection batch_rejections[] = {
  {"the null handle", null_batch, "must not be null"},
  {"a live handle of another kind", runtime_as_batch, "mln_runtime"},
  {"a value no handle kind uses", malformed_batch,
   "not a valid mln_event_batch"},
};

// Reading a value that names no live event batch reports why and leaves the
// output untouched, and releasing one is a no-op that leaves whatever the
// value does name alone.
static void a_value_that_names_no_batch_is_rejected_without_effect(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const size_t rows = sizeof(batch_rejections) / sizeof(*batch_rejections);
  for (size_t index = 0; index < rows; index += 1) {
    const batch_rejection* row = &batch_rejections[index];
    const mln_event_batch value = row->handle(runtime);
    const mln_runtime_event sentinel = {0};
    // A valid size makes the handle the argument that fails.
    mln_event_batch_view view = {
      .size = sizeof(view), .events = &sentinel, .event_count = 99
    };
    mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT,
      mln_event_batch_get(value, &view, &diagnostic), row->label
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(diagnostic.message, row->fragment), row->label
    );
    TEST_ASSERT_EQUAL_PTR_MESSAGE(&sentinel, view.events, row->label);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(99, view.event_count, row->label);

    mln_event_batch_release(value);
  }

  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  MLN_TEST_INVALID(mln_event_batch_get(MLN_HANDLE_NULL, NULL, &diagnostic));
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "out_view"));

  // Releasing the runtime's value as an event batch left the runtime live.
  MLN_TEST_OK(mln_test_runtime_barrier(runtime));
  mln_test_destroy_runtime(runtime);
}

// MapLibre throws while parsing malformed style JSON. The command reports that
// exception as NATIVE_ERROR carrying the exception's text, the same text the
// map's loading-failed event carries.
static void a_native_exception_becomes_native_error_with_its_text(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_set_style_json(
    map, MLN_BUFFER_LITERAL("{"), &completion.descriptor, NULL
  ));
  MLN_TEST_STATUS(
    MLN_STATUS_NATIVE_ERROR, mln_test_completion_finish(&completion)
  );
  TEST_ASSERT_EQUAL_UINT32(
    MLN_COMMAND_DISPOSITION_FAILED, mln_test_completion_disposition(&completion)
  );
  char diagnostic[MLN_DIAGNOSTIC_MESSAGE_CAPACITY];
  const char* completion_diagnostic =
    mln_test_completion_diagnostic(&completion);
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(completion_diagnostic));
  TEST_ASSERT_LESS_THAN_size_t(
    sizeof(diagnostic), strlen(completion_diagnostic)
  );
  snprintf(diagnostic, sizeof(diagnostic), "%s", completion_diagnostic);
  mln_test_completion_destroy(&completion);

  mln_runtime_event event = {0};
  char message[MLN_DIAGNOSTIC_MESSAGE_CAPACITY];
  TEST_ASSERT_TRUE(mln_test_await_event(
    runtime, MLN_RUNTIME_EVENT_MAP_LOADING_FAILED, map, &event, message,
    sizeof(message)
  ));
  TEST_ASSERT_EQUAL_STRING(diagnostic, message);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// A binding compares the ABI version against the one it was generated for,
// and the backend mask decides which render targets it offers. The mask must
// name exactly the backend this preset built.
static void the_library_reports_its_abi_version_and_built_backend(void) {
  TEST_ASSERT_EQUAL_UINT32(0, mln_c_version());

#if defined(MLN_FFI_TEST_BACKEND_METAL)
  const uint32_t expected = MLN_RENDER_BACKEND_FLAG_METAL;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  const uint32_t expected = MLN_RENDER_BACKEND_FLAG_VULKAN;
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
  const uint32_t expected = MLN_RENDER_BACKEND_FLAG_OPENGL;
#elif defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  const uint32_t expected = MLN_RENDER_BACKEND_FLAG_WEBGPU;
#else
#error "the native suite builds for exactly one render backend"
#endif
  TEST_ASSERT_EQUAL_HEX32(expected, mln_supported_render_backend_mask());
}

MLN_TEST_GROUP {
  RUN_TEST(a_failed_call_writes_its_diagnostic_and_a_successful_call_clears_it);
  RUN_TEST(a_diagnostic_is_written_within_its_declared_size);
  RUN_TEST(a_value_that_names_no_batch_is_rejected_without_effect);
  RUN_TEST(a_native_exception_becomes_native_error_with_its_text);
  RUN_TEST(the_library_reports_its_abi_version_and_built_backend);
}
