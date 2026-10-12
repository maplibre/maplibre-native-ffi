#ifndef MLN_NATIVE_TESTS_CAMERA_H
#define MLN_NATIVE_TESTS_CAMERA_H

// Camera helpers: a probe that records the end handler of one camera command.

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "maplibre_native_c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mln_test_transition_end mln_test_transition_end;

// Runs inside the end callback, on the runtime worker, before the probe
// records the call.
typedef void (*mln_test_transition_end_hook)(
  mln_test_transition_end* probe, const mln_camera_transition_end* end
);

// Records each call of an end handler and of its release, and pulses on both.
// The probe must outlive the release.
struct mln_test_transition_end {
  atomic_int calls;
  atomic_int releases;
  // Set when the release ran before any call.
  atomic_bool released_first;
  _Atomic uint32_t outcome;
  _Atomic uint64_t generation;
  _Atomic uint32_t end_size;
  mln_test_transition_end_hook hook;
  void* context;
};

void mln_test_transition_end_init(mln_test_transition_end* probe);
// An enabled handler that records into `probe`.
mln_camera_transition_handler mln_test_transition_end_handler(
  mln_test_transition_end* probe
);
// Waits within the default deadline until the handler ran and released once,
// and reports whether it did.
bool mln_test_wait_transition_end(mln_test_transition_end* probe);

#ifdef __cplusplus
}
#endif

#endif
