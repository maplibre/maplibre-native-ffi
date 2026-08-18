// Creating a runtime and a map, choosing the options that are fixed for the
// map's life, and closing the two in order.

#include <maplibre_native_c.h>

mln_status open_runtime(const char* cache_path, mln_runtime* out_runtime) {
  // #region runtime
  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = cache_path;
  return mln_runtime_create(&options, out_runtime);
  // #endregion runtime
}

static void map_created(void* user_data, const mln_completion_result* result) {
  mln_map* out_map = user_data;
  if (result->status == MLN_STATUS_OK && result->value_count == 1) {
    *out_map = *(const mln_map*)result->value;
  }
}

mln_status open_map(
  mln_runtime runtime, uint32_t width, uint32_t height, double scale_factor,
  mln_map* out_map
) {
  // #region map
  mln_map_options options = mln_map_options_default();
  options.initial_extent = (mln_logical_extent){
    .width = width, .height = height, .scale_factor = scale_factor
  };
  *out_map = MLN_HANDLE_NULL;
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = map_created,
    .user_data = out_map,
  };
  return mln_map_create(runtime, &options, &completion);
  // #endregion map
}

mln_status open_static_map(
  mln_runtime runtime, uint32_t width, uint32_t height, mln_map* out_map
) {
  // #region mode
  mln_map_options options = mln_map_options_default();
  options.initial_extent =
    (mln_logical_extent){.width = width, .height = height, .scale_factor = 1.0};
  options.map_mode = MLN_MAP_MODE_STATIC;
  *out_map = MLN_HANDLE_NULL;
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = map_created,
    .user_data = out_map,
  };
  return mln_map_create(runtime, &options, &completion);
  // #endregion mode
}

void close_map(mln_runtime runtime, mln_map map) {
  // #region release
  (void)mln_map_release(map);
  (void)mln_runtime_release(runtime);
  // #endregion release
}
