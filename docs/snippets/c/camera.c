// Moving the camera. Camera options are a field mask plus values: only the
// fields you flag are applied, so partial updates leave everything else alone.

#include <maplibre_native_c.h>

void move_camera(mln_map* map) {
  mln_camera_options camera = mln_camera_options_default();
  camera.fields = MLN_CAMERA_OPTION_CENTER | MLN_CAMERA_OPTION_ZOOM;
  camera.latitude = 37.7749;
  camera.longitude = -122.4194;
  camera.zoom = 13.0;

  mln_map_jump_to(map, &camera);

  // ease_to defaults to a zero duration, so it behaves like a jump unless you
  // ask for time.
  mln_animation_options animation = mln_animation_options_default();
  animation.fields = MLN_ANIMATION_OPTION_DURATION;
  animation.duration_ms = 800.0;
  mln_map_ease_to(map, &camera, &animation);

  // fly_to is the exception: with no options it derives its own duration from
  // the distance travelled.
  mln_map_fly_to(map, &camera, NULL);
}

void fit_bounds(mln_map* map, mln_lat_lng_bounds bounds) {
  mln_camera_fit_options fit = mln_camera_fit_options_default();
  fit.fields = MLN_CAMERA_FIT_OPTION_PADDING;
  fit.padding =
    (mln_edge_insets){.top = 24, .left = 24, .bottom = 24, .right = 24};

  // camera_for_* computes a camera; it does not apply one. Hand the result to a
  // transition to move there.
  mln_camera_options fitted = mln_camera_options_default();
  const mln_status status =
    mln_map_camera_for_lat_lng_bounds(map, bounds, &fit, &fitted);
  if (status == MLN_STATUS_OK) {
    mln_map_ease_to(map, &fitted, NULL);
  }
}
