// Raw C ABI coverage: core runtime/map/style/event/diagnostic tests for unsafe
// inputs, stale handles, and diagnostic writes hidden by bindings.

#include <stddef.h>
#include <string.h>

#include "support/harness.h"
#include "support/test_support.h"
#include "unity.h"

static void runtime_rejects_invalid_arguments(void) {
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_create(NULL, NULL, NULL)
  );

  mln_runtime_options small_options = mln_runtime_options_default();
  small_options.size = sizeof(mln_runtime_options) - 1;
  mln_runtime runtime = MLN_HANDLE_NULL;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_create(&small_options, &runtime, NULL)
  );
  TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, runtime);

  runtime = 1;
  const mln_runtime_options options = mln_runtime_options_default();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_create(&options, &runtime, NULL)
  );
  const mln_completion discard = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_release(MLN_HANDLE_NULL, &discard, NULL)
  );
}

static void runtime_rejects_stale_handles(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_destroy_runtime(runtime);
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_release(runtime, &completion, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_runtime_barrier(runtime, &completion, NULL)
  );
}

static void runtime_barrier_rejects_null_runtime(void) {
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_runtime_barrier(MLN_HANDLE_NULL, &completion, NULL)
  );
}

static void map_create_rejects_invalid_arguments(void) {
  const mln_map_options options = mln_map_options_default();
  mln_completion completion = mln_test_discard_completion();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_create(MLN_HANDLE_NULL, &options, &completion, NULL)
  );

  mln_runtime runtime = mln_test_create_runtime();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_create(runtime, &options, NULL, NULL)
  );
  mln_map_options small_options = mln_map_options_default();
  small_options.size = sizeof(mln_map_options) - 1;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_create(runtime, &small_options, &completion, NULL)
  );

  mln_map_options invalid_options = mln_map_options_default();
  invalid_options.map_mode = (mln_map_mode)999;
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_create(runtime, &invalid_options, &completion, NULL)
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
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_request_still_image(map, &completion, NULL)
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

  // A rejected style read never invokes or releases the completion, so the
  // caller still owns the user_data and releases it itself.
  mln_test_completion held = mln_test_completion_buffer_view();
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_loaded_style_json(map, NULL, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT, mln_map_style_url(map, NULL, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_loaded_style_json(MLN_HANDLE_NULL, &held.descriptor, NULL)
  );
  TEST_ASSERT_EQUAL_INT(
    MLN_STATUS_INVALID_ARGUMENT,
    mln_map_style_url(MLN_HANDLE_NULL, &held.descriptor, NULL)
  );
  TEST_ASSERT_FALSE(mln_test_completion_poll(&held));
  mln_test_completion_reject(&held);
  mln_test_completion_destroy(&held);

  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

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

MLN_TEST_GROUP {
  RUN_TEST(runtime_rejects_invalid_arguments);
  RUN_TEST(runtime_rejects_stale_handles);
  RUN_TEST(runtime_barrier_rejects_null_runtime);
  RUN_TEST(map_create_rejects_invalid_arguments);
  RUN_TEST(map_lifecycle_rejects_invalid_state_and_stale_handles);
  RUN_TEST(style_functions_reject_null_inputs);
  RUN_TEST(a_failed_call_writes_its_diagnostic_and_a_successful_call_clears_it);
  RUN_TEST(a_diagnostic_is_written_within_its_declared_size);
}
