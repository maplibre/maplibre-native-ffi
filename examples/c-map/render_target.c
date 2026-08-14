#include "render_target.h"

#include "diagnostics.h"

static app_error operation_failure(
  app_error error, const char* message, mln_status status
) {
  diagnostics_log_status(message, status);
  return error;
}

app_error render_session_complete_operation(
  render_session* session, mln_operation operation, app_error error,
  const char* message
) {
  bool completed = false;
  mln_status status = MLN_STATUS_OK;
  while (!completed) {
    size_t serviced = 0;
    status = mln_render_session_service_driver_work(
      session->handle, SIZE_MAX, &serviced
    );
    if (status != MLN_STATUS_OK) {
      mln_operation_release(operation);
      return operation_failure(error, message, status);
    }
    status = mln_operation_poll(operation, &completed);
    if (status != MLN_STATUS_OK) {
      mln_operation_release(operation);
      return operation_failure(error, message, status);
    }
    if (!completed && serviced == 0) {
      status = mln_operation_wait(operation, 1, &completed);
      if (status != MLN_STATUS_OK) {
        mln_operation_release(operation);
        return operation_failure(error, message, status);
      }
    }
  }
  mln_status terminal = MLN_STATUS_OK;
  status = mln_operation_get_status(operation, &terminal);
  mln_operation_release(operation);
  if (status != MLN_STATUS_OK || terminal != MLN_STATUS_OK) {
    return operation_failure(
      error, message, status != MLN_STATUS_OK ? status : terminal
    );
  }
  return APP_OK;
}

mln_render_session_attach_options render_session_attach_options(void) {
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  options.requested_texture_ring_depth = 2;
  return options;
}

void render_session_close(render_session* session) {
  if (session->kind != RENDER_SESSION_NONE) {
    mln_operation operation = MLN_HANDLE_NULL;
    const mln_status status =
      mln_render_session_detach_start(session->handle, &operation);
    if (status == MLN_STATUS_OK) {
      const app_error detached = render_session_complete_operation(
        session, operation, APP_ERROR_BACKEND_SETUP_FAILED,
        "render session detach failed"
      );
      if (detached != APP_OK) {
        mln_render_abandon_result result = {.size = sizeof(result)};
        (void)mln_render_session_abandon(session->handle, &result);
      }
    } else {
      mln_render_abandon_result result = {.size = sizeof(result)};
      (void)mln_render_session_abandon(session->handle, &result);
    }
    (void)mln_render_session_destroy(session->handle);
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
  const mln_render_target_extent extent =
    render_target_extent(current_viewport);
  mln_operation operation = MLN_HANDLE_NULL;
  const mln_status status =
    mln_render_session_resize_start(session->handle, &extent, &operation);
  const app_error error = is_surface ? APP_ERROR_SURFACE_RESIZE_FAILED
                                     : APP_ERROR_TEXTURE_RESIZE_FAILED;
  const char* message =
    is_surface ? "surface resize failed" : "texture resize failed";
  if (status != MLN_STATUS_OK) {
    return operation_failure(error, message, status);
  }
  return render_session_complete_operation(session, operation, error, message);
}

app_error render_session_render_update(
  render_session* session, bool* out_rendered
) {
  *out_rendered = false;
  if (session->kind == RENDER_SESSION_NONE) {
    return APP_OK;
  }
  const bool is_surface = session->kind == RENDER_SESSION_SURFACE;
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags |= is_surface ? MLN_FRAME_DEMAND_PRESENT : 0;
  demand.token = ++session->next_frame_token;
  mln_status status =
    mln_render_session_request_frame(session->handle, &demand);
  if (status != MLN_STATUS_OK) {
    return operation_failure(
      is_surface ? APP_ERROR_SURFACE_RENDER_FAILED
                 : APP_ERROR_TEXTURE_RENDER_FAILED,
      "frame demand failed", status
    );
  }
  size_t serviced = 0;
  status = mln_render_session_service_driver_work(
    session->handle, SIZE_MAX, &serviced
  );
  if (status != MLN_STATUS_OK) {
    return operation_failure(
      is_surface ? APP_ERROR_SURFACE_RENDER_FAILED
                 : APP_ERROR_TEXTURE_RENDER_FAILED,
      "driver service failed", status
    );
  }
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  status = mln_render_session_drain_frame_results(session->handle, 0, &batch);
  if (status == MLN_STATUS_NOT_READY) {
    return APP_OK;
  }
  if (status != MLN_STATUS_OK) {
    return operation_failure(
      is_surface ? APP_ERROR_SURFACE_RENDER_FAILED
                 : APP_ERROR_TEXTURE_RENDER_FAILED,
      "frame result drain failed", status
    );
  }
  size_t count = 0;
  status = mln_render_frame_batch_count(batch, &count);
  for (size_t i = 0; status == MLN_STATUS_OK && i < count; ++i) {
    mln_render_frame_result result = {.size = sizeof(result)};
    status = mln_render_frame_batch_get(batch, i, &result);
    if (status == MLN_STATUS_OK && result.token == demand.token) {
      *out_rendered = result.disposition == MLN_RENDER_RESULT_RENDERED;
    }
  }
  mln_render_frame_batch_release(batch);
  if (status != MLN_STATUS_OK) {
    return operation_failure(
      is_surface ? APP_ERROR_SURFACE_RENDER_FAILED
                 : APP_ERROR_TEXTURE_RENDER_FAILED,
      "frame result read failed", status
    );
  }
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
