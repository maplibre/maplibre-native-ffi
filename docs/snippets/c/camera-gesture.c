// Applying drag and pinch input within one gesture.

#include <maplibre_native_c.h>

void begin_gesture(mln_map map) {
  // #region bracket
  // Give the gesture sole control of the camera.
  mln_map_cancel_transitions(map);
  mln_map_set_gesture_in_progress(map, true);
  // #endregion bracket
}

void drag_by(mln_map map, double delta_x, double delta_y) {
  // #region drag
  // Screen-space deltas in logical pixels, applied to whatever the camera is
  // now. Send the movement since the last frame rather than a total.
  mln_map_move_by(map, delta_x, delta_y);
  // #endregion drag
}

void pinch_by(mln_map map, double scale, mln_screen_point focus) {
  // #region pinch
  // The anchor stays put on screen while the map scales around it.
  mln_map_scale_by(map, scale, &focus);
  // #endregion pinch
}

void end_gesture(mln_map map, double residual_scale, mln_screen_point focus) {
  // #region release
  mln_map_set_gesture_in_progress(map, false);

  // Carry the gesture's momentum into an animation once the fingers lift.
  mln_animation_options animation = mln_animation_options_default();
  animation.fields |= MLN_ANIMATION_OPTION_DURATION;
  animation.duration_ms = 250.0;
  mln_map_scale_by_animated(map, residual_scale, &focus, &animation);
  // #endregion release
}
