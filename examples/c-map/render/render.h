// The uniform render-target interface. Exactly one backend translation unit is
// compiled per build, so the linker does the dispatch.

#ifndef C_MAP_RENDER_RENDER_H
#define C_MAP_RENDER_RENDER_H

#include <SDL3/SDL.h>
#include <maplibre_native_c.h>

#include "../render_target.h"
#include "../types.h"

typedef struct render_target render_target;

/// The mln_render_backend_flag bit the active backend requires the native
/// library to support.
uint32_t render_target_backend_flag(void);

/// Applies the SDL hints the active backend needs before SDL_Init.
void render_target_apply_sdl_hints(void);

/// Configures backend video state after SDL_Init and before window creation.
[[nodiscard]] app_error render_target_configure_video(void);

/// The SDL window flag the active backend's surface needs.
SDL_WindowFlags render_target_window_flags(void);

/// Opens the scope one render-loop iteration runs inside. Metal returns an
/// autorelease pool that collects the iteration's presentation objects; the
/// other backends return null.
void* render_target_frame_scope_open(void);

/// Closes a scope returned by render_target_frame_scope_open().
void render_target_frame_scope_close(void* scope);

/// Creates the graphics context and the mode's presentation resources on the
/// calling thread, which must be the render loop thread that owns the window.
[[nodiscard]] app_error render_target_init(
  render_target** out_target, SDL_Window* window, viewport current_viewport,
  render_target_mode mode
);

/// Attaches a render session to the live map on the render-loop thread, which
/// owns the graphics context the session drives.
[[nodiscard]] app_error render_target_attach(
  render_target* target, mln_map map, viewport current_viewport
);

/// Closes the session and releases every backend resource. Safe to call with
/// no session attached.
void render_target_deinit(render_target* target);

/// Starts the session resize or target replacement a new viewport needs and
/// returns without waiting. The render loop drives it through
/// render_target_poll_pending().
[[nodiscard]] app_error render_target_resize(
  render_target* target, viewport current_viewport
);

/// Services caller-driver work, releases anything a completed target
/// replacement retired, and reports whether an ordered submission is still
/// outstanding. The render loop holds frame demand back while one is.
[[nodiscard]] app_error render_target_poll_pending(
  render_target* target, bool* out_pending
);

/// Runs once per render loop iteration, before the render request is consumed:
/// fences, pacing, and deferred presentation cleanup.
[[nodiscard]] app_error render_target_finish_frame(render_target* target);

/// Consumes one render request: renders, composites when the mode needs it,
/// and presents. Reports the demand's outcome.
[[nodiscard]] app_error render_target_render_update(
  render_target* target, viewport current_viewport,
  render_frame_outcome* out_outcome
);

#endif  // C_MAP_RENDER_RENDER_H
