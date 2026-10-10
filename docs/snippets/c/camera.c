// Applying one camera two ways, immediately and over 800 milliseconds, then
// learning how the animated transition ended, or stopping it.

#include <maplibre_native_c.h>

static mln_camera_options downtown(void) {
  // #region camera
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM |
                  MLN_CAMERA_OPTION_BEARING | MLN_CAMERA_OPTION_PITCH;
  camera.center.latitude = 37.7749;
  camera.center.longitude = -122.4194;
  camera.zoom = 13.0;
  camera.bearing = 12.0;
  camera.pitch = 30.0;
  // #endregion camera
  return camera;
}

// #region jump
mln_status jump_downtown(mln_map map, const mln_completion* completion) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_JUMP;
  update.camera = downtown();
  return mln_map_update_camera(map, &update, completion, NULL);
}
// #endregion jump

// #region end
static void transition_ended(
  void* user_data, const mln_camera_transition_end* end
) {
  // Runs once on the runtime worker. The snapshot at end->generation shows
  // the camera where the transition stopped.
  bool* arrived = user_data;
  *arrived = end->outcome == MLN_CAMERA_TRANSITION_OUTCOME_COMPLETED;
}
// #endregion end

mln_status ease_downtown(
  mln_map map, uint64_t transition_id, bool* arrived,
  const mln_completion* completion
) {
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
  update.animation.end_handler.callback = transition_ended;
  update.animation.end_handler.user_data = arrived;
  return mln_map_update_camera(map, &update, completion, NULL);
  // #endregion ease
}

mln_status stop_downtown(
  mln_map map, uint64_t transition_id, const mln_completion* completion
) {
  // #region cancel
  return mln_map_cancel_camera_transition(map, transition_id, completion, NULL);
  // #endregion cancel
}
