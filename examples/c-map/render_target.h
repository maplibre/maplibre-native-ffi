// The backend-agnostic slice of the render target: the attached session, its
// frame demands and results, and the extent a viewport maps to.

#ifndef C_MAP_RENDER_TARGET_H
#define C_MAP_RENDER_TARGET_H

#include <maplibre_native_c.h>
#include <stdatomic.h>

#include "events.h"
#include "types.h"

typedef struct render_session {
  mln_render_session handle;
  /// The map this session renders. Target replacement changes only the
  /// graphics resource, so those paths carry the extent to the map directly.
  mln_map map;
  /// Whether demands ask the driver to present, as a surface target does.
  bool presents;
  uint64_t next_frame_token;
  /// The newest demand token with a rendered result.
  uint64_t rendered_token;
} render_session;

/// What one drain of the frame-result queue found.
typedef struct frame_results {
  /// A demand rendered a frame.
  bool rendered;
  /// The map asked for another frame while it rendered one.
  bool needs_repaint;
  /// The target could not produce a frame, so the loop retries later.
  bool target_not_ready;
} frame_results;

/// Caller-driver attach options whose wakes post APP_EVENT_FRAME_RESULTS and
/// APP_EVENT_DRIVER_WORK.
mln_render_session_attach_options render_session_attach_options(void);

/// Finishes an attach call: services driver work until the attachment
/// completes, and abandons the session when it fails. Pass the completion the
/// call took and the status and diagnostic it returned.
[[nodiscard]] app_error render_session_finish_attach(
  render_session* session, mln_render_session handle, mln_map map,
  bool presents, awaited_completion* attached, mln_status status,
  const mln_diagnostic* diagnostic
);

/// Detaches through the driver, abandoning instead when that fails, then
/// destroys the session. Safe to call with no session attached.
void render_session_close(render_session* session);

/// Services every queued caller-driver item on the graphics thread.
[[nodiscard]] app_error render_session_service(render_session* session);

/// Demands a frame. A forced demand renders even without a newer map update,
/// which a retry after an undrawn frame needs.
[[nodiscard]] app_error render_session_request_frame(
  render_session* session, bool force
);

/// Drains every queued frame result.
[[nodiscard]] app_error render_session_drain_results(
  render_session* session, frame_results* out_results
);

/// Starts the session resize that carries the new logical extent to the map.
[[nodiscard]] app_error render_session_resize(
  render_session* session, viewport current_viewport
);

/// Carries the new logical extent to the map on the paths where the session
/// cannot: a caller-owned texture the host sizes, and a replaced surface
/// target. Both change only the graphics resource.
[[nodiscard]] app_error render_session_resize_map(
  render_session* session, viewport current_viewport
);

/// Acquires the newest rendered frame, releasing any older one unsampled.
/// Leaves *out_frame null when the ring holds none.
[[nodiscard]] app_error render_session_acquire_newest(
  render_session* session, mln_acquired_frame* out_frame
);

/// Releases a sampled frame. The compositor waits for its GPU work before
/// returning, so CPU-complete consumer synchronization is accurate.
void render_session_release_frame(mln_acquired_frame* frame);

/// Reads the producer synchronization an acquired frame carries, reporting a
/// backend-draw failure for anything this example cannot wait on.
[[nodiscard]] app_error render_session_require_cpu_complete_producer(
  mln_acquired_frame frame, const char* message
);

/// The caller-owned textures a borrowed-texture target hands over on resize,
/// oldest first. The session renders into a texture until its replacement
/// completes, so each outgoing texture stays alive until then.
typedef struct texture_replacement texture_replacement;
typedef struct texture_replacements {
  texture_replacement* oldest;
  texture_replacement* newest;
} texture_replacements;

/// Allocates the entry for texture and writes the completion to submit with
/// its set_target call. Returns null when allocation fails.
texture_replacement* texture_replacement_begin(
  void* texture, mln_completion* out_completion
);

/// Queues an entry whose set_target call returned status, or frees it when the
/// call failed.
void texture_replacements_queue(
  texture_replacements* replacements, texture_replacement* replacement,
  mln_status status
);

/// Takes the oldest replacement that a rendered frame has drawn into, writing
/// its texture, or null when none has. A completed replacement holds no frame
/// yet, so the first call that finds it demands one. A failed replacement
/// reports its error.
[[nodiscard]] app_error texture_replacements_take_shown(
  texture_replacements* replacements, render_session* session,
  void** out_texture
);

/// Takes the oldest replacement whatever its state, for teardown after the
/// session detached, writing null once none remains.
void texture_replacements_take_any(
  texture_replacements* replacements, void** out_texture
);

mln_render_target_extent render_target_extent(viewport current_viewport);

#endif  // C_MAP_RENDER_TARGET_H
