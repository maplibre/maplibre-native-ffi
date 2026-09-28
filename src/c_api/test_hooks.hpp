#ifndef MLN_C_API_TEST_HOOKS_HPP
#define MLN_C_API_TEST_HOOKS_HPP

#include <atomic>

#include "maplibre_native_c/base.h"
#include "maplibre_native_c/completion.h"

// The C ABI suite links the shipped shared library so it exercises the export
// boundary hosts link against. Internal concurrency probes cross that boundary
// here, with C linkage and an mln_ name that the library's export list matches.
// src/c_api/test_hooks.cpp compiles them only when MLN_FFI_ENABLE_TEST_HOOKS is
// defined, which only a test build sets, so a packaged artifact carries none.
extern "C" {

// Exercises mln::core::Completion end to end: inline resolution before
// acceptance, rejection, abandonment reported as MLN_STATUS_CANCELLED, and a
// repeated accept-against-resolve race. Returns the clause that failed, or null
// when every one held.
MLN_API const char* mln_test_hook_completion_contract(void);

// Occupies the session's driver until *release is set, publishing *entered once
// it runs. Returns MLN_STATUS_INVALID_ARGUMENT when either pointer is null, and
// otherwise the status of enqueuing the driver operation.
MLN_API mln_status mln_test_hook_enqueue_blocking_render_operation(
  mln_render_session session, std::atomic_bool* entered,
  const std::atomic_bool* release, const mln_completion* completion
);

// Keeps the operation pending after its run-loop callback returns. The test
// consumes its opaque reference through complete_runtime_operation().
MLN_API mln_status mln_test_hook_enqueue_pending_runtime_operation(
  mln_runtime runtime, std::atomic_bool* entered, void** out_operation,
  const mln_completion* completion
);
MLN_API void mln_test_hook_complete_runtime_operation(void* operation);

// Parks physical map cleanup after the public map release completes.
MLN_API mln_status mln_test_hook_block_map_cleanup(
  mln_map map, std::atomic_bool* entered, const std::atomic_bool* release
);

}  // extern "C"

#endif  // MLN_C_API_TEST_HOOKS_HPP
