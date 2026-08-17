#include "map_state.h"

#include "diagnostics.h"

static app_error await_operation(
  mln_operation operation, app_error error, const char* message
) {
  bool completed = false;
  mln_status status = mln_operation_wait(operation, -1, &completed);
  if (status == MLN_STATUS_OK && completed) {
    status = mln_operation_get_status(operation, &status) == MLN_STATUS_OK
               ? status
               : MLN_STATUS_NATIVE_ERROR;
  }
  if (status != MLN_STATUS_OK || !completed) {
    diagnostics_log_status(message, status);
    return error;
  }
  return APP_OK;
}

static app_error create_runtime(
  map_state* state, mln_notification_callback callback, void* user_data
) {
  mln_status status =
    mln_notification_source_create(&state->notification_source);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("notification source create failed", status);
    return APP_ERROR_RUNTIME_CREATE_FAILED;
  }
  status = mln_notification_source_set_callback(
    state->notification_source, callback, user_data
  );
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("notification callback install failed", status);
    mln_notification_source_close(state->notification_source);
    state->notification_source = MLN_HANDLE_NULL;
    return APP_ERROR_RUNTIME_CREATE_FAILED;
  }

  mln_runtime_options options = mln_runtime_options_default();
  options.notification_source = state->notification_source;
  options.cache_path = ":memory:";
  mln_operation operation = MLN_HANDLE_NULL;
  status = mln_runtime_create_start(&options, &operation);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("runtime create start failed", status);
    return APP_ERROR_RUNTIME_CREATE_FAILED;
  }
  const app_error waited = await_operation(
    operation, APP_ERROR_RUNTIME_CREATE_FAILED, "runtime create failed"
  );
  if (waited == APP_OK) {
    status = mln_runtime_create_take_result(operation, &state->runtime);
  }
  mln_operation_release(operation);
  if (waited != APP_OK || status != MLN_STATUS_OK) {
    if (status != MLN_STATUS_OK) {
      diagnostics_log_status("runtime create result failed", status);
    }
    return APP_ERROR_RUNTIME_CREATE_FAILED;
  }
  return APP_OK;
}

static app_error create_map(map_state* state, viewport initial_viewport) {
  mln_map_options options = mln_map_options_default();
  options.initial_extent = (mln_logical_extent){
    .width = initial_viewport.logical_width,
    .height = initial_viewport.logical_height,
    .scale_factor = initial_viewport.scale_factor,
  };
  options.map_mode = MLN_MAP_MODE_CONTINUOUS;

  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status =
    mln_map_create_start(state->runtime, &options, &operation);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("map create start failed", status);
    return APP_ERROR_MAP_CREATE_FAILED;
  }
  const app_error waited = await_operation(
    operation, APP_ERROR_MAP_CREATE_FAILED, "map create failed"
  );
  if (waited == APP_OK) {
    status = mln_map_create_take_result(operation, &state->map);
  }
  mln_operation_release(operation);
  if (waited != APP_OK || status != MLN_STATUS_OK) {
    if (status != MLN_STATUS_OK) {
      diagnostics_log_status("map create result failed", status);
    }
    return APP_ERROR_MAP_CREATE_FAILED;
  }
  return APP_OK;
}

static app_error configure_map(map_state* state) {
  uint64_t command_id = 0;
  mln_status status = mln_map_set_event_mask(
    state->map,
    MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE |
      MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED,
    &command_id
  );
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("event mask select failed", status);
    return APP_ERROR_EVENT_MASK_FAILED;
  }

  status = mln_map_set_style_url(
    state->map, "https://tiles.openfreemap.org/styles/bright", &command_id
  );
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("style load failed", status);
    return APP_ERROR_STYLE_LOAD_FAILED;
  }

  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 13.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  return map_state_update_camera(
    state, &camera, MLN_CAMERA_UPDATE_MODE_JUMP, NULL, MLN_GESTURE_PHASE_NONE, 0
  );
}

app_error map_state_init(
  map_state* out_state, viewport initial_viewport,
  mln_notification_callback notification_callback, void* notification_user_data
) {
  *out_state = (map_state){};
  app_error error =
    create_runtime(out_state, notification_callback, notification_user_data);
  if (error == APP_OK) {
    error = create_map(out_state, initial_viewport);
  }
  if (error == APP_OK) {
    error = configure_map(out_state);
  }
  if (error != APP_OK) {
    map_state_deinit(out_state);
  }
  return error;
}

static void close_handle(
  mln_status (*start)(uint64_t, mln_operation*), uint64_t handle,
  const char* message
) {
  mln_operation operation = MLN_HANDLE_NULL;
  const mln_status status = start(handle, &operation);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status(message, status);
    return;
  }
  (void)await_operation(operation, APP_ERROR_MAP_CREATE_FAILED, message);
  mln_operation_release(operation);
}

