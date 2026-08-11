// Downloading one offline region: define the area, create the region, observe
// it, then activate the download.

#include <maplibre_native_c.h>
#include <stdbool.h>
#include <string.h>

// Pumps until the completion event for this operation arrives, and returns the
// operation's own result status.
static mln_status await_operation(
  mln_runtime runtime, mln_offline_operation_id operation_id
) {
  for (;;) {
    mln_runtime_pump(runtime, 100);

    mln_runtime_event_batch batch = mln_runtime_event_batch_default();
    if (mln_runtime_drain_events(runtime, 0, &batch) != MLN_STATUS_OK) {
      return MLN_STATUS_NATIVE_ERROR;
    }

    for (size_t index = 0; index < batch.event_count; index++) {
      const char* bytes = (const char*)batch.events + index * batch.event_size;
      const mln_runtime_event* event = (const mln_runtime_event*)bytes;
      if (event->type != MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED) {
        continue;
      }
      const mln_runtime_event_offline_operation_completed* completed =
        &event->payload.offline_operation_completed;
      if (completed->operation_id == operation_id) {
        return (mln_status)completed->result_status;
      }
    }
  }
}

// An operation with no result still holds runtime state until a discard.
static mln_status finish_operation(
  mln_runtime runtime, mln_offline_operation_id operation_id
) {
  const mln_status status = await_operation(runtime, operation_id);
  mln_runtime_offline_operation_discard(runtime, operation_id);
  return status;
}

// Returns the new region's ID, or zero.
mln_offline_region_id download_region(
  mln_runtime runtime, mln_lat_lng_bounds bounds, const char* metadata
) {
  // The wait below ends on the completion event, so the subscription has to
  // select that type.
  mln_runtime_set_event_mask(
    runtime, MLN_RUNTIME_EVENT_MASK_OFFLINE_OPERATION_COMPLETED |
               MLN_RUNTIME_EVENT_MASK_OFFLINE_REGION_STATUS_CHANGED
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
  mln_offline_operation_id create_id = 0;
  if (
    mln_runtime_offline_region_create_start(
      runtime, &definition, (const uint8_t*)metadata, strlen(metadata),
      &create_id
    ) != MLN_STATUS_OK
  ) {
    return 0;
  }
  // #endregion create

  // #region result
  mln_offline_region_snapshot region = MLN_HANDLE_NULL;
  if (await_operation(runtime, create_id) == MLN_STATUS_OK) {
    mln_runtime_offline_region_create_take_result(runtime, create_id, &region);
  }
  if (region == MLN_HANDLE_NULL) {
    mln_runtime_offline_operation_discard(runtime, create_id);
    return 0;
  }
  // #endregion result

  // #region region-id
  // Copy metadata before destroying the snapshot; info.id remains valid.
  mln_offline_region_info info = {.size = sizeof(info)};
  mln_offline_region_snapshot_get(region, &info);
  mln_offline_region_snapshot_destroy(region);
  // #endregion region-id

  mln_offline_operation_id observe_id = 0;
  if (
    mln_runtime_offline_region_set_observed_start(
      runtime, info.id, true, &observe_id
    ) != MLN_STATUS_OK ||
    finish_operation(runtime, observe_id) != MLN_STATUS_OK
  ) {
    return 0;
  }

  // #region download
  mln_offline_operation_id download_id = 0;
  if (
    mln_runtime_offline_region_set_download_state_start(
      runtime, info.id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, &download_id
    ) != MLN_STATUS_OK ||
    finish_operation(runtime, download_id) != MLN_STATUS_OK
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
