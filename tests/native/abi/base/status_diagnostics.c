// The status and diagnostic contract every call shares, and what the library
// reports about its own build.

#include "support/test_support.h"

static void a_failed_call_writes_its_diagnostic_and_a_successful_call_clears_it(
  void
) {
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  mln_map_snapshot view = {.size = sizeof(view)};
  MLN_TEST_INVALID(mln_map_get_snapshot(MLN_HANDLE_NULL, &view, &diagnostic));
  TEST_ASSERT_GREATER_THAN_size_t(0, strlen(diagnostic.message));

  uint32_t network_status = 0;
  MLN_TEST_OK(mln_network_get_status(&network_status, &diagnostic));
  TEST_ASSERT_EQUAL_size_t(0, strlen(diagnostic.message));
}

static void a_diagnostic_is_written_within_its_declared_size(void) {
  mln_diagnostic diagnostic;
  memset(&diagnostic, 'x', sizeof(diagnostic));
  diagnostic.size = (uint32_t)(offsetof(mln_diagnostic, message) + 4);
  mln_map_snapshot view = {.size = sizeof(view)};
  MLN_TEST_INVALID(mln_map_get_snapshot(MLN_HANDLE_NULL, &view, &diagnostic));
  TEST_ASSERT_EQUAL_size_t(3, strlen(diagnostic.message));
  TEST_ASSERT_EQUAL_INT('x', diagnostic.message[4]);
}

// A style that fails to parse completes with the parser's text, which the
// loading-failed event repeats.
static void a_malformed_style_fails_with_the_parser_text(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_map(runtime);
  mln_test_drain_all(runtime);

  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_set_style_json(
    map, MLN_BUFFER_LITERAL("{"), &completion.descriptor, NULL
  ));
  MLN_TEST_STATUS(
    MLN_STATUS_INVALID_ARGUMENT, mln_test_completion_finish(&completion)
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

static void android_initialization_requires_its_environment_and_context(void) {
  int sentinel = 0;
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  MLN_TEST_INVALID(mln_android_init(NULL, &sentinel, &diagnostic));
  TEST_ASSERT_EQUAL_STRING(
    "jni_env and context must not be null", diagnostic.message
  );
#ifndef __ANDROID__
  MLN_TEST_STATUS(
    MLN_STATUS_UNSUPPORTED, mln_android_init(&sentinel, &sentinel, &diagnostic)
  );
  TEST_ASSERT_EQUAL_STRING(
    "Android initialization is not supported", diagnostic.message
  );
#endif
}

// The backend mask decides which render targets a binding offers. A build
// can draw with more backends than the preset's.
static void the_library_reports_its_built_backend(void) {
#if defined(MLN_FFI_TEST_BACKEND_METAL)
  const uint32_t expected = MLN_RENDER_BACKEND_FLAG_METAL;
#elif defined(MLN_FFI_TEST_BACKEND_VULKAN)
  const uint32_t expected = MLN_RENDER_BACKEND_FLAG_VULKAN;
#elif defined(MLN_FFI_TEST_BACKEND_OPENGL)
  const uint32_t expected = MLN_RENDER_BACKEND_FLAG_OPEN_GL;
#else
#error "the native suite builds for exactly one render backend"
#endif
  TEST_ASSERT_EQUAL_HEX32(
    expected, mln_supported_render_backend_mask() & expected
  );
}

MLN_TEST_GROUP {
  RUN_TEST(a_failed_call_writes_its_diagnostic_and_a_successful_call_clears_it);
  RUN_TEST(a_diagnostic_is_written_within_its_declared_size);
  RUN_TEST(a_malformed_style_fails_with_the_parser_text);
  RUN_TEST(android_initialization_requires_its_environment_and_context);
  RUN_TEST(the_library_reports_its_built_backend);
}
