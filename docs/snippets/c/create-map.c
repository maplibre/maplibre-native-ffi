// Creating a runtime and a map on one thread, choosing the options that are
// fixed for the map's life, and releasing the two in order.

#include <maplibre_native_c.h>

mln_status open_runtime(const char* cache_path, mln_runtime* out_runtime) {
  // #region runtime
  mln_runtime_options options = mln_runtime_options_default();

  // A filesystem path keeps cached tiles across runs of the host. The default,
  // ":memory:", discards them when the runtime is destroyed.
  options.cache_path = cache_path;

  return mln_runtime_create(&options, out_runtime);
  // #endregion runtime
}

mln_status open_map(
  mln_runtime runtime, uint32_t width, uint32_t height, double scale_factor,
  mln_map* out_map
) {
  // #region map
  mln_map_options options = mln_map_options_default();

  // Replaced by the render target's extent at the first attach. Until then it
  // is the viewport that camera and projection queries answer against.
  options.width = width;
  options.height = height;

  // Fixed for the map's life. It selects sprites, glyphs, and raster tiles.
  options.scale_factor = scale_factor;

  return mln_map_create(runtime, &options, out_map);
  // #endregion map
}

mln_status open_static_map(
  mln_runtime runtime, uint32_t width, uint32_t height, mln_map* out_map
) {
  // #region mode
  mln_map_options options = mln_map_options_default();
  options.width = width;
  options.height = height;
  options.scale_factor = 1.0;
  options.map_mode = MLN_MAP_MODE_STATIC;

  return mln_map_create(runtime, &options, out_map);
  // #endregion mode
}

void close_map(mln_runtime runtime, mln_map map) {
  // #region release
  mln_map_destroy(map);
  mln_runtime_destroy(runtime);
  // #endregion release
}
