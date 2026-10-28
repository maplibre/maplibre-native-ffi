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

// Loads an empty style with no transition and waits for the runtime to apply
// it, so a demand has an update to render.
void mln_test_render_prepare_map(mln_map map);

// Requests a forced frame of the latest update with the supplied token.
// Every forced demand remains independent.
void mln_test_render_request_forced(
  const mln_test_render_fixture* fixture, uint64_t token
);

// Services the fixture until a drain yields results and returns that batch.
// The caller releases it.
mln_render_frame_batch mln_test_render_wait_for_results(
  const mln_test_render_fixture* fixture
);

// Services the fixture until `count` results have arrived, over as many drains
// as they take, and copies them to `out` in order.
void mln_test_render_wait_for_result_list(
  const mln_test_render_fixture* fixture, size_t count,
  mln_render_frame_result* out
);

// Drains frame results as mln_render_session_drain_frame_results() does, and
// records the drained batch's view for mln_test_render_batch_view().
mln_status mln_test_drain_frame_results(
  mln_render_session session, mln_render_frame_batch* out_batch,
  mln_diagnostic* out_diagnostic
);

// The borrowed view that the drain of `batch` returned, which stays valid
// until the batch's release. `batch` must come from
// mln_test_drain_frame_results().
mln_render_frame_batch_view mln_test_render_batch_view(
  mln_render_frame_batch batch
);

// Returns record `index` of `view`, stepping by its stride.
const mln_render_frame_result* mln_test_render_view_result(
  const mln_render_frame_batch_view* view, size_t index
);

// Copies record `index` of `batch`.
mln_render_frame_result mln_test_render_batch_result(
  mln_render_frame_batch batch, size_t index
);

// Renders each update that the map publishes, one frame per batch of updates,
// until a frame shows the map settled, as mln_render_frame_result.mode
// describes. After it returns, no update reaches a demand until the case
// publishes one. The case drains the runtime's events only through this call
// meanwhile, and the call discards them.
void mln_test_render_until_idle(
  mln_runtime runtime, mln_map map, const mln_test_render_fixture* fixture
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
