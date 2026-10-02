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
  mln_render_driver_kind driver;
  /// Whether demands ask the driver to present, as a surface target does.
  bool presents;
  /// Whether the session and the host take turns with one texture, as a
  /// core-worker borrowed texture does. The session owns it from a demand
  /// until its result, and the host owns it until the compositor's reads
  /// finish, so at most one demand is outstanding.
  bool takes_turns;
  bool demand_outstanding;
  /// A demand that arrived while one was outstanding, sent once the
  /// compositor is done. A forced one renders without a newer map update.
  bool demand_wanted;
  bool wanted_forced;
  uint64_t next_frame_token;
  /// The newest demand token with a rendered result.
  uint64_t rendered_token;
} render_session;

/// What one drain of the frame-result queue found.
typedef struct frame_results {
  /// The drain found at least one result.
  bool any;
  /// A demand rendered a frame.
  bool rendered;
  /// The map asked for another frame while it rendered one.
  bool needs_repaint;
  /// The target could not produce a frame, so the loop retries later.
  bool target_not_ready;
} frame_results;

const char* render_driver_label(mln_render_driver_kind driver);

/// Attach options for driver whose wakes post APP_EVENT_FRAME_RESULTS and, for
/// a caller driver, APP_EVENT_DRIVER_WORK.
mln_render_session_attach_options render_session_attach_options(
  mln_render_driver_kind driver
);

/// Finishes an attach call: awaits the attachment, servicing a caller driver
/// meanwhile, and abandons the session when it fails. Pass the options and
/// completion the call took, and the status and diagnostic it returned.
[[nodiscard]] app_error render_session_finish_attach(
  render_session* session, mln_render_session handle, mln_map map,
  const mln_render_session_attach_options* options, render_target_mode mode,
  awaited_completion* attached, mln_status status,
  const mln_diagnostic* diagnostic
);

/// Detaches, abandoning instead when that fails, then destroys the session.
/// Safe to call with no session attached.
void render_session_close(render_session* session);

/// Services every queued caller-driver item on the graphics thread.
[[nodiscard]] app_error render_session_service(render_session* session);

/// Demands a frame, and writes the token whose result shows it when
/// out_token is not null. A forced demand renders even without a newer map
/// update, which a retry after an undrawn frame needs. While a turn-taking
/// session has a demand outstanding, the demand waits for
/// render_session_compositor_done().
[[nodiscard]] app_error render_session_request_frame(
  render_session* session, bool force, uint64_t* out_token
);

/// Drains every queued frame result.
[[nodiscard]] app_error render_session_drain_results(
  render_session* session, frame_results* out_results
);

/// Ends the host's turn with a turn-taking session's texture after a drain
/// that found results, sending any demand that waited for it.
[[nodiscard]] app_error render_session_compositor_done(render_session* session);

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

/// Replaces *held with the newest rendered frame, releasing every older one,
/// and reports whether it found one. The compositor waits for its GPU work
/// before returning, so the held frame's reads are done by then.
[[nodiscard]] app_error render_session_acquire_newest(
  render_session* session, mln_acquired_frame* held, bool* out_acquired
);

/// Releases a sampled frame with CPU-complete synchronization. Safe to call
/// with a null frame.
void render_session_release_frame(mln_acquired_frame* frame);

/// Reads the producer synchronization an acquired frame carries, reporting a
/// backend-draw failure for anything this example cannot wait on.
[[nodiscard]] app_error render_session_require_cpu_complete_producer(
  mln_acquired_frame frame, const char* message
);

/// The caller-owned textures a borrowed-texture target hands over on resize,
/// oldest first. The session renders into a texture until its replacement
/// completes, so each outgoing texture stays alive until then. Each
/// completion posts APP_EVENT_TARGET_REPLACED.
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
