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
  const mln_camera_options camera = downtown();
  return mln_map_jump_to(map, &camera);
}
// #endregion jump

mln_status ease_downtown(mln_map map, uint64_t transition_id) {
  const mln_camera_options camera = downtown();

  // #region ease
  mln_animation_options animation = mln_animation_options_default();
  animation.fields = MLN_ANIMATION_OPTION_DURATION |
                     MLN_ANIMATION_OPTION_EASING |
                     MLN_ANIMATION_OPTION_TRANSITION_ID;
  animation.duration_ms = 800.0;
  // Control points of a cubic bezier that runs from (0, 0) to (1, 1).
  animation.easing =
    (mln_unit_bezier){.x1 = 0.25, .y1 = 0.1, .x2 = 0.25, .y2 = 1.0};
  animation.transition_id = transition_id;
  return mln_map_ease_to(map, &camera, &animation);
  // #endregion ease
}

// A map reports a finished transition only while its subscription selects this
// type.
mln_status select_camera_events(mln_map map) {
  return mln_map_set_event_mask(
    map, MLN_RUNTIME_EVENT_MASK_MAP_CAMERA_TRANSITION_FINISHED
  );
}

bool transition_finished(
  const mln_runtime_event* event, uint64_t transition_id
) {
  // #region finished
  if (event->type != MLN_RUNTIME_EVENT_MAP_CAMERA_TRANSITION_FINISHED) {
    return false;
  }
  const mln_runtime_event_camera_transition_finished* finished =
    &event->payload.camera_transition_finished;
  return finished->transition_id == transition_id;
  // #endregion finished
}
