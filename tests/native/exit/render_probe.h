// The render-session exit program, shared by the backends that give a core
// worker a session-owned texture. Each backend's file defines the attach and
// calls probe_exit_while_rendering() from main.

#ifndef MLN_NATIVE_EXIT_RENDER_PROBE_H
#define MLN_NATIVE_EXIT_RENDER_PROBE_H

#include "mln_test_graphics.h"
#include "probe.h"

// Starts attaching a session-owned texture on `map` for the graphics context.
typedef mln_status (*probe_attach_fn)(
  mln_map map, const mln_test_graphics_context* context,
  const mln_render_session_attach_options* options,
  mln_render_session* out_session, const mln_completion* completion
);

static const mln_render_target_extent probe_extent = {
  .size = sizeof(mln_render_target_extent),
  .width = 256,
  .height = 256,
  .scale_factor = 1.0,
};

static inline void probe_ignore_frame_wake(void* user_data) { (void)user_data; }

// Leaves a runtime, a map loading the probe's style, and a core-worker render
// session with frame demands queued, all live, and returns the exit status.
static inline int probe_exit_while_rendering(
  uint32_t graphics_backend, probe_attach_fn attach
) {
  probe_start();
  const mln_runtime runtime = probe_create_runtime();
  const mln_map map = probe_create_map(runtime);

  mln_test_graphics* graphics = mln_test_graphics_create(graphics_backend);
  mln_test_graphics_context context = {0};
  if (graphics == NULL || !mln_test_graphics_get_context(graphics, &context)) {
    (void)fprintf(
      stderr, "no graphics context: %s\n", mln_test_graphics_last_error()
    );
    return 1;
  }
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CORE_WORKER;
  options.frame_wake =
    (mln_wake){.size = sizeof(mln_wake), .callback = probe_ignore_frame_wake};
  mln_render_session session = MLN_HANDLE_NULL;
  PROBE_AWAIT(
    "attaching the render session",
    attach(map, &context, &options, &session, &completion)
  );

  // The worker renders these while the process exits, and the style is still
  // loading, so the renderer and the tile workers are busy too.
  for (uint64_t token = 1; token <= 8; token += 1) {
    mln_frame_demand demand = mln_frame_demand_default();
    demand.token = token;
    demand.coalescing_boundary = token;
    probe_require(
      mln_render_session_request_frame(session, &demand, NULL),
      "requesting a frame"
    );
  }
  return 0;
}

#endif
