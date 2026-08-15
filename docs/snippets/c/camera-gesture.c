// Applying drag and pinch input within one gesture. The host derives absolute
// camera targets from its input state, ordered camera queries, or a
// projection's synchronous conversions.

#include <maplibre_native_c.h>

static mln_status submit_gesture_update(
  mln_map map, const mln_camera_options* camera, uint32_t mode, uint32_t phase,
  uint64_t gesture_id, double duration_ms
) {
  mln_camera_update update = mln_camera_update_default();
  update.mode = mode;
  update.camera = *camera;
  update.gesture_phase = phase;
  update.gesture_id = gesture_id;
  if (mode != MLN_CAMERA_UPDATE_MODE_JUMP) {
    update.animation.fields = MLN_ANIMATION_OPTION_DURATION;
    update.animation.duration_ms = duration_ms;
  }
  uint64_t command_id = 0;
  return mln_map_update_camera(map, &update, &command_id);
}

void begin_gesture(mln_map map, uint64_t gesture_id) {
  // #region bracket
  mln_camera_options camera = mln_camera_options_default();
  submit_gesture_update(
    map, &camera, MLN_CAMERA_UPDATE_MODE_JUMP, MLN_GESTURE_PHASE_BEGIN,
    gesture_id, 0.0
  );
  // #endregion bracket
}

void drag_to(mln_map map, mln_lat_lng center, uint64_t gesture_id) {
  // #region drag
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER;
  camera.latitude = center.latitude;
  camera.longitude = center.longitude;
  submit_gesture_update(
    map, &camera, MLN_CAMERA_UPDATE_MODE_JUMP, MLN_GESTURE_PHASE_UPDATE,
    gesture_id, 0.0
  );
  // #endregion drag
}

void pinch_to(
  mln_map map, double zoom, mln_screen_point focus, uint64_t gesture_id
) {
  // #region pinch
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_ZOOM | MLN_CAMERA_OPTION_ANCHOR;
  camera.zoom = zoom;
  camera.anchor = focus;
  submit_gesture_update(
    map, &camera, MLN_CAMERA_UPDATE_MODE_JUMP, MLN_GESTURE_PHASE_UPDATE,
    gesture_id, 0.0
  );
  // #endregion pinch
}

void end_gesture(
  mln_map map, double residual_zoom, mln_screen_point focus, uint64_t gesture_id
) {
  // #region release
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_ZOOM | MLN_CAMERA_OPTION_ANCHOR;
  camera.zoom = residual_zoom;
  camera.anchor = focus;
  submit_gesture_update(
    map, &camera, MLN_CAMERA_UPDATE_MODE_EASE, MLN_GESTURE_PHASE_END,
    gesture_id, 250.0
  );
  // #endregion release
}
