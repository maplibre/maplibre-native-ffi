// Fitting the camera to geographic bounds, then flying to the result.

#include <maplibre_native_c.h>

static mln_status wait_ok(mln_operation operation) {
  bool completed = false;
  mln_status status = mln_operation_wait(operation, -1, &completed);
  if (status != MLN_STATUS_OK || !completed) return status;
  return mln_operation_get_status(operation, &status) == MLN_STATUS_OK
           ? status
           : MLN_STATUS_NATIVE_ERROR;
}

mln_status fly_to_bounds(mln_map map, mln_lat_lng_bounds bounds) {
  // #region padding
  mln_camera_fit_options fit = mln_camera_fit_options_default();
  fit.fields = MLN_CAMERA_FIT_OPTION_PADDING;
  fit.padding =
    (mln_edge_insets){.top = 24, .left = 24, .bottom = 24, .right = 24};
  // #endregion padding

  // #region fit
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status =
    mln_map_camera_for_lat_lng_bounds_start(map, bounds, &fit, &operation);
  if (status == MLN_STATUS_OK) status = wait_ok(operation);
  mln_camera_options fitted = mln_camera_options_default();
  if (status == MLN_STATUS_OK) {
    status = mln_map_camera_for_lat_lng_bounds_take_result(operation, &fitted);
  }
  mln_operation_release(operation);
  if (status != MLN_STATUS_OK) return status;

  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_FLY;
  update.camera = fitted;
  uint64_t command_id = 0;
  return mln_map_update_camera(map, &update, &command_id);
  // #endregion fit
}
