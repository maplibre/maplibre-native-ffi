// The backend-agnostic slice of the render target: the attached session and
// the extent a viewport maps to.

#ifndef C_MAP_RENDER_TARGET_H
#define C_MAP_RENDER_TARGET_H

#include <maplibre_native_c.h>
#include <stdatomic.h>

#include "types.h"

typedef enum render_session_kind : uint8_t {
  RENDER_SESSION_NONE,
  RENDER_SESSION_TEXTURE,
  RENDER_SESSION_SURFACE,
} render_session_kind;

typedef struct render_completion {
  atomic_bool completed;
  mln_status status;
  mln_completion descriptor;
} render_completion;

typedef struct render_session {
  render_session_kind kind;
  mln_render_session handle;
  /// The map this session renders. Target replacement changes only the
  /// graphics resource, so those paths carry the extent to the map directly.
  mln_map map;
  uint64_t next_frame_token;
  /// Storage for the one ordered submission that can be outstanding: attach,
  /// resize, target replacement, or detach. The core copies the descriptor and
  /// runs it from the driver, so the storage outlives the submitting frame.
  render_completion pending;
  bool pending_active;
  app_error pending_error;
  const char* pending_message;
} render_session;

/// One frame demand's outcome: whether the session rendered the demand, and
/// whether the map asked for another frame while it rendered this one.
typedef struct render_frame_outcome {
  bool rendered;
  bool needs_repaint;
} render_frame_outcome;

/// Arms the session's one completion slot, which at most one ordered
/// submission uses at a time. Pass the returned descriptor to the C API, then
/// report the status it returned to render_session_submitted().
mln_completion* render_session_begin_submission(
  render_session* session, app_error error, const char* message
);

/// Records an ordered submission's synchronous status. A non-OK status logs the
/// failure and leaves the slot free.
[[nodiscard]] app_error render_session_submitted(
  render_session* session, mln_status status
);

/// Services caller-driver work and reports whether the outstanding submission
/// is still pending. A submission that failed reports its error.
[[nodiscard]] app_error render_session_poll(
  render_session* session, bool* out_pending
);

/// Services caller-driver work until the outstanding submission completes.
/// Startup and shutdown block here; the render loop polls instead.
[[nodiscard]] app_error render_session_await(render_session* session);

void render_session_close(render_session* session);

/// Starts the session resize that carries the new logical extent to the map.
/// The render loop drives it to completion through render_session_poll().
[[nodiscard]] app_error render_session_resize(
  render_session* session, viewport current_viewport
);

/// Carries the new logical extent to the map on the paths where the session
/// cannot: a caller-owned texture the host sizes, and a replaced surface
/// target. Both change only the graphics resource.
[[nodiscard]] app_error render_session_resize_map(
  render_session* session, viewport current_viewport
);

/// Submits one frame demand, services caller-driver work, and drains the
/// result the demand's token identifies.
[[nodiscard]] app_error render_session_render_update(
  render_session* session, render_frame_outcome* out_outcome
);

/// Reads the producer synchronization an acquired frame carries, reporting a
/// backend-draw failure for anything this example cannot wait on.
[[nodiscard]] app_error render_session_require_cpu_complete_producer(
  mln_acquired_frame frame, const char* message
);

mln_render_session_attach_options render_session_attach_options(void);

mln_render_target_extent render_target_extent(viewport current_viewport);

#endif  // C_MAP_RENDER_TARGET_H
