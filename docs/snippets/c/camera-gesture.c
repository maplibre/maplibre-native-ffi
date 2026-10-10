// Applying relative drag and pinch input within one gesture.

#include <maplibre_native_c.h>

static mln_status submit_gesture_phase(
  mln_map map, uint32_t phase, const mln_completion* completion
) {
  mln_camera_update update = mln_camera_update_default();
  update.gesture_phase = phase;
  return mln_map_update_camera(map, &update, completion, NULL);
}

void begin_gesture(mln_map map, const mln_completion* completion) {
  // #region bracket
  submit_gesture_phase(map, MLN_GESTURE_PHASE_BEGIN, completion);
  // #endregion bracket
}

void drag_by(
  mln_map map, mln_screen_point offset, const mln_completion* completion
) {
  // #region drag
  mln_camera_delta delta = mln_camera_delta_default();
  delta.offset = offset;
  mln_map_apply_camera_delta(map, &delta, completion, NULL);
  // #endregion drag
}

void pinch_by(
  mln_map map, double scale, mln_screen_point focus,
  const mln_completion* completion
) {
  // #region pinch
  mln_camera_delta delta = mln_camera_delta_default();
  delta.kind = MLN_CAMERA_DELTA_SCALE;
  delta.amount = scale;
  delta.fields |= MLN_CAMERA_DELTA_FIELD_ANCHOR;
  delta.anchor = focus;
  mln_map_apply_camera_delta(map, &delta, completion, NULL);
  // #endregion pinch
}

void end_gesture(
  mln_map map, double residual_scale, mln_screen_point focus,
  const mln_completion* inertia_completion, const mln_completion* end_completion
) {
  // #region release
  mln_camera_delta delta = mln_camera_delta_default();
  delta.kind = MLN_CAMERA_DELTA_SCALE;
  delta.amount = residual_scale;
  delta.fields |= MLN_CAMERA_DELTA_FIELD_ANCHOR;
  delta.anchor = focus;
  delta.animation.fields = MLN_ANIMATION_OPTION_DURATION;
  delta.animation.duration_ms = 250.0;
  mln_map_apply_camera_delta(map, &delta, inertia_completion, NULL);
  submit_gesture_phase(map, MLN_GESTURE_PHASE_END, end_completion);
  // #endregion release
}
