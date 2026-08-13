// Applying one camera two ways, immediately and over 800 milliseconds, then
// matching the event that reports the animated transition as finished.

#include <maplibre_native_c.h>

static mln_camera_options downtown(void) {
  // #region camera
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 13.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  // #endregion camera
  return camera;
}

// #region jump
mln_status jump_downtown(mln_map map) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  update.camera = downtown();
  uint64_t command_id = 0;
  return mln_map_update_camera(map, &update, &command_id);
}
// #endregion jump

mln_status ease_downtown(mln_map map, uint64_t transition_id) {
  // #region ease
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_EASE;
  update.camera = downtown();
  update.animation.fields = MLN_ANIMATION_OPTION_DURATION |
                            MLN_ANIMATION_OPTION_EASING |
                            MLN_ANIMATION_OPTION_TRANSITION_ID;
  update.animation.duration_ms = 800.0;
  update.animation.easing =
    (mln_unit_bezier){.x1 = 0.25, .y1 = 0.1, .x2 = 0.25, .y2 = 1.0};
  update.animation.transition_id = transition_id;
  uint64_t command_id = 0;
  return mln_map_update_camera(map, &update, &command_id);
  // #endregion ease
}

mln_status select_camera_events(mln_map map) {
  uint64_t command_id = 0;
  return mln_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED, &command_id
  );
}

bool transition_finished(
  const mln_runtime_event* event, uint64_t transition_id
) {
  // #region finished
  if (event->type != MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED) {
    return false;
  }
  return event->payload.camera_transition_finished.transition_id ==
         transition_id;
  // #endregion finished
}
