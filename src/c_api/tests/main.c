#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

void setUp(void) {}

// Unity runs this even after a test body aborts on a failing assertion, so
// reclaiming leaked handles here keeps one failure from cascading into every
// later test that needs a runtime on this thread.
void tearDown(void) {
  if (!mln_test_reclaim_thread_resources()) {
    return;
  }
  if (!Unity.CurrentTestFailed && !Unity.CurrentTestIgnored) {
    TEST_FAIL_MESSAGE(
      "Test finished with live handles; the suite reclaimed them. Destroy the "
      "render session, map, and runtime the test created."
    );
  }
}

int main(void) {
  UNITY_BEGIN();
  run_browser_http_abi_tests();
  run_callback_adapter_abi_tests();
  run_core_abi_tests();
  run_custom_geometry_source_abi_tests();
  run_handles_abi_tests();
  run_map_options_abi_tests();
  run_render_backend_abi_tests();
  run_owned_texture_abi_tests();
  run_render_target_lifecycle_abi_tests();
  run_render_thread_abi_tests();
  run_query_abi_tests();
  run_notification_operation_abi_tests();
  run_projection_abi_tests();
  run_mlt_decode_abi_tests();
  run_resources_abi_tests();
  run_runtime_events_abi_tests();
  run_runtime_lifecycle_abi_tests();
  run_style_values_abi_tests();
  const int failures = UNITY_END();
  // main() runs on a pthread under -sPROXY_TO_PTHREAD, where a graphics device
  // held past the entry point keeps the runtime alive and the process never
  // exits.
  mln_test_release_thread_gpu_resources();
  return failures;
}
