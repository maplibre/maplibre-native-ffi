// Applying drag and pinch input within one gesture.

#include <maplibre_native_c.h>

void begin_gesture(mln_map map) {
  // #region bracket
  mln_map_cancel_transitions(map);
  mln_map_set_gesture_in_progress(map, true);
  // #endregion bracket
}

void drag_by(mln_map map, double delta_x, double delta_y) {
  // #region drag
  // Screen-space deltas in logical pixels, measured since the last call rather
  // than from the start of the gesture.
  mln_map_move_by(map, delta_x, delta_y);
  // #endregion drag
}

void pinch_by(mln_map map, double scale, mln_screen_point focus) {
  // #region pinch
  // The focus point stays fixed on screen while the map scales around it.
  mln_map_scale_by(map, scale, &focus);
  // #endregion pinch
}

void end_gesture(mln_map map, double residual_scale, mln_screen_point focus) {
  // #region release
  mln_map_set_gesture_in_progress(map, false);

  mln_animation_options animation = mln_animation_options_default();
  animation.fields |= MLN_ANIMATION_OPTION_DURATION;
  animation.duration_ms = 250.0;
  mln_map_scale_by_animated(map, residual_scale, &focus, &animation);
  // #endregion release
}
