// Downloading one offline region: define the area, create the region, observe
// it, then activate the download.

#include <maplibre_native_c.h>
#include <stdbool.h>
#include <string.h>

typedef struct offline_download {
  mln_runtime runtime;
  mln_offline_region_id region_id;
  mln_completion observe_completion;
  mln_completion download_completion;
} offline_download;

static void region_created(
  void* user_data, const mln_completion_result* result
) {
  offline_download* download = user_data;
  // #region result
  if (result->status != MLN_STATUS_OK || result->value_count != 1) return;
  const mln_offline_region_info* info = result->value;
  // Copy definition and metadata here if the host keeps them after return.
  // #endregion result

  // #region region-id
  download->region_id = info->id;
  // #endregion region-id

  (void)mln_runtime_offline_region_set_observed(
    download->runtime, info->id, true, &download->observe_completion
  );

  // #region download
  (void)mln_runtime_offline_region_set_download_state(
    download->runtime, info->id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE,
    &download->download_completion
  );
  // #endregion download
}

mln_status download_region(
  mln_runtime runtime, mln_lat_lng_bounds bounds, const char* metadata,
  offline_download* download, const mln_completion* observe_completion,
  const mln_completion* download_completion
) {
  (void)mln_runtime_set_event_mask(
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

  download->runtime = runtime;
  download->region_id = 0;
  download->observe_completion = *observe_completion;
  download->download_completion = *download_completion;
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = region_created,
    .user_data = download,
  };

  // #region create
  return mln_runtime_offline_region_create(
    runtime, &definition, (const uint8_t*)metadata, strlen(metadata),
    &completion
  );
  // #endregion create
}

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

  *out_fraction = progress->status.required_resource_count == 0
                    ? 0.0
                    : (double)progress->status.completed_resource_count /
                        (double)progress->status.required_resource_count;
  return progress->status.complete;
  // #endregion progress
}
