// Downloading one offline region: define the area, create the region, observe
// it, then activate the download.

#include <maplibre_native_c.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_operation_diagnostic(mln_operation operation) {
  size_t size = 0;
  if (
    mln_operation_copy_diagnostic(operation, NULL, 0, &size) != MLN_STATUS_OK ||
    size == 0
  ) {
    return;
  }

  char* diagnostic = malloc(size);
  if (diagnostic == NULL) return;
  if (
    mln_operation_copy_diagnostic(operation, diagnostic, size, &size) ==
    MLN_STATUS_OK
  ) {
    fprintf(stderr, "offline operation failed: %.*s\n", (int)size, diagnostic);
  }
  free(diagnostic);
}

// Waits for the operation, then returns its terminal status.
static mln_status await_operation(
  mln_runtime runtime, mln_operation operation
) {
  (void)runtime;
  bool completed = false;
  if (
    mln_operation_wait(operation, -1, &completed) != MLN_STATUS_OK || !completed
  ) {
    return MLN_STATUS_NATIVE_ERROR;
  }

  mln_status status = MLN_STATUS_NATIVE_ERROR;
  if (mln_operation_get_status(operation, &status) != MLN_STATUS_OK) {
    return MLN_STATUS_NATIVE_ERROR;
  }
  if (status != MLN_STATUS_OK) print_operation_diagnostic(operation);
  return status;
}

// Finish consumes an operation whose typed result is not needed.
static mln_status finish_operation(
  mln_runtime runtime, mln_operation operation
) {
  const mln_status status = await_operation(runtime, operation);
  mln_operation_finish(operation);
  return status;
}

// Returns the new region's ID, or zero.
mln_offline_region_id download_region(
  mln_runtime runtime, mln_lat_lng_bounds bounds, const char* metadata
) {
  mln_runtime_set_event_mask(
    runtime, MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED
  );

  // #region define
  mln_offline_tile_pyramid_region_definition pyramid = {
    .size = sizeof(pyramid),
    .style_url = "https://tiles.openfreemap.org/styles/bright",
    .bounds = bounds,
    .min_zoom = 10.0,
    .max_zoom = 15.0,
    .pixel_ratio = 1.0f,
    .include_ideographs = false,
  };
  // #endregion define

  // #region tag
  mln_offline_region_definition definition = {
    .size = sizeof(definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .data.tile_pyramid = pyramid,
  };
  // #endregion tag

  // #region create
  mln_operation create_operation = MLN_HANDLE_NULL;
  if (
    mln_runtime_offline_region_create_start(
      runtime, &definition, (const uint8_t*)metadata, strlen(metadata),
      &create_operation
    ) != MLN_STATUS_OK
  ) {
    return 0;
  }
  // #endregion create

  // #region result
  mln_offline_region_snapshot region = MLN_HANDLE_NULL;
  if (await_operation(runtime, create_operation) == MLN_STATUS_OK) {
    mln_runtime_offline_region_create_take_result(create_operation, &region);
  }
  if (region == MLN_HANDLE_NULL) {
    mln_operation_finish(create_operation);
    mln_operation_release(create_operation);
    return 0;
  }
  mln_operation_release(create_operation);
  // #endregion result

  // #region region-id
  // Copy metadata before destroying the snapshot; info.id remains valid.
  mln_offline_region_info info = {.size = sizeof(info)};
  mln_offline_region_snapshot_get(region, &info);
  mln_offline_region_snapshot_destroy(region);
  // #endregion region-id

  mln_operation observe_operation = MLN_HANDLE_NULL;
  if (
    mln_runtime_offline_region_set_observed_start(
      runtime, info.id, true, &observe_operation
    ) != MLN_STATUS_OK ||
    finish_operation(runtime, observe_operation) != MLN_STATUS_OK
  ) {
    return 0;
  }

  // #region download
  mln_operation download_operation = MLN_HANDLE_NULL;
  if (
    mln_runtime_offline_region_set_download_state_start(
      runtime, info.id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, &download_operation
    ) != MLN_STATUS_OK ||
    finish_operation(runtime, download_operation) != MLN_STATUS_OK
  ) {
    return 0;
  }
  // #endregion download

  return info.id;
}

// Reads one progress event, and reports whether the region is fully stored.
bool region_progress(
  const mln_runtime_event* event, mln_offline_region_id region_id,
  double* out_fraction
) {
  if (event->type != MLN_RUNTIME_EVENT_OFFLINE_REGION_STATUS_CHANGED) {
    return false;
  }
  // #region progress
  const mln_runtime_event_offline_region_status* progress =
    &event->payload.offline_region_status;
  if (progress->region_id != region_id) return false;

  // Treat the fraction as an estimate until the required count is precise.
  *out_fraction = progress->status.required_resource_count == 0
                    ? 0.0
                    : (double)progress->status.completed_resource_count /
                        (double)progress->status.required_resource_count;
  return progress->status.complete;
  // #endregion progress
}
