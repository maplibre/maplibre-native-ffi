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

/// Opens the scope one render-loop event runs inside. Metal returns an
/// autorelease pool that collects the event's presentation objects; the other
/// backends return null.
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

render_session* render_target_session(render_target* target);

/// Starts the session resize or target replacement a new viewport needs and
/// returns without waiting for it.
[[nodiscard]] app_error render_target_resize(
  render_target* target, viewport current_viewport
);

/// Services caller-driver work, then shows any replacement texture a rendered
/// frame has drawn into.
[[nodiscard]] app_error render_target_service(render_target* target);

/// Shows the newest rendered frame: the texture modes sample it into the
/// window, switching to a replacement texture once a frame has rendered into
/// it, and a surface target already presented it. Reports false when no
/// frame reached the window.
[[nodiscard]] app_error render_target_present(
  render_target* target, viewport current_viewport, bool* out_presented
);

#endif  // C_MAP_RENDER_RENDER_H
