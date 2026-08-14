#include "render_target.h"

#include "diagnostics.h"

void render_session_close(render_session* session) {
  if (session->kind != RENDER_SESSION_NONE) {
    mln_render_session_destroy(session->handle);
  }
  *session = (render_session){.handle = MLN_HANDLE_NULL};
}

app_error render_session_resize(
  render_session* session, viewport current_viewport
) {
  if (session->kind == RENDER_SESSION_NONE) {
    return APP_ERROR_TEXTURE_RESIZE_FAILED;
  }
  const bool is_surface = session->kind == RENDER_SESSION_SURFACE;
  const mln_status status = mln_render_session_resize(
    session->handle, current_viewport.logical_width,
    current_viewport.logical_height, current_viewport.scale_factor
  );
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status(
      is_surface ? "surface resize failed" : "texture resize failed", status
    );
    return is_surface ? APP_ERROR_SURFACE_RESIZE_FAILED
                      : APP_ERROR_TEXTURE_RESIZE_FAILED;
  }
  return APP_OK;
}

app_error render_session_render_update(
  render_session* session, bool* out_rendered
) {
  *out_rendered = false;
  if (session->kind == RENDER_SESSION_NONE) {
    return APP_OK;
  }
  const bool is_surface = session->kind == RENDER_SESSION_SURFACE;
  mln_render_result result = MLN_RENDER_RESULT_RENDERED;
  bool needs_repaint = false;
  const mln_status status =
    mln_render_session_render_update(session->handle, &result, &needs_repaint);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status(
      is_surface ? "surface render failed" : "texture render failed", status
    );
    return is_surface ? APP_ERROR_SURFACE_RENDER_FAILED
                      : APP_ERROR_TEXTURE_RENDER_FAILED;
  }
  *out_rendered = result == MLN_RENDER_RESULT_RENDERED;
  return APP_OK;
}

mln_render_target_extent render_target_extent(viewport current_viewport) {
  return (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = current_viewport.logical_width,
    .height = current_viewport.logical_height,
    .scale_factor = current_viewport.scale_factor,
  };
}
