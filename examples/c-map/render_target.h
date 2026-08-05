// The backend-agnostic slice of the render target: the attached session and
// the extent a viewport maps to.

#ifndef C_MAP_RENDER_TARGET_H
#define C_MAP_RENDER_TARGET_H

#include <maplibre_native_c.h>

#include "types.h"

typedef enum render_session_kind : uint8_t {
  RENDER_SESSION_NONE,
  RENDER_SESSION_TEXTURE,
  RENDER_SESSION_SURFACE,
} render_session_kind;

typedef struct render_session {
  render_session_kind kind;
  mln_render_session handle;
} render_session;

void render_session_close(render_session* session);

[[nodiscard]] app_error render_session_resize(
  render_session* session, viewport current_viewport
);

[[nodiscard]] app_error render_session_render_update(
  render_session* session, bool* out_rendered
);

mln_render_target_extent render_target_extent(viewport current_viewport);

#endif  // C_MAP_RENDER_TARGET_H