void map_state_deinit(map_state* state) {
  if (state->map != MLN_HANDLE_NULL) {
    close_handle(mln_map_close_start, state->map, "map close failed");
    state->map = MLN_HANDLE_NULL;
  }
  if (state->runtime != MLN_HANDLE_NULL) {
    close_handle(
      mln_runtime_close_start, state->runtime, "runtime close failed"
    );
    state->runtime = MLN_HANDLE_NULL;
  }
  if (state->notification_source != MLN_HANDLE_NULL) {
    mln_notification_source_clear_callback(state->notification_source);
    const mln_status status =
      mln_notification_source_close(state->notification_source);
    if (status != MLN_STATUS_OK) {
      diagnostics_log_status("notification source close failed", status);
    }
    state->notification_source = MLN_HANDLE_NULL;
  }
}

app_error map_state_camera_query(
  map_state* state, mln_camera_options* out_camera
) {
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status = mln_map_camera_query_start(state->map, &operation);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("camera query start failed", status);
    return APP_ERROR_CAMERA_COMMAND_FAILED;
  }
  const app_error waited = await_operation(
    operation, APP_ERROR_CAMERA_COMMAND_FAILED, "camera query failed"
  );
  mln_camera_query_result result = {
    .size = sizeof(mln_camera_query_result),
  };
  if (waited == APP_OK) {
    status = mln_map_camera_query_take_result(operation, &result);
    *out_camera = result.camera;
  }
  mln_operation_release(operation);
  if (waited != APP_OK || status != MLN_STATUS_OK) {
    if (status != MLN_STATUS_OK) {
      diagnostics_log_status("camera query result failed", status);
    }
    return APP_ERROR_CAMERA_COMMAND_FAILED;
  }
  return APP_OK;
}

app_error map_state_update_camera(
  map_state* state, const mln_camera_options* camera, uint32_t mode,
  const mln_animation_options* animation, uint32_t gesture_phase,
  uint64_t gesture_id
) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = mode;
  update.camera = *camera;
  if (animation != NULL) {
    update.animation = *animation;
  }
  update.gesture_phase = gesture_phase;
  update.gesture_id = gesture_id;
  uint64_t command_id = 0;
  const mln_status status =
    mln_map_update_camera(state->map, &update, &command_id);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("camera command failed", status);
    return APP_ERROR_CAMERA_COMMAND_FAILED;
  }
  return APP_OK;
}

app_error map_state_resize(map_state* state, viewport value) {
  const mln_logical_extent extent = {
    .width = value.logical_width,
    .height = value.logical_height,
    .scale_factor = value.scale_factor,
  };
  uint64_t command_id = 0;
  const mln_status status = mln_map_resize(state->map, extent, &command_id);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("map resize failed", status);
    return APP_ERROR_MAP_RESIZE_FAILED;
  }
  return APP_OK;
}

static app_error drain_runtime_events(
  map_state* state, bool* out_render_update
) {
  mln_event_batch batch = MLN_HANDLE_NULL;
  mln_status status = mln_runtime_drain_events(state->runtime, &batch);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("event drain failed", status);
    return APP_ERROR_EVENT_DRAIN_FAILED;
  }
  mln_runtime_event_batch_view view = {
    .size = sizeof(mln_runtime_event_batch_view),
  };
  status = mln_event_batch_get(batch, &view);
  if (status != MLN_STATUS_OK) {
    mln_event_batch_release(batch);
    diagnostics_log_status("event batch read failed", status);
    return APP_ERROR_EVENT_DRAIN_FAILED;
  }
  for (size_t index = 0; index < view.event_count; index += 1) {
    const char* bytes = (const char*)view.events + index * view.event_size;
    const mln_runtime_event* event = (const mln_runtime_event*)bytes;
    if (
      event->source_type != MLN_RUNTIME_EVENT_SOURCE_MAP ||
      event->source != state->map
    ) {
      continue;
    }
    if (
      event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE ||
      (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED &&
       event->payload.render_frame.needs_repaint)
    ) {
      *out_render_update = true;
    }
  }
  mln_event_batch_release(batch);
  return APP_OK;
}

app_error map_state_drain_notifications(
  map_state* state, bool* out_render_update
) {
  *out_render_update = false;
  mln_ready_batch batch = MLN_HANDLE_NULL;
  mln_status status =
    mln_notification_source_drain_ready(state->notification_source, &batch);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("notification drain failed", status);
    return APP_ERROR_EVENT_DRAIN_FAILED;
  }
  mln_ready_batch_view view = {.size = sizeof(mln_ready_batch_view)};
  status = mln_ready_batch_get(batch, &view);
  if (status != MLN_STATUS_OK) {
    mln_ready_batch_release(batch);
    diagnostics_log_status("notification batch read failed", status);
    return APP_ERROR_EVENT_DRAIN_FAILED;
  }
  app_error error = APP_OK;
  for (size_t index = 0; index < view.endpoint_count; index += 1) {
    const char* bytes =
      (const char*)view.endpoints + index * view.endpoint_size;
    const mln_ready_endpoint* endpoint = (const mln_ready_endpoint*)bytes;
    if (
      endpoint->kind == MLN_NOTIFICATION_ENDPOINT_RUNTIME_EVENTS &&
      endpoint->id == state->runtime
    ) {
      error = drain_runtime_events(state, out_render_update);
      if (error != APP_OK) {
        break;
      }
    }
  }
  mln_ready_batch_release(batch);
  return error;
}
