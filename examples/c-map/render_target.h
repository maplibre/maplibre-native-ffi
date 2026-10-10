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
  uint64_t next_frame_token;
} render_session;

/// What one drain of the frame-result queue found.
typedef struct frame_results {
  /// A demand rendered a frame.
  bool rendered;
  /// The map asked for another frame while it rendered one.
  bool needs_repaint;
  /// The target could not produce a frame. The loop retries later, because
  /// this result does not cause a map-update event.
  bool target_not_ready;
} frame_results;

/// The slot count of a texture ring, owned or borrowed. The host holds the
/// newest frame until a newer one arrives, and the session never renders into
/// a held frame's texture, so it renders into the other one meanwhile.
enum { RING_DEPTH = 2 };

const char* render_driver_label(mln_render_driver_kind driver);

/// Attach options for driver whose wakes post APP_EVENT_FRAME_RESULTS and, for
/// a caller driver, APP_EVENT_DRIVER_WORK. An owned texture asks for a ring of
/// RING_DEPTH slots; a borrowed ring's depth is its texture count.
mln_render_session_attach_options render_session_attach_options(
  render_target_mode mode, mln_render_driver_kind driver
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

/// Whether an abandon kept graphics objects until the process exits. A kept
/// Vulkan object is a child of the host's device, and a kept swapchain of its
/// surface, so a Vulkan host then keeps those until the process exits too.
bool render_session_graphics_kept(void);

/// Services every queued caller-driver item on the graphics thread.
[[nodiscard]] app_error render_session_service(render_session* session);

/// Demands a frame. A forced demand renders even without a newer map update,
/// which a retry after a frame that missed the window needs.
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

/// The rings of caller-owned textures that a borrowed-texture target retires
/// on resize, oldest first. The session renders into a ring until the
/// replacement that retires it completes, so each retired ring stays alive
/// until then. Each completion posts APP_EVENT_TARGET_REPLACED.
typedef struct texture_replacement texture_replacement;
typedef struct texture_replacements {
  texture_replacement* oldest;
  texture_replacement* newest;
} texture_replacements;

/// Allocates the entry for a retired ring and writes the completion to submit
/// with the set_target call that retires it. Returns null when allocation
/// fails.
texture_replacement* texture_replacement_begin(
  void* retired, mln_completion* out_completion
);

/// Queues an entry whose set_target call returned status, or frees it when the
/// call failed.
void texture_replacements_queue(
  texture_replacements* replacements, texture_replacement* replacement,
  mln_status status
);

/// Takes the oldest retired ring whose replacement has completed, writing it,
/// or null when none has. A replacement publishes no map update, and a frame
/// rendered before it can no longer be acquired, so taking one demands a
/// forced frame for the new ring. A failed replacement reports its error.
[[nodiscard]] app_error texture_replacements_take_completed(
  texture_replacements* replacements, render_session* session,
  void** out_retired
);

/// Takes the oldest retired ring whatever its state, for teardown after the
/// session detached, writing null once none remains.
void texture_replacements_take_any(
  texture_replacements* replacements, void** out_retired
);

mln_logical_extent render_target_extent(viewport current_viewport);

#endif  // C_MAP_RENDER_TARGET_H
