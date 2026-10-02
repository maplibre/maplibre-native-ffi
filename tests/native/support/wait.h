#ifndef MLN_NATIVE_TESTS_WAIT_H
#define MLN_NATIVE_TESTS_WAIT_H

// The waits the native suites use. Every wait blocks on a signal and gives up
// at a deadline, so a case never decides pass or fail by how long it slept.
//
// The signal is the pulse: one process-wide counter that every setter in these
// helpers bumps, along with the wakes and completions the fixtures install. A
// waiter re-checks its condition on each pulse. It also re-checks every few
// milliseconds, which covers state the library publishes without a wake.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"

#ifdef __cplusplus
extern "C" {
#endif

// A point in time a wait gives up at. The default is 10 s times the timeout
// scale, the budget of any single wait.
typedef struct mln_test_deadline {
  uint64_t expires_at_milliseconds;
} mln_test_deadline;

mln_test_deadline mln_test_deadline_default(void);
// `milliseconds` times the timeout scale from now.
mln_test_deadline mln_test_deadline_after(uint64_t milliseconds);
bool mln_test_deadline_passed(mln_test_deadline deadline);

// Wakes every waiter so it re-checks its condition. Safe to call from any
// thread, including inside library callbacks and wakes.
void mln_test_pulse(void);
// An mln_wake whose callback pulses, for options a fixture fills in.
mln_wake mln_test_pulse_wake(void);

// Blocks until `ready(context)` holds or the deadline passes, and reports
// whether it held. `what` names the condition for the hang watchdog.
bool mln_test_await(
  bool (*ready)(void* context), void* context, mln_test_deadline deadline,
  const char* what
);

// A flag one thread sets and another waits for.
void mln_test_flag_set(atomic_bool* flag);
// Waits for the flag within the default deadline.
bool mln_test_wait_for_flag(const atomic_bool* flag);
bool mln_test_wait_for_flag_until(
  const atomic_bool* flag, mln_test_deadline deadline
);

// Waits until `counter` reaches at least `target`. The thread that counts must
// pulse, or the wait falls back to re-checking.
bool mln_test_wait_for_count(const atomic_int* counter, int target);

// Parks whichever thread enters it until the test releases it: a runtime
// worker inside a completion, a MapLibre thread inside a provider callback. A
// parked thread gives up at the default deadline, so a failing case cannot
// strand it.
typedef struct mln_test_gate {
  atomic_bool entered;
  atomic_bool released;
} mln_test_gate;

void mln_test_gate_init(mln_test_gate* gate);
// Called on the thread to park.
void mln_test_gate_park(mln_test_gate* gate);
bool mln_test_gate_wait_entered(mln_test_gate* gate);
void mln_test_gate_release(mln_test_gate* gate);
// A completion that parks the thread that delivers it on `gate`. That is the
// submitting thread when the work finishes before the submission returns, so
// the completion cannot be relied on to park a library worker. The gate must
// outlive the delivery.
mln_completion mln_test_gate_completion(mln_test_gate* gate);

// Records one completion delivery and its release for a test to inspect.
typedef struct mln_test_completion {
  mln_completion descriptor;
  void* state;
} mln_test_completion;

// Submits a command through `expression` and asserts its terminal status. The
// macro declares the `completion` the expression must pass, so an expression
// that names anything else does not compile.
#define MLN_TEST_AWAIT_COMMAND(expected_status, expression)                   \
  do {                                                                        \
    mln_test_completion completion = mln_test_completion_default(0);          \
    TEST_ASSERT_EQUAL_INT_MESSAGE(MLN_STATUS_OK, (expression), #expression);  \
    TEST_ASSERT_EQUAL_INT_MESSAGE(                                            \
      (expected_status), mln_test_completion_finish(&completion), #expression \
    );                                                                        \
    mln_test_completion_destroy(&completion);                                 \
  } while (false)
// The same, expecting the command to succeed.
#define MLN_TEST_AWAIT_OK(...) \
  MLN_TEST_AWAIT_COMMAND(MLN_STATUS_OK, (__VA_ARGS__))

mln_test_completion mln_test_completion_default(size_t value_size);
mln_test_completion mln_test_completion_buffer_view(void);
mln_test_completion mln_test_completion_readback(void);
mln_completion mln_test_discard_completion(void);
void mln_test_completion_destroy(mln_test_completion* completion);
void mln_test_completion_reject(mln_test_completion* completion);
// Waits for the completion to be delivered and reports whether it arrived. A
// negative timeout_ms waits the default deadline; a positive one is scaled
// like every other deadline.
bool mln_test_completion_wait(
  mln_test_completion* completion, int64_t timeout_ms
);
mln_status mln_test_completion_finish(mln_test_completion* completion);
// Waits for the completion, destroys it, and reports its terminal status.
mln_status mln_test_completion_settle(mln_test_completion* completion);
// The same, copying value_size bytes of the completion's value into out_value
// first. Reports MLN_STATUS_NATIVE_ERROR when the copy fails.
mln_status mln_test_completion_finish_value(
  mln_test_completion* completion, void* out_value, size_t value_size
);
// The same for a query that may deliver no value, such as a read of a missing
// style entity: `*out_found` reports whether a value arrived, and only then is
// it copied.
mln_status mln_test_completion_finish_optional(
  mln_test_completion* completion, void* out_value, size_t value_size,
  bool* out_found
);
bool mln_test_completion_poll(mln_test_completion* completion);
mln_status mln_test_completion_status(mln_test_completion* completion);
uint32_t mln_test_completion_disposition(mln_test_completion* completion);
uint64_t mln_test_completion_generation(mln_test_completion* completion);
const char* mln_test_completion_diagnostic(mln_test_completion* completion);
size_t mln_test_completion_value_count(mln_test_completion* completion);
bool mln_test_completion_copy_value(
  mln_test_completion* completion, void* out_value, size_t value_size
);

#ifdef __cplusplus
}
#endif

#endif
