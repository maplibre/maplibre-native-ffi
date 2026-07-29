#ifndef MLN_C_API_TEST_SUPPORT_H
#define MLN_C_API_TEST_SUPPORT_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"

typedef struct mln_test_render_fixture {
  mln_render_session* session;
  void* backend_state;
} mln_test_render_fixture;

// Opaque host thread used by tests that need a second owner thread.
typedef struct mln_test_thread mln_test_thread;

mln_test_thread* mln_test_thread_start(void (*entry)(void*), void* argument);
void mln_test_thread_join(mln_test_thread* thread);
void mln_test_sleep_milliseconds(unsigned int milliseconds);
// Monotonic milliseconds, for tests that assert a pump returned promptly rather
// than sat out its timeout.
uint64_t mln_test_monotonic_milliseconds(void);

// These helpers track what they create per calling thread so the suite can
// reclaim handles a test left behind. The matching destroy helpers untrack.
mln_runtime* mln_test_create_runtime(void);
mln_map* mln_test_create_map(mln_runtime* runtime);
mln_map* mln_test_create_map_with_options(
  mln_runtime* runtime, const mln_map_options* options
);
void mln_test_destroy_runtime(mln_runtime* runtime);
void mln_test_destroy_map(mln_map* map);
void mln_test_sleep_millisecond(void);

bool mln_test_render_fixture_create(
  mln_map* map, mln_test_render_fixture* fixture
);
void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture);

// Pumps the runtime until `flag` is set or the deadline passes. Returns whether
// the flag was observed. Use for waiting on work another thread reports.
bool mln_test_pump_until(mln_runtime* runtime, atomic_bool* flag);

// Destroys everything this thread still has tracked, render session first, then
// map, then runtime, and reports whether it reclaimed anything. Safe to call
// after an aborted test: it reports through its return value rather than
// through assertions, which would longjmp out of teardown.
bool mln_test_reclaim_thread_resources(void);

#endif
