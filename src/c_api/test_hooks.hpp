#ifndef MLN_C_API_TEST_HOOKS_HPP
#define MLN_C_API_TEST_HOOKS_HPP

#include <atomic>

#include "maplibre_native_c/base.h"
#include "maplibre_native_c/completion.h"

// The C ABI suite links the shipped shared library so it exercises the export
// boundary hosts link against. The two things it needs that no public function
// reaches — the completion state machine and a driver it can park — cross that
// boundary here, with C linkage and an mln_ name the library's export list
// already matches. src/c_api/test_hooks.cpp compiles them only when
// MLN_FFI_ENABLE_TEST_HOOKS is defined, which only a test build sets, so a
// packaged artifact carries neither.
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

}  // extern "C"

#endif  // MLN_C_API_TEST_HOOKS_HPP
