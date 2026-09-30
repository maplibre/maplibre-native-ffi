#ifndef MLN_NATIVE_TESTS_HOOKS_H
#define MLN_NATIVE_TESTS_HOOKS_H

// Entry points below the public ABI: the library's test hooks, which it
// exports only when built with MLN_FFI_ENABLE_TEST_HOOKS, and the browser
// run-loop probes, which reach its C++ internals.

#include <stdatomic.h>
#include <stdbool.h>

#include "maplibre_native_c.h"

#ifdef __cplusplus
extern "C" {
#endif

// Exercises the completion state machine inside the library and returns the
// clause that failed, or null when every one held.
const char* mln_test_completion_contract(void);

mln_status mln_test_render_session_blocking_operation_create(
  mln_render_session session, atomic_bool* entered, const atomic_bool* release,
  const mln_completion* completion
);

mln_status mln_test_pending_runtime_operation(
  mln_runtime runtime, atomic_bool* entered, void** out_operation,
  const mln_completion* completion
);

void mln_test_complete_runtime_operation(void* operation);

mln_status mln_test_block_map_cleanup(
  mln_map map, atomic_bool* entered, const atomic_bool* release
);

#if defined(__EMSCRIPTEN__)
bool mln_test_browser_async_task_runs_after_clock_advance(void);
bool mln_test_browser_stop_allows_immediate_destruction(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
