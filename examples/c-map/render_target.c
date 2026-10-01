#include <stdio.h>
#include <stdlib.h>

#include "render_target.h"

#include "diagnostics.h"
#include "util.h"

static app_error log_failure(
  app_error error, const char* message, mln_status status,
  const mln_diagnostic* diagnostic
) {
  diagnostics_log_status(message, status, diagnostic);
  return error;
}

mln_render_session_attach_options render_session_attach_options(void) {
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  options.requested_texture_ring_depth = 2;
  options.frame_wake = app_event_wake(APP_EVENT_FRAME_RESULTS);
  options.driver_work_wake = app_event_wake(APP_EVENT_DRIVER_WORK);
  return options;
}

app_error render_session_service(render_session* session) {
  size_t serviced = 0;
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status = mln_render_session_service_driver_work(
    session->handle, 0, &serviced, &diagnostic
  );
  if (status != MLN_STATUS_OK) {
    return log_failure(
      APP_ERROR_BACKEND_DRAW_FAILED, "driver service failed", status,
      &diagnostic
    );
  }
  return APP_OK;
}

/// Services driver work until a lifecycle submission completes, abandoning the
/// session when the driver cannot be serviced. Startup and shutdown block here.
/// The driver's wake goes to the SDL loop rather than to this wait, so the wait
/// paces the service calls instead.
static mln_status service_until_complete(
  render_session* session, awaited_completion* completion
) {
  do {
    if (render_session_service(session) != APP_OK) {
      // Abandonment completes the pending submission with target loss.
      mln_render_abandon_result result = {.size = sizeof(result)};
      (void)mln_render_session_abandon(session->handle, &result, NULL);
      awaited_completion_wait(completion, -1);
      break;
    }
  } while (!awaited_completion_wait(completion, 1));
  return completion->status;
}

app_error render_session_finish_attach(
  render_session* session, mln_render_session handle, mln_map map,
  bool presents, awaited_completion* attached, mln_status status,
  const mln_diagnostic* diagnostic
) {
  *session =
    (render_session){.handle = handle, .map = map, .presents = presents};
  if (status != MLN_STATUS_OK) {
    awaited_completion_deinit(attached);
    session->handle = MLN_HANDLE_NULL;
    return log_failure(
      APP_ERROR_ATTACH_FAILED, "render target attach failed", status, diagnostic
    );
  }
  status = service_until_complete(session, attached);
  awaited_completion_deinit(attached);
  if (status != MLN_STATUS_OK) {
    mln_render_abandon_result result = {.size = sizeof(result)};
    (void)mln_render_session_abandon(session->handle, &result, NULL);
    (void)mln_render_session_destroy(session->handle, NULL);
    session->handle = MLN_HANDLE_NULL;
    return log_failure(
      APP_ERROR_ATTACH_FAILED, "render target attach failed", status, NULL
    );
  }
  return APP_OK;
}

void render_session_close(render_session* session) {
  if (session->handle == MLN_HANDLE_NULL) return;
  bool detached = false;
  awaited_completion completed;
  mln_completion completion;
  if (awaited_completion_init(&completed, &completion) == APP_OK) {
    mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
    const mln_status status =
      mln_render_session_detach(session->handle, &completion, &diagnostic);
    if (status == MLN_STATUS_OK) {
      detached = service_until_complete(session, &completed) == MLN_STATUS_OK;
    } else {
      diagnostics_log_status(
        "render session detach failed", status, &diagnostic
      );
    }
    awaited_completion_deinit(&completed);
  }
  if (!detached) {
    mln_render_abandon_result result = {.size = sizeof(result)};
    (void)mln_render_session_abandon(session->handle, &result, NULL);
  }
  (void)mln_render_session_destroy(session->handle, NULL);
  *session = (render_session){.handle = MLN_HANDLE_NULL};
}

app_error render_session_request_frame(render_session* session, bool force) {
  mln_frame_demand demand = mln_frame_demand_default();
  demand.flags = (force ? 0 : MLN_FRAME_DEMAND_IF_NEEDED) |
                 (session->presents ? MLN_FRAME_DEMAND_PRESENT : 0);
  demand.token = ++session->next_frame_token;
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status =
    mln_render_session_request_frame(session->handle, &demand, &diagnostic);
  if (status != MLN_STATUS_OK) {
    return log_failure(
      APP_ERROR_RENDER_FAILED, "frame demand failed", status, &diagnostic
    );
  }
  return APP_OK;
}

app_error render_session_drain_results(
  render_session* session, frame_results* out_results
) {
  *out_results = (frame_results){};
  mln_render_frame_batch batch = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  mln_status status = mln_render_session_drain_frame_results(
    session->handle, &batch, &diagnostic
  );
  if (status == MLN_STATUS_NOT_READY) return APP_OK;
  if (status != MLN_STATUS_OK) {
    return log_failure(
      APP_ERROR_RENDER_FAILED, "frame result drain failed", status, &diagnostic
    );
  }
  size_t count = 0;
  status = mln_render_frame_batch_count(batch, &count, &diagnostic);
  for (size_t i = 0; status == MLN_STATUS_OK && i < count; ++i) {
    mln_render_frame_result result = {.size = sizeof(result)};
    status = mln_render_frame_batch_get(batch, i, &result, &diagnostic);
    if (status != MLN_STATUS_OK) break;
    // No update and size pending wait for the map's next update, superseded
    // demands have a newer one behind them, and no demand carries a timeout.
    switch (result.disposition) {
      case MLN_RENDER_RESULT_RENDERED:
        out_results->rendered = true;
        out_results->needs_repaint = result.needs_repaint;
        break;
      case MLN_RENDER_RESULT_TARGET_NOT_READY:
        out_results->target_not_ready = true;
        break;
      default:
        break;
    }
  }
  mln_render_frame_batch_release(batch);
  if (status != MLN_STATUS_OK) {
    return log_failure(
      APP_ERROR_RENDER_FAILED, "frame result read failed", status, &diagnostic
    );
  }
  return APP_OK;
}

