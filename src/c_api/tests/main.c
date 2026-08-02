#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

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
  run_callback_adapter_abi_tests();
  run_core_abi_tests();
  run_handles_abi_tests();
  run_map_options_abi_tests();
  run_render_backend_abi_tests();
  run_owned_texture_abi_tests();
  run_render_target_lifecycle_abi_tests();
  run_render_thread_abi_tests();
  run_query_abi_tests();
  run_mlt_decode_abi_tests();
  run_resources_abi_tests();
  run_runtime_wake_abi_tests();
  run_style_values_abi_tests();
  const int failures = UNITY_END();
#if defined(__EMSCRIPTEN__) && defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  // Returning is not enough to exit here. main() runs on a pthread under
  // -sPROXY_TO_PTHREAD, and requesting a WebGPU device suspends that thread
  // through Asyncify, which leaves emscripten unable to carry the entry point
  // across the suspension: it decides whether to exit before the entry really
  // returns, and skips the exit because a keepalive is held at that moment. The
  // suite would then print its summary and hang instead of reporting a status.
  // Forcing the exit is what turns a finished run into a result. See the note
  // on MLN_FFI_TEST_JOIN_ON_FLAG in test_support.c for the whole mechanism.
  // Forcing the exit skips the usual teardown, so anything still buffered has
  // to go out first or the run loses the report it just produced.
  fflush(NULL);
  emscripten_force_exit(failures);
#endif
  return failures;
}
