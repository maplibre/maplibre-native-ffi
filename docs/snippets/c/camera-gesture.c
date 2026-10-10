// Applying relative drag, pinch, and rotate input within one gesture.

#include <maplibre_native_c.h>

void begin_gesture(mln_map map, const mln_completion* completion) {
  // #region bracket
  mln_camera_delta delta = mln_camera_delta_default();
  delta.gesture_phase = MLN_GESTURE_PHASE_BEGIN;
  mln_map_apply_camera_delta(map, &delta, completion, NULL);
  // #endregion bracket
}

void drag_by(
  mln_map map, mln_screen_point offset, const mln_completion* completion
) {
  // #region drag
  mln_camera_delta delta = mln_camera_delta_default();
  delta.fields = MLN_CAMERA_DELTA_OFFSET;
  delta.offset = offset;
  delta.gesture_phase = MLN_GESTURE_PHASE_UPDATE;
  mln_map_apply_camera_delta(map, &delta, completion, NULL);
  // #endregion drag
}

void pinch_by(
  mln_map map, mln_screen_point centroid_movement, double scale,
  mln_screen_point centroid, const mln_completion* completion
) {
  // #region pinch
  mln_camera_delta delta = mln_camera_delta_default();
  delta.fields =
    MLN_CAMERA_DELTA_OFFSET | MLN_CAMERA_DELTA_SCALE | MLN_CAMERA_DELTA_ANCHOR;
  delta.offset = centroid_movement;
  delta.scale = scale;
  delta.anchor = centroid;
  delta.gesture_phase = MLN_GESTURE_PHASE_UPDATE;
  mln_map_apply_camera_delta(map, &delta, completion, NULL);
  // #endregion pinch
}

void rotate_by(
  mln_map map, double bearing, double pitch, mln_screen_point centroid,
  const mln_completion* completion
) {
  // #region rotate
  mln_camera_delta delta = mln_camera_delta_default();
  delta.fields =
    MLN_CAMERA_DELTA_BEARING | MLN_CAMERA_DELTA_PITCH | MLN_CAMERA_DELTA_ANCHOR;
  delta.bearing = bearing;
  delta.pitch = pitch;
  delta.anchor = centroid;
  delta.gesture_phase = MLN_GESTURE_PHASE_UPDATE;
  mln_map_apply_camera_delta(map, &delta, completion, NULL);
  // #endregion rotate
}

void end_gesture(
  mln_map map, const mln_screen_point* inertia, const mln_completion* completion
) {
  // #region release
  mln_camera_delta delta = mln_camera_delta_default();
  if (inertia != NULL) {
    delta.fields = MLN_CAMERA_DELTA_OFFSET;
    delta.offset = *inertia;
    delta.animation.fields = MLN_ANIMATION_OPTION_DURATION;
    delta.animation.duration_ms = 250.0;
  }
  delta.gesture_phase = MLN_GESTURE_PHASE_END;
  mln_map_apply_camera_delta(map, &delta, completion, NULL);
  // #endregion release
}