app_error render_session_resize(
  render_session* session, viewport current_viewport
) {
  const mln_render_target_extent extent =
    render_target_extent(current_viewport);
  // A later resize supersedes this one, so a live resize needs no pacing.
  const mln_completion completion =
    diagnostics_completion("render session resize failed");
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status = mln_render_session_resize(
    session->handle, &extent, &completion, &diagnostic
  );
  if (status != MLN_STATUS_OK) {
    return log_failure(
      APP_ERROR_RESIZE_FAILED, "render session resize failed", status,
      &diagnostic
    );
  }
  return APP_OK;
}

app_error render_session_resize_map(
  render_session* session, viewport current_viewport
) {
  const mln_logical_extent extent = {
    .width = current_viewport.logical_width,
    .height = current_viewport.logical_height,
    .scale_factor = current_viewport.scale_factor,
  };
  const mln_completion completion = diagnostics_completion("map resize failed");
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status =
    mln_map_resize(session->map, extent, &completion, &diagnostic);
  if (status != MLN_STATUS_OK) {
    return log_failure(
      APP_ERROR_RESIZE_FAILED, "map resize failed", status, &diagnostic
    );
  }
  return APP_OK;
}

app_error render_session_acquire_newest(
  render_session* session, mln_acquired_frame* out_frame
) {
  *out_frame = MLN_HANDLE_NULL;
  while (true) {
    mln_acquired_frame frame = MLN_HANDLE_NULL;
    mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
    const mln_status status =
      mln_render_session_acquire_frame(session->handle, &frame, &diagnostic);
    if (status == MLN_STATUS_NOT_READY) return APP_OK;
    if (status != MLN_STATUS_OK) {
      return log_failure(
        APP_ERROR_BACKEND_DRAW_FAILED, "texture acquire failed", status,
        &diagnostic
      );
    }
    render_session_release_frame(out_frame);
    *out_frame = frame;
  }
}

void render_session_release_frame(mln_acquired_frame* frame) {
  if (*frame == MLN_HANDLE_NULL) return;
  mln_gpu_sync sync = mln_gpu_sync_default();
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status =
    mln_acquired_frame_release(frame, &sync, &diagnostic);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("texture release failed", status, &diagnostic);
  }
}

app_error render_session_require_cpu_complete_producer(
  mln_acquired_frame frame, const char* message
) {
  mln_gpu_sync sync = mln_gpu_sync_default();
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status =
    mln_acquired_frame_get_producer_sync(frame, &sync, &diagnostic);
  if (status != MLN_STATUS_OK) {
    return log_failure(
      APP_ERROR_BACKEND_DRAW_FAILED, message, status, &diagnostic
    );
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

struct texture_replacement {
  texture_replacement* next;
  void* texture;
  atomic_bool completed;
  mln_status status;
};

static void complete_replacement(
  void* user_data, const mln_completion_result* result
) {
  texture_replacement* replacement = user_data;
  replacement->status = result->status;
  atomic_store_explicit(&replacement->completed, true, memory_order_release);
}

texture_replacement* texture_replacement_begin(
  void* texture, mln_completion* out_completion
) {
  texture_replacement* replacement = calloc(1, sizeof(texture_replacement));
  if (replacement == nullptr) return nullptr;
  replacement->texture = texture;
  *out_completion = (mln_completion){
    .size = sizeof(mln_completion),
    .callback = complete_replacement,
    .user_data = replacement,
  };
  return replacement;
}

void texture_replacements_queue(
  texture_replacements* replacements, texture_replacement* replacement,
  mln_status status
) {
  if (status != MLN_STATUS_OK) {
    free(replacement);
    return;
  }
  if (replacements->newest != nullptr) {
    replacements->newest->next = replacement;
  } else {
    replacements->oldest = replacement;
  }
  replacements->newest = replacement;
}

static void* take_oldest(texture_replacements* replacements) {
  texture_replacement* oldest = replacements->oldest;
  void* texture = oldest->texture;
  replacements->oldest = oldest->next;
  if (replacements->oldest == nullptr) replacements->newest = nullptr;
  free(oldest);
  return texture;
}

app_error texture_replacements_take_completed(
  texture_replacements* replacements, void** out_texture
) {
  *out_texture = nullptr;
  texture_replacement* oldest = replacements->oldest;
  if (
    oldest == nullptr ||
    !atomic_load_explicit(&oldest->completed, memory_order_acquire)
  ) {
    return APP_OK;
  }
  if (oldest->status != MLN_STATUS_OK) {
    // The session may still render into the outgoing texture or the
    // replacement, so neither can be released before it detaches.
    return log_failure(
      APP_ERROR_RESIZE_FAILED, "texture replacement failed", oldest->status,
      NULL
    );
  }
  *out_texture = take_oldest(replacements);
  return APP_OK;
}

void texture_replacements_take_any(
  texture_replacements* replacements, void** out_texture
) {
  *out_texture =
    replacements->oldest != nullptr ? take_oldest(replacements) : nullptr;
}

mln_render_target_extent render_target_extent(viewport current_viewport) {
  return (mln_render_target_extent){
    .size = sizeof(mln_render_target_extent),
    .width = current_viewport.logical_width,
    .height = current_viewport.logical_height,
    .scale_factor = current_viewport.scale_factor,
  };
}
