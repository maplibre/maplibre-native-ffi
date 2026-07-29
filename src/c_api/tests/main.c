#include "abi_tests.h"
#include "test_support.h"
#include "unity.h"

void setUp(void) {}

// Unity runs this even when a test body aborted through a failing assertion, so
// it is the one place that reliably sees handles a test left behind. Reclaiming
// them here keeps a single genuine failure from cascading into every later test
// that needs a runtime on this thread.
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
  run_core_abi_tests();
  run_handles_abi_tests();
  run_map_options_abi_tests();
  run_render_backend_abi_tests();
  run_owned_texture_abi_tests();
  run_render_thread_abi_tests();
  run_query_abi_tests();
  run_resources_abi_tests();
  run_runtime_wake_abi_tests();
  run_style_values_abi_tests();
  return UNITY_END();
}
