// Creating a runtime and a map, choosing the options that are fixed for the
// map's life, and closing the two in order.

#include <maplibre_native_c.h>

static void events_ready(void* user_data) {
  // Called from any native thread: schedule a drain on the host receiver and
  // return.
  (void)user_data;
}

mln_status open_runtime(
  const char* cache_path, void* receiver, mln_runtime* out_runtime
) {
  // #region runtime
  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = cache_path;
  options.event_wake.callback = events_ready;
  options.event_wake.user_data = receiver;
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
  uint32_t map_mode, mln_map* out_map
) {
  // #region map
  mln_map_options options = mln_map_options_default();
  options.initial_extent = (mln_logical_extent){
    .width = width, .height = height, .scale_factor = scale_factor
  };
  options.map_mode = map_mode;
  *out_map = MLN_HANDLE_NULL;
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = map_created,
    .user_data = out_map,
  };
  return mln_map_create(runtime, &options, &completion);
  // #endregion map
}

static void runtime_torn_down(
  void* user_data, const mln_completion_result* result
) {
  (void)result;
  // Signal the host's own shutdown gate here; after this callback returns, no
  // library thread runs, so the process may exit.
  (void)user_data;
}

static void map_torn_down(
  void* user_data, const mln_completion_result* result
) {
  (void)user_data;
  (void)result;
}

void close_map(mln_runtime runtime, mln_map map) {
  // #region release
  const mln_completion map_teardown = {
    .size = sizeof(mln_completion),
    .callback = map_torn_down,
  };
  (void)mln_map_release(map, &map_teardown);
  const mln_completion teardown = {
    .size = sizeof(mln_completion),
    .callback = runtime_torn_down,
  };
  (void)mln_runtime_release(runtime, &teardown);
  // #endregion release
}
