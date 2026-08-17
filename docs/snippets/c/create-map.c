// Creating a runtime and a map, choosing the options that are fixed for the
// map's life, and closing the two in order.

#include <maplibre_native_c.h>

static mln_status wait_ok(mln_operation operation) {
  bool completed = false;
  mln_status status = mln_operation_wait(operation, -1, &completed);
  if (status != MLN_STATUS_OK || !completed) return status;
  if (mln_operation_get_status(operation, &status) != MLN_STATUS_OK) {
    return MLN_STATUS_NATIVE_ERROR;
  }
  return status;
}

mln_status open_runtime(
  const char* cache_path, mln_runtime* out_runtime,
  mln_notification_source* out_notifications
) {
  // #region runtime
  mln_status status = mln_notification_source_create(out_notifications);
  if (status != MLN_STATUS_OK) return status;
  mln_runtime_options options = mln_runtime_options_default();
  options.cache_path = cache_path;
  options.notification_source = *out_notifications;

  status = mln_runtime_create(&options, out_runtime);
  // #endregion runtime
  if (status != MLN_STATUS_OK) {
    mln_notification_source_release(*out_notifications);
    *out_notifications = MLN_HANDLE_NULL;
  }
  return status;
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

  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status = mln_map_create_start(runtime, &options, &operation);
  if (status == MLN_STATUS_OK) status = wait_ok(operation);
  if (status == MLN_STATUS_OK) {
    status = mln_map_create_take_result(operation, out_map);
  }
  mln_operation_release(operation);
  return status;
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

  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status = mln_map_create_start(runtime, &options, &operation);
  if (status == MLN_STATUS_OK) status = wait_ok(operation);
  if (status == MLN_STATUS_OK) {
    status = mln_map_create_take_result(operation, out_map);
  }
  mln_operation_release(operation);
  return status;
  // #endregion mode
}

void close_map(
  mln_runtime runtime, mln_map map, mln_notification_source notifications
) {
  // #region release
  (void)mln_map_release(map);
  (void)mln_runtime_release(runtime);
  mln_notification_source_release(notifications);
  // #endregion release
}
