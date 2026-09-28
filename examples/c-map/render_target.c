#include <stdio.h>

#include "render_target.h"

#include "diagnostics.h"
#include "map_state.h"
#include "util.h"

static app_error completion_failure(
  app_error error, const char* message, mln_status status
) {
  diagnostics_log_status(message, status);
  return error;
}

static void complete_render_work(
  void* user_data, const mln_completion_result* result
) {
  render_completion* completion = user_data;
  completion->status = result->status;
  atomic_store_explicit(&completion->completed, true, memory_order_release);
}

mln_completion* render_session_begin_submission(
  render_session* session, app_error error, const char* message
) {
  session->pending = (render_completion){
    .completed = false,
    .descriptor = {
      .size = sizeof(mln_completion),
      .callback = complete_render_work,
      .user_data = &session->pending,
    },
  };
  session->pending_error = error;
  session->pending_message = message;
  return &session->pending.descriptor;
}

app_error render_session_submitted(render_session* session, mln_status status) {
  if (status != MLN_STATUS_OK) {
    return completion_failure(
      session->pending_error, session->pending_message, status
    );
  }
  session->pending_active = true;
  return APP_OK;
}

app_error render_session_poll(render_session* session, bool* out_pending) {
  *out_pending = false;
  if (!session->pending_active) {
    return APP_OK;
  }
  size_t serviced = 0;
  const mln_status status =
    mln_render_session_service_driver_work(session->handle, 0, &serviced);
  if (status != MLN_STATUS_OK) {
    session->pending_active = false;
    return completion_failure(
      session->pending_error, session->pending_message, status
    );
  }
  if (!atomic_load_explicit(
        &session->pending.completed, memory_order_acquire
      )) {
    *out_pending = true;
    return APP_OK;
  }
  session->pending_active = false;
  if (session->pending.status != MLN_STATUS_OK) {
    return completion_failure(
      session->pending_error, session->pending_message, session->pending.status
    );
  }
  return APP_OK;
}

app_error render_session_await(render_session* session) {
  bool pending = true;
  while (pending) {
    MAP_TRY(render_session_poll(session, &pending));
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
  if (session->handle != MLN_HANDLE_NULL) {
    // The outstanding submission owns the completion slot the detach needs, so
    // it finishes first.
    bool abandon = render_session_await(session) != APP_OK;
    if (!abandon) {
      const mln_status status = mln_render_session_detach(
        session->handle, render_session_begin_submission(
                           session, APP_ERROR_BACKEND_SETUP_FAILED,
                           "render session detach failed"
                         )
      );
      abandon = render_session_submitted(session, status) != APP_OK ||
                render_session_await(session) != APP_OK;
    }
    if (abandon) {
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
  const mln_status status = mln_render_session_resize(
    session->handle, &extent,
    render_session_begin_submission(
      session,
      is_surface ? APP_ERROR_SURFACE_RESIZE_FAILED
                 : APP_ERROR_TEXTURE_RESIZE_FAILED,
      is_surface ? "surface resize failed" : "texture resize failed"
    )
  );
  return render_session_submitted(session, status);
}

app_error render_session_resize_map(
  render_session* session, viewport current_viewport
) {
  const mln_logical_extent extent = {
    .width = current_viewport.logical_width,
    .height = current_viewport.logical_height,
    .scale_factor = current_viewport.scale_factor,
  };
  const mln_status status =
    mln_map_resize(session->map, extent, map_state_discarded_completion());
  if (status != MLN_STATUS_OK) {
    return completion_failure(
      session->kind == RENDER_SESSION_SURFACE ? APP_ERROR_SURFACE_RESIZE_FAILED
                                              : APP_ERROR_TEXTURE_RESIZE_FAILED,
      "map resize failed", status
    );
  }
  return APP_OK;
}

app_error render_session_render_update(
  render_session* session, render_frame_outcome* out_outcome
) {
  *out_outcome = (render_frame_outcome){};
  if (session->kind == RENDER_SESSION_NONE) {
    return APP_OK;
  }
  const bool is_surface = session->kind == RENDER_SESSION_SURFACE;
  const app_error error = is_surface ? APP_ERROR_SURFACE_RENDER_FAILED
                                     : APP_ERROR_TEXTURE_RENDER_FAILED;
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags |= is_surface ? MLN_FRAME_DEMAND_PRESENT : 0;
  demand.token = ++session->next_frame_token;
  mln_status status =
    mln_render_session_request_frame(session->handle, &demand);
  if (status != MLN_STATUS_OK) {
    return completion_failure(error, "frame demand failed", status);
  }
  size_t serviced = 0;
  status =
    mln_render_session_service_driver_work(session->handle, 0, &serviced);
  if (status != MLN_STATUS_OK) {
    return completion_failure(error, "driver service failed", status);
  }
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  status = mln_render_session_drain_frame_results(session->handle, &batch);
  if (status == MLN_STATUS_NOT_READY) {
    return APP_OK;
  }
  if (status != MLN_STATUS_OK) {
    return completion_failure(error, "frame result drain failed", status);
  }
  size_t count = 0;
  status = mln_render_frame_batch_count(batch, &count);
  for (size_t i = 0; status == MLN_STATUS_OK && i < count; ++i) {
    mln_render_frame_result result = {.size = sizeof(result)};
    status = mln_render_frame_batch_get(batch, i, &result);
    if (status == MLN_STATUS_OK && result.token == demand.token) {
      out_outcome->rendered = result.disposition == MLN_RENDER_RESULT_RENDERED;
      out_outcome->needs_repaint = result.needs_repaint;
    }
  }
  mln_render_frame_batch_release(batch);
  if (status != MLN_STATUS_OK) {
    return completion_failure(error, "frame result read failed", status);
  }
  return APP_OK;
}

app_error render_session_require_cpu_complete_producer(
  mln_acquired_frame frame, const char* message
) {
  mln_gpu_sync sync = mln_gpu_sync_default();
  const mln_status status = mln_acquired_frame_get_producer_sync(frame, &sync);
  if (status != MLN_STATUS_OK) {
    return completion_failure(APP_ERROR_BACKEND_DRAW_FAILED, message, status);
  }
  if (sync.kind != MLN_GPU_SYNC_CPU_COMPLETE) {
    fprintf(
      stderr, "%s: producer synchronization kind %u is not CPU-complete\n",
      message, sync.kind
    );
    return APP_ERROR_BACKEND_DRAW_FAILED;
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
