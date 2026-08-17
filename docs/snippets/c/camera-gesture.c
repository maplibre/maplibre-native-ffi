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
  uint64_t command_id = 0;
  mln_map_move_by(map, offset, NULL, &command_id);
  // #endregion drag
}

void pinch_by(mln_map map, double scale, mln_screen_point focus) {
  // #region pinch
  uint64_t command_id = 0;
  mln_map_scale_by(map, scale, &focus, NULL, &command_id);
  // #endregion pinch
}

void end_gesture(mln_map map, double residual_scale, mln_screen_point focus) {
  // #region release
  mln_animation_options animation = mln_animation_options_default();
  animation.fields = MLN_ANIMATION_OPTION_DURATION;
  animation.duration_ms = 250.0;
  uint64_t command_id = 0;
  mln_map_scale_by(map, residual_scale, &focus, &animation, &command_id);
  submit_gesture_phase(map, MLN_GESTURE_PHASE_END);
  // #endregion release
}
