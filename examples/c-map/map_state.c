#include "map_state.h"

#include "diagnostics.h"
#include "events.h"
#include "util.h"

static app_error create_runtime(map_state* state) {
  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = ":memory:";
  options.event_wake = app_event_wake(APP_EVENT_RUNTIME_EVENTS);
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status =
    mln_runtime_create(&options, &state->runtime, &diagnostic);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("runtime create failed", status, &diagnostic);
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
  // The render loop re-arms from the frame result's repaint flag, so the map
  // only has to report updates that arrive between frames.
  options.event_mask = MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE;

  awaited_completion created;
  mln_completion completion;
  MAP_TRY(awaited_completion_init(&created, &completion));
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status =
    mln_map_create(state->runtime, &options, &completion, &diagnostic);
  if (status == MLN_STATUS_OK) {
    awaited_completion_wait(&created, -1);
  }
  awaited_completion_deinit(&created);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("map create start failed", status, &diagnostic);
    return APP_ERROR_MAP_CREATE_FAILED;
  }
  if (created.status != MLN_STATUS_OK || created.map == MLN_HANDLE_NULL) {
    diagnostics_log_status("map create failed", created.status, NULL);
    return APP_ERROR_MAP_CREATE_FAILED;
  }
  state->map = created.map;
  return APP_OK;
}

/// The style a smoke run renders, which needs no network.
static const char smoke_style_json[] =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background\","
  "\"type\":\"background\",\"paint\":{\"background-color\":\"#d8f1ff\"}}]}";

static mln_status load_style(
  map_state* state, bool smoke, mln_diagnostic* diagnostic
) {
  const mln_completion completion = diagnostics_completion("style load failed");
  if (smoke) {
    return mln_map_set_style_json(
      state->map,
      (mln_buffer_view){
        .data = smoke_style_json, .size = sizeof(smoke_style_json) - 1
      },
      &completion, diagnostic
    );
  }
  return mln_map_set_style_url(
    state->map, "https://tiles.openfreemap.org/styles/bright", &completion,
    diagnostic
  );
}

static app_error configure_map(map_state* state, bool smoke) {
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status = load_style(state, smoke, &diagnostic);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("style load failed", status, &diagnostic);
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
    state, &camera, MLN_CAMERA_UPDATE_MODE_JUMP, NULL, MLN_GESTURE_PHASE_NONE
  );
}

app_error map_state_init(
  map_state* out_state, viewport initial_viewport, bool smoke
) {
  *out_state = (map_state){};
  app_error error = create_runtime(out_state);
  if (error == APP_OK) {
    error = create_map(out_state, initial_viewport);
  }
  if (error == APP_OK) {
    error = configure_map(out_state, smoke);
  }
  if (error != APP_OK) {
    map_state_deinit(out_state);
  }
  return error;
}

/// mln_map_release and mln_runtime_release.
typedef mln_status (*release_function)(
  uint64_t handle, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
);

/// Releases a handle and waits for native retirement, so the map's render
/// resources are gone before its runtime closes, and the runtime's threads
/// stop before the app tears down state its callbacks use.
static void release_and_wait(
  const char* message, release_function release, uint64_t handle
) {
  awaited_completion released;
  mln_completion completion;
  if (awaited_completion_init(&released, &completion) != APP_OK) return;
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status = release(handle, &completion, &diagnostic);
  if (status == MLN_STATUS_OK) {
    awaited_completion_wait(&released, -1);
  } else {
    diagnostics_log_status(message, status, &diagnostic);
  }
  awaited_completion_deinit(&released);
}

void map_state_deinit(map_state* state) {
  if (state->map != MLN_HANDLE_NULL) {
    release_and_wait("map release failed", mln_map_release, state->map);
    state->map = MLN_HANDLE_NULL;
  }
  if (state->runtime != MLN_HANDLE_NULL) {
    release_and_wait(
      "runtime release failed", mln_runtime_release, state->runtime
    );
    state->runtime = MLN_HANDLE_NULL;
  }
}

app_error map_state_update_camera(
  map_state* state, const mln_camera_options* camera, uint32_t mode,
  const mln_animation_options* animation, uint32_t gesture_phase
) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = mode;
  update.camera = *camera;
  if (animation != NULL) {
    update.animation = *animation;
  }
  update.gesture_phase = gesture_phase;
  const mln_completion completion =
    diagnostics_completion("camera command failed");
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status =
    mln_map_update_camera(state->map, &update, &completion, &diagnostic);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("camera command failed", status, &diagnostic);
    return APP_ERROR_CAMERA_COMMAND_FAILED;
  }
  return APP_OK;
}

app_error map_state_cancel_transitions(map_state* state) {
  const mln_completion completion =
    diagnostics_completion("camera transition cancel failed");
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  const mln_status status =
    mln_map_cancel_transitions(state->map, &completion, &diagnostic);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status(
      "camera transition cancel failed", status, &diagnostic
    );
    return APP_ERROR_CAMERA_COMMAND_FAILED;
  }
  return APP_OK;
}

app_error map_state_drain_events(map_state* state, bool* out_render_update) {
  *out_render_update = false;
  mln_event_batch batch = MLN_HANDLE_NULL;
  mln_diagnostic diagnostic = {.size = sizeof(diagnostic)};
  mln_status status =
    mln_runtime_drain_events(state->runtime, &batch, &diagnostic);
  if (status == MLN_STATUS_NOT_READY) {
    return APP_OK;
  }
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("event drain failed", status, &diagnostic);
    return APP_ERROR_EVENT_DRAIN_FAILED;
  }
  mln_runtime_event_batch_view view = {
    .size = sizeof(mln_runtime_event_batch_view),
  };
  status = mln_event_batch_get(batch, &view, &diagnostic);
  if (status != MLN_STATUS_OK) {
    mln_event_batch_release(batch);
    diagnostics_log_status("event batch read failed", status, &diagnostic);
    return APP_ERROR_EVENT_DRAIN_FAILED;
  }
  for (size_t index = 0; index < view.event_count; index += 1) {
    const char* bytes = (const char*)view.events + index * view.event_size;
    const mln_runtime_event* event = (const mln_runtime_event*)bytes;
    if (
      event->source_type == MLN_RUNTIME_EVENT_SOURCE_MAP &&
      event->source == state->map &&
      event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE
    ) {
      *out_render_update = true;
    }
  }
  mln_event_batch_release(batch);
  return APP_OK;
}
