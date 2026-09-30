#ifndef MLN_NATIVE_TESTS_FRAMES_H
#define MLN_NATIVE_TESTS_FRAMES_H

// Frame demands and their results through the render fixture, for the render
// suites. The helpers that return a value assert, so a failure ends the case.

#include <stddef.h>
#include <stdint.h>

#include "maplibre_native_c.h"
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

// Loads the empty style and waits for the runtime to apply it, so a demand
// has an update to render.
void mln_test_render_prepare_map(mln_runtime runtime, mln_map map);

// Requests a forced frame, which renders the latest update whether or not it
// is new, with `token` as both its token and its coalescing boundary so it
// never supersedes an earlier demand.
void mln_test_render_request_forced(
  const mln_test_render_fixture* fixture, uint64_t token
);

// Services the fixture until no demand is pending and one drain yields at
// least `minimum` results, and returns that batch. The caller releases it.
mln_render_frame_batch mln_test_render_wait_for_results(
  const mln_test_render_fixture* fixture, size_t minimum
);

// Copies record `index` of `batch`.
mln_render_frame_result mln_test_render_batch_result(
  mln_render_frame_batch batch, size_t index
);

// Requests a forced frame, waits for it to render, and acquires it.
mln_acquired_frame mln_test_render_and_acquire(
  const mln_test_render_fixture* fixture, uint64_t token
);

#ifdef __cplusplus
}
#endif

#endif
