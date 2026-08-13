#include "map_state.h"

#include "diagnostics.h"
#include "util.h"

static double clamp(double value, double min, double max) {
  if (value < min) {
    return min;
  }
  if (value > max) {
    return max;
  }
  return value;
}

static app_error expect_camera_status(mln_status status, const char* message) {
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status(message, status);
    return APP_ERROR_CAMERA_COMMAND_FAILED;
  }
  return APP_OK;
}

static app_error current_camera(mln_map map, mln_camera_options* out_camera) {
  *out_camera = mln_camera_options_default();
  const mln_status status = mln_map_get_camera(map, out_camera);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("camera snapshot failed", status);
    return APP_ERROR_CAMERA_COMMAND_FAILED;
  }
  return APP_OK;
}

static mln_animation_options animation(double duration_ms) {
  mln_animation_options options = mln_animation_options_default();
  options.fields = MLN_ANIMATION_OPTION_DURATION;
  options.duration_ms = duration_ms;
  return options;
}

/// Applies one decoded camera command. Runs on the map's owner thread, so the
/// read-modify-write commands read the current camera here.
static app_error apply_camera_command(mln_map map, camera_command command) {
  switch (command.kind) {
    case CAMERA_COMMAND_CANCEL_TRANSITIONS:
      return expect_camera_status(
        mln_map_cancel_transitions(map), "cancel camera transitions failed"
      );
    case CAMERA_COMMAND_SET_GESTURE_IN_PROGRESS:
      return expect_camera_status(
        mln_map_set_gesture_in_progress(
          map, command.as.set_gesture_in_progress.in_progress
        ),
        "set gesture in progress failed"
      );
    case CAMERA_COMMAND_MOVE_BY:
      return expect_camera_status(
        mln_map_move_by(map, command.as.move_by.dx, command.as.move_by.dy),
        "camera pan failed"
      );
    case CAMERA_COMMAND_MOVE_BY_ANIMATED: {
      const mln_animation_options options =
        animation(command.as.move_by_animated.duration_ms);
      return expect_camera_status(
        mln_map_move_by_animated(
          map, command.as.move_by_animated.dx, command.as.move_by_animated.dy,
          &options
        ),
        "keyboard pan failed"
      );
    }
    case CAMERA_COMMAND_SCALE_BY:
      return expect_camera_status(
        mln_map_scale_by(
          map, command.as.scale_by.scale, &command.as.scale_by.anchor
        ),
        "camera zoom failed"
      );
    case CAMERA_COMMAND_SCALE_BY_ANIMATED: {
      const mln_animation_options options =
        animation(command.as.scale_by_animated.duration_ms);
      return expect_camera_status(
        mln_map_scale_by_animated(
          map, command.as.scale_by_animated.scale,
          &command.as.scale_by_animated.anchor, &options
        ),
        "keyboard zoom failed"
      );
    }
    case CAMERA_COMMAND_PITCH_BY:
      return expect_camera_status(
        mln_map_pitch_by(map, command.as.delta.delta), "camera pitch failed"
      );
    case CAMERA_COMMAND_ADJUST_BEARING: {
      mln_camera_options camera;
      MAP_TRY(current_camera(map, &camera));
      const double bearing =
        (camera.fields & MLN_CAMERA_OPTION_BEARING) != 0 ? camera.bearing : 0;
      mln_camera_options target = mln_camera_options_default();
      target.fields = MLN_CAMERA_OPTION_BEARING;
      target.bearing = bearing + command.as.delta.delta;
      return expect_camera_status(
        mln_map_jump_to(map, &target), "camera rotate failed"
      );
    }
    case CAMERA_COMMAND_ADJUST_BEARING_ANIMATED: {
      mln_camera_options camera;
      MAP_TRY(current_camera(map, &camera));
      const double bearing =
        (camera.fields & MLN_CAMERA_OPTION_BEARING) != 0 ? camera.bearing : 0;
      mln_camera_options target = mln_camera_options_default();
      target.fields = MLN_CAMERA_OPTION_BEARING;
      target.bearing = bearing + command.as.animated_delta.delta;
      const mln_animation_options options =
        animation(command.as.animated_delta.duration_ms);
      return expect_camera_status(
        mln_map_ease_to(map, &target, &options), "keyboard rotate failed"
      );
    }
    case CAMERA_COMMAND_ADJUST_PITCH_ANIMATED: {
      mln_camera_options camera;
      MAP_TRY(current_camera(map, &camera));
      const double pitch =
        (camera.fields & MLN_CAMERA_OPTION_PITCH) != 0 ? camera.pitch : 0;
      mln_camera_options target = mln_camera_options_default();
      target.fields = MLN_CAMERA_OPTION_PITCH;
      target.pitch = clamp(pitch + command.as.animated_delta.delta, 0.0, 60.0);
      const mln_animation_options options =
        animation(command.as.animated_delta.duration_ms);
      return expect_camera_status(
        mln_map_ease_to(map, &target, &options), "keyboard pitch failed"
      );
    }
    case CAMERA_COMMAND_RESET_ORIENTATION: {
      mln_camera_options target = mln_camera_options_default();
      target.fields = MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
      target.bearing = 0;
      target.pitch = 0;
      const mln_animation_options options =
        animation(command.as.reset_orientation.duration_ms);
      return expect_camera_status(
        mln_map_ease_to(map, &target, &options), "camera reset failed"
      );
    }
  }
  return APP_ERROR_CAMERA_COMMAND_FAILED;
}

