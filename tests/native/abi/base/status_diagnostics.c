// The status and diagnostic contract every call shares, the buffer handle
// boundary, and what the library reports about its own build.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void a_failed_call_writes_its_diagnostic_and_a_successful_call_clears_it(
  void
) {
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  mln_buffer_view view = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_buffer_get(MLN_HANDLE_NULL, &view, &diagnostic)
  );
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(diagnostic.message));

  uint32_t network_status = 0;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_network_status_get(&network_status, &diagnostic)
  );
  TEST_ASSERT_EQUAL_size_t(0, strlen(diagnostic.message));
}

static void a_diagnostic_is_written_within_its_declared_size(void) {
  mln_diagnostic diagnostic;
  memset(&diagnostic, 'x', sizeof(diagnostic));
  diagnostic.size = (uint32_t)(offsetof(mln_diagnostic, message) + 4);
  mln_buffer_view view = {0};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_buffer_get(MLN_HANDLE_NULL, &view, &diagnostic)
  );
  TEST_ASSERT_EQUAL_size_t(3, strlen(diagnostic.message));
  TEST_ASSERT_EQUAL_INT('x', diagnostic.message[4]);
}

typedef struct buffer_rejection {
  const char* label;
  // Builds the handle to pass. The runtime is live for the row's duration.
  mln_buffer (*handle)(mln_runtime runtime);
  const char* fragment;
} buffer_rejection;

static mln_buffer null_buffer(mln_runtime runtime) {
  (void)runtime;
  return MLN_HANDLE_NULL;
}

static mln_buffer runtime_as_buffer(mln_runtime runtime) { return runtime; }

static mln_buffer malformed_buffer(mln_runtime runtime) {
  (void)runtime;
  return (mln_buffer)0xDEADBEEFDEADBEEFULL;
}

static const buffer_rejection buffer_rejections[] = {
  {"the null handle", null_buffer, "must not be null"},
  {"a live handle of another kind", runtime_as_buffer, "mln_runtime"},
  {"a value no handle kind uses", malformed_buffer, "not a valid mln_buffer"},
};

// No public call returns a buffer handle, so every value a host can pass names
// no live buffer. Reading one reports why and leaves the output untouched, and
// destroying one is a no-op that leaves whatever the value does name alone.
static void a_value_that_names_no_buffer_is_rejected_without_effect(void) {
  mln_runtime runtime = mln_test_create_runtime();
  const size_t rows = sizeof(buffer_rejections) / sizeof(*buffer_rejections);
  for (size_t index = 0; index < rows; index += 1) {
    const buffer_rejection* row = &buffer_rejections[index];
    const mln_buffer buffer = row->handle(runtime);
    const char sentinel = 0;
    mln_buffer_view view = {.data = &sentinel, .size = 7};
    mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
    TEST_ASSERT_EQUAL_INT_MESSAGE(
      MLN_STATUS_INVALID_ARGUMENT, mln_buffer_get(buffer, &view, &diagnostic),
      row->label
    );
    TEST_ASSERT_NOT_NULL_MESSAGE(
      strstr(diagnostic.message, row->fragment), row->label
    );
    TEST_ASSERT_EQUAL_PTR_MESSAGE(&sentinel, view.data, row->label);
    TEST_ASSERT_EQUAL_size_t_MESSAGE(7, view.size, row->label);

    mln_buffer_destroy(buffer);
  }

  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_buffer_get(MLN_HANDLE_NULL, NULL, &diagnostic)
  );
  TEST_ASSERT_NOT_NULL(strstr(diagnostic.message, "out_view"));

  // Destroying the runtime's value as a buffer left the runtime live.
  TEST_ASSERT_EQUAL_INT(MLN_STATUS_OK, mln_test_runtime_barrier(runtime));
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
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_OK, mln_map_set_style_json(
                     map, MLN_BUFFER_LITERAL("{"), &completion.descriptor, NULL
                   )
  );
  TEST_ASSERT_EQUAL_INT(
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
  RUN_TEST(a_value_that_names_no_buffer_is_rejected_without_effect);
  RUN_TEST(a_native_exception_becomes_native_error_with_its_text);
  RUN_TEST(the_library_reports_its_abi_version_and_built_backend);
}
