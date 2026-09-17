// Fitting the camera to geographic bounds, then flying to the result.

#include <maplibre_native_c.h>

typedef struct fly_to_bounds_state {
  mln_map map;
  mln_completion fly_completion;
} fly_to_bounds_state;

static void fitted(void* user_data, const mln_completion_result* result) {
  fly_to_bounds_state* state = user_data;
  if (result->status != MLN_STATUS_OK || result->value_count != 1) return;

  // #region fit
  mln_camera_update update = mln_camera_update_default();
  update.mode = MLN_CAMERA_UPDATE_MODE_FLY;
  update.camera = *(const mln_camera_options*)result->value;
  mln_map_update_camera(state->map, &update, &state->fly_completion);
  // #endregion fit
}

mln_status fly_to_bounds(
  mln_map map, mln_lat_lng_bounds bounds, fly_to_bounds_state* state,
  const mln_completion* fly_completion
) {
  // #region padding
  mln_camera_fit_options fit = mln_camera_fit_options_default();
  fit.fields = MLN_CAMERA_FIT_OPTION_PADDING;
  fit.padding =
    (mln_edge_insets){.top = 24, .left = 24, .bottom = 24, .right = 24};
  // #endregion padding

  state->map = map;
  state->fly_completion = *fly_completion;
  const mln_completion fit_completion = {
    .size = sizeof(mln_completion),
    .callback = fitted,
    .user_data = state,
  };
  return mln_map_camera_for_lat_lng_bounds(map, bounds, &fit, &fit_completion);
}
