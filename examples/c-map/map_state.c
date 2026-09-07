#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdatomic.h>

#include "map_state.h"

#include "diagnostics.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <sched.h>
#endif

/// Hands the processor to the runtime threads that complete the operations
/// this file waits on. The Apple SDK ships no C11 <threads.h>, so the wait
/// yields through the platform primitive rather than thrd_yield.
static void yield_to_runtime(void) {
#if defined(_WIN32)
  Sleep(0);
#else
  sched_yield();
#endif
}

typedef struct map_create_completion {
  atomic_bool completed;
  mln_status status;
  mln_map map;
} map_create_completion;

static void discard_completion(
  void* user_data, const mln_completion_result* result
) {
  (void)user_data;
  (void)result;
}

static const mln_completion discarded_completion = {
  .size = sizeof(mln_completion),
  .callback = discard_completion,
};

const mln_completion* map_state_discarded_completion(void) {
  return &discarded_completion;
}

static void complete_map_create(
  void* user_data, const mln_completion_result* result
) {
  map_create_completion* state = user_data;
  state->status = result->status;
  if (result->status == MLN_STATUS_OK && result->value_count == 1) {
    state->map = *(const mln_map*)result->value;
  }
  atomic_store_explicit(&state->completed, true, memory_order_release);
}

static app_error create_runtime(
  map_state* state, mln_wake_callback callback, void* user_data
) {
  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = ":memory:";
  options.event_wake = (mln_wake){
    .size = sizeof(mln_wake),
    .callback = callback,
    .user_data = user_data,
  };
  const mln_status status = mln_runtime_create(&options, &state->runtime);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("runtime create failed", status);
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

  map_create_completion result = {.completed = false};
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = complete_map_create,
    .user_data = &result,
  };
  mln_status status = mln_map_create(state->runtime, &options, &completion);
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("map create start failed", status);
    return APP_ERROR_MAP_CREATE_FAILED;
  }
  while (!atomic_load_explicit(&result.completed, memory_order_acquire)) {
    yield_to_runtime();
  }
  if (result.status != MLN_STATUS_OK || result.map == MLN_HANDLE_NULL) {
    diagnostics_log_status("map create failed", result.status);
    return APP_ERROR_MAP_CREATE_FAILED;
  }
  state->map = result.map;
  return APP_OK;
}

static app_error configure_map(map_state* state) {
  // The render loop re-arms from the frame result's repaint flag, so the map
  // only has to report updates that arrive between frames.
  mln_status status = mln_map_set_event_mask(
    state->map, MLN_RUNTIME_EVENT_MASK_MAP_RENDER_UPDATE_AVAILABLE,
    map_state_discarded_completion()
  );
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("event mask select failed", status);
    return APP_ERROR_EVENT_MASK_FAILED;
  }

  status = mln_map_set_style_url(
    state->map, "https://tiles.openfreemap.org/styles/bright",
    map_state_discarded_completion()
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
    state, &camera, MLN_CAMERA_UPDATE_MODE_JUMP, NULL, MLN_GESTURE_PHASE_NONE
  );
}

app_error map_state_init(
  map_state* out_state, viewport initial_viewport, mln_wake_callback event_wake,
  void* event_wake_user_data
) {
  *out_state = (map_state){};
  app_error error = create_runtime(out_state, event_wake, event_wake_user_data);
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

typedef struct runtime_teardown_completion {
  atomic_bool completed;
} runtime_teardown_completion;

static void complete_runtime_teardown(
  void* user_data, const mln_completion_result* result
) {
  (void)result;
  runtime_teardown_completion* teardown = user_data;
  atomic_store_explicit(&teardown->completed, true, memory_order_release);
}

void map_state_deinit(map_state* state) {
  runtime_teardown_completion teardown = {.completed = false};
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = complete_runtime_teardown,
    .user_data = &teardown,
  };
  if (state->map != MLN_HANDLE_NULL) {
    const mln_status status = mln_map_release(state->map, &completion);
    if (status != MLN_STATUS_OK)
      diagnostics_log_status("map release failed", status);
    else
      while (!atomic_load_explicit(&teardown.completed, memory_order_acquire))
        yield_to_runtime();
    state->map = MLN_HANDLE_NULL;
  }
  if (state->runtime != MLN_HANDLE_NULL) {
    atomic_store_explicit(&teardown.completed, false, memory_order_release);
    const mln_status status = mln_runtime_release(state->runtime, &completion);
    if (status != MLN_STATUS_OK) {
      diagnostics_log_status("runtime release failed", status);
    } else {
      // Waiting for the completion keeps process exit ordered after native
      // teardown.
      while (!atomic_load_explicit(&teardown.completed, memory_order_acquire)) {
        yield_to_runtime();
      }
    }
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
  const mln_status status = mln_map_update_camera(
    state->map, &update, map_state_discarded_completion()
  );
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("camera command failed", status);
    return APP_ERROR_CAMERA_COMMAND_FAILED;
  }
  return APP_OK;
}

app_error map_state_cancel_transitions(map_state* state) {
  const mln_status status =
    mln_map_cancel_transitions(state->map, map_state_discarded_completion());
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("camera transition cancel failed", status);
    return APP_ERROR_CAMERA_COMMAND_FAILED;
  }
  return APP_OK;
}

app_error map_state_drain_events(map_state* state, bool* out_render_update) {
  *out_render_update = false;
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
    if (event->type == MLN_RUNTIME_EVENT_MAP_RENDER_UPDATE_AVAILABLE) {
      *out_render_update = true;
    }
  }
  mln_event_batch_release(batch);
  return APP_OK;
}