/// Selects the two event types the runtime loop reads. The map queues no other
/// type once this returns, and it runs before the style load, because a map
/// keeps the events it has already queued.
static app_error select_events(mln_map map) {
  const mln_status status = mln_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE |
           MLN_RUNTIME_EVENT_MASK_MAP_RENDER_FRAME_FINISHED
  );
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("event mask select failed", status);
    return APP_ERROR_EVENT_MASK_FAILED;
  }
  return APP_OK;
}

static app_error load_style(mln_map map) {
  const mln_status status =
    mln_map_set_style_url(map, "https://tiles.openfreemap.org/styles/bright");
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("style load failed", status);
    return APP_ERROR_STYLE_LOAD_FAILED;
  }
  return APP_OK;
}

static app_error set_camera(mln_map map) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 13.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  const mln_status status = mln_map_jump_to(map, &camera);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("camera jump failed", status);
    return APP_ERROR_CAMERA_JUMP_FAILED;
  }
  return APP_OK;
}

app_error map_state_init(map_state* out_state, viewport initial_viewport) {
  *out_state = (map_state){
    .runtime = MLN_HANDLE_NULL,
    .map = MLN_HANDLE_NULL,
  };

  mln_runtime_options runtime_options = mln_runtime_options_default();
  runtime_options.cache_path = ":memory:";
  mln_status status = mln_runtime_create(&runtime_options, &out_state->runtime);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("runtime create failed", status);
    return APP_ERROR_RUNTIME_CREATE_FAILED;
  }

  mln_map_options map_options = mln_map_options_default();
  map_options.width = initial_viewport.logical_width;
  map_options.height = initial_viewport.logical_height;
  map_options.scale_factor = initial_viewport.scale_factor;
  map_options.map_mode = MLN_MAP_MODE_CONTINUOUS;
  status = mln_map_create(out_state->runtime, &map_options, &out_state->map);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("map create failed", status);
    map_state_deinit(out_state);
    return APP_ERROR_MAP_CREATE_FAILED;
  }

  app_error error = select_events(out_state->map);
  if (error == APP_OK) {
    error = load_style(out_state->map);
  }
  if (error == APP_OK) {
    error = set_camera(out_state->map);
  }
  if (error != APP_OK) {
    map_state_deinit(out_state);
    return error;
  }
  return APP_OK;
}

void map_state_deinit(map_state* state) {
  // Children first: a runtime cannot be destroyed while its maps are live.
  if (state->map != MLN_HANDLE_NULL) {
    mln_map_destroy(state->map);
    state->map = MLN_HANDLE_NULL;
  }
  if (state->runtime != MLN_HANDLE_NULL) {
    mln_runtime_destroy(state->runtime);
    state->runtime = MLN_HANDLE_NULL;
  }
}

app_error map_state_apply_commands(
  map_state* state, command_queue* commands, command_list* batch
) {
  command_queue_drain_into(commands, batch);
  for (size_t i = 0; i < batch->len; i += 1) {
    const app_error error = apply_camera_command(state->map, batch->items[i]);
    if (error != APP_OK) {
      return error;
    }
  }
  return APP_OK;
}

app_error map_state_drain_events(map_state* state, bool* out_render_update) {
  *out_render_update = false;
  // One drain takes every event the pump produced. The batch borrows runtime
  // storage, and this loop keeps nothing from it.
  mln_runtime_event_batch batch = mln_runtime_event_batch_default();
  const mln_status status = mln_runtime_drain_events(state->runtime, 0, &batch);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("event drain failed", status);
    return APP_ERROR_EVENT_DRAIN_FAILED;
  }
  for (size_t index = 0; index < batch.event_count; index += 1) {
    // Step by the batch's stride: a newer runtime reports a wider event.
    const char* bytes = (const char*)batch.events + index * batch.event_size;
    const mln_runtime_event* event = (const mln_runtime_event*)bytes;
    if (
      event->source_type != MLN_RUNTIME_EVENT_SOURCE_MAP ||
      event->source != state->map
    ) {
      continue;
    }
    switch (event->type) {
      case MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE:
        *out_render_update = true;
        break;
      case MLN_RUNTIME_EVENT_MAP_RENDER_FRAME_FINISHED:
        if (event->payload.render_frame.needs_repaint) {
          *out_render_update = true;
        }
        break;
      default:
        break;
    }
  }
  return APP_OK;
}
