// Fitting the camera to geographic bounds, then flying to the result.

#include <maplibre_native_c.h>

mln_status fly_to_bounds(mln_map map, mln_lat_lng_bounds bounds) {
  mln_camera_fit_options fit = mln_camera_fit_options_default();
  fit.fields = MLN_CAMERA_FIT_OPTION_PADDING;
  fit.padding =
    (mln_edge_insets){.top = 24, .left = 24, .bottom = 24, .right = 24};

  mln_camera_options fitted = mln_camera_options_default();
  const mln_status status =
    mln_map_camera_for_lat_lng_bounds(map, bounds, &fit, &fitted);
  if (status != MLN_STATUS_OK) {
    return status;
  }

  return mln_map_fly_to(map, &fitted, NULL);
}
