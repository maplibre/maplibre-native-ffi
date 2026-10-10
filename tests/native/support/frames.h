#ifndef MLN_NATIVE_TESTS_FRAMES_H
#define MLN_NATIVE_TESTS_FRAMES_H

// Frame demands and their results through the render fixture, for the render
// suites. The helpers that return a value assert, so a failure ends the case.
//
// A case that checks that something on the driver has not happened yet, such
// as a barrier that must still be pending, fences the driver first with a
// session maintenance command such as mln_render_session_reduce_memory_use().
// That command runs after every work item that the driver already holds.
// Reducing memory use makes the map publish an update, so a case whose demand
// waits for one fences with mln_render_session_dump_debug_logs() instead.

#include <stdbool.h>
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

// Renders each update that the map publishes, one frame per batch of updates,
// until the map reports idle and the session has rendered its latest update.
// After it returns, no update reaches a demand until the case publishes one.
// The case drains the runtime's events only through this call meanwhile.
// Returns whether a frame finished on the way, and copies the statistics of
// the latest one to `latest_stats` unless it is null.
bool mln_test_render_until_idle(
  mln_runtime runtime, const mln_test_render_fixture* fixture,
  mln_rendering_stats* latest_stats
);

// Requests a forced frame, waits for it to render, and acquires it.
mln_acquired_frame mln_test_render_and_acquire(
  const mln_test_render_fixture* fixture, uint64_t token
);

// Releases `*frame` with no consumer synchronization, and expects the release
// to consume the handle.
void mln_test_render_release_frame(mln_acquired_frame* frame);

// Reads the fixture's latest frame back. On MLN_STATUS_OK, writes its image
// info to `out_info` and copies up to `capacity` bytes of its tightly packed
// pixels into `out_pixels`, which may be null. Otherwise returns the
// readback's terminal status and writes nothing.
mln_status mln_test_render_read_back(
  const mln_test_render_fixture* fixture, mln_texture_image_info* out_info,
  uint8_t* out_pixels, size_t capacity
);

#ifdef __cplusplus
}
#endif

#endif
