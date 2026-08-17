// Applying relative drag and pinch input within one gesture.

#include <maplibre_native_c.h>

static mln_status submit_gesture_phase(mln_map map, uint32_t phase) {
  mln_camera_update update = mln_camera_update_default();
  update.gesture_phase = phase;
  uint64_t command_id = 0;
  return mln_map_update_camera(map, &update, &command_id);
}

void begin_gesture(mln_map map) {
  // #region bracket
  submit_gesture_phase(map, MLN_GESTURE_PHASE_BEGIN);
  // #endregion bracket
}

void drag_by(mln_map map, mln_screen_point offset) {
  // #region drag
  mln_camera_delta delta = mln_camera_delta_default();
  delta.offset = offset;
  uint64_t command_id = 0;
  mln_map_apply_camera_delta(map, &delta, &command_id);
  // #endregion drag
}

void pinch_by(mln_map map, double scale, mln_screen_point focus) {
  // #region pinch
  mln_camera_delta delta = mln_camera_delta_default();
  delta.kind = MLN_CAMERA_DELTA_SCALE;
  delta.amount = scale;
  delta.has_anchor = true;
  delta.anchor = focus;
  uint64_t command_id = 0;
  mln_map_apply_camera_delta(map, &delta, &command_id);
  // #endregion pinch
}

void end_gesture(mln_map map, double residual_scale, mln_screen_point focus) {
  // #region release
  mln_camera_delta delta = mln_camera_delta_default();
  delta.kind = MLN_CAMERA_DELTA_SCALE;
  delta.amount = residual_scale;
  delta.has_anchor = true;
  delta.anchor = focus;
  delta.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  delta.animation.duration_ms = 250.0;
  uint64_t command_id = 0;
  mln_map_apply_camera_delta(map, &delta, &command_id);
  submit_gesture_phase(map, MLN_GESTURE_PHASE_END);
  // #endregion release
}
