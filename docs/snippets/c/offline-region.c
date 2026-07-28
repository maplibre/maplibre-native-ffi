// Creating an offline region and starting its download. Every offline call is
// asynchronous in the same way: start it, wait for the completion event that
// carries your operation ID, then take the result or discard the operation.

#include <maplibre_native_c.h>
#include <string.h>

// Pumps until the completion event for one operation arrives. Returns the
// operation's own status, which is separate from the status of starting it.
static mln_status await_operation(
  mln_runtime* runtime, mln_offline_operation_id operation_id
) {
  for (;;) {
    mln_runtime_pump(runtime, 100);

    mln_runtime_event event = {.size = sizeof(event)};
    bool has_event = false;
    while (mln_runtime_poll_event(runtime, &event, &has_event) ==
             MLN_STATUS_OK &&
           has_event) {
      if (event.type != MLN_RUNTIME_EVENT_OFFLINE_OPERATION_COMPLETED) continue;
      if (
        event.payload_type !=
        MLN_RUNTIME_EVENT_PAYLOAD_OFFLINE_OPERATION_COMPLETED
      ) {
        continue;
      }
      const mln_runtime_event_offline_operation_completed* completed =
        event.payload;
      if (completed->operation_id == operation_id) {
        return (mln_status)completed->result_status;
      }
    }
  }
}

mln_offline_region_id download_region(
  mln_runtime* runtime, const char* style_url, mln_lat_lng_bounds bounds
) {
  mln_offline_region_definition definition = {
    .size = sizeof(definition),
    .type = MLN_OFFLINE_REGION_DEFINITION_TILE_PYRAMID,
    .data.tile_pyramid = {
      .size = sizeof(mln_offline_tile_pyramid_region_definition),
      .style_url = style_url,
      .bounds = bounds,
      .min_zoom = 10.0,
      .max_zoom = 14.0,
      .pixel_ratio = 2.0f,
      .include_ideographs = true,
    },
  };

  // Metadata is opaque to MapLibre and comes back with the region later.
  const char* metadata = "{\"name\":\"San Francisco\"}";

  mln_offline_operation_id create_id = 0;
  const mln_status started = mln_runtime_offline_region_create_start(
    runtime, &definition, (const uint8_t*)metadata, strlen(metadata), &create_id
  );
  if (started != MLN_STATUS_OK) return 0;

  if (await_operation(runtime, create_id) != MLN_STATUS_OK) {
    mln_runtime_offline_operation_discard(runtime, create_id);
    return 0;
  }

  mln_offline_region_snapshot* region = NULL;
  const mln_status taken =
    mln_runtime_offline_region_create_take_result(runtime, create_id, &region);
  if (taken != MLN_STATUS_OK) {
    // A failed take leaves the operation live so you can retry it.
    mln_runtime_offline_operation_discard(runtime, create_id);
    return 0;
  }

  mln_offline_region_info info = {.size = sizeof(info)};
  mln_offline_region_snapshot_get(region, &info);
  const mln_offline_region_id region_id = info.id;
  mln_offline_region_snapshot_destroy(region);

  // Observing turns on the progress events; the download state starts the work.
  // Neither produces a result, so discard the operations once they complete.
  mln_offline_operation_id observe_id = 0;
  mln_status observing = mln_runtime_offline_region_set_observed_start(
    runtime, region_id, true, &observe_id
  );
  if (observing == MLN_STATUS_OK) {
    observing = await_operation(runtime, observe_id);
    mln_runtime_offline_operation_discard(runtime, observe_id);
  }
  if (observing != MLN_STATUS_OK) return 0;

  mln_offline_operation_id download_id = 0;
  mln_status downloading = mln_runtime_offline_region_set_download_state_start(
    runtime, region_id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, &download_id
  );
  if (downloading == MLN_STATUS_OK) {
    downloading = await_operation(runtime, download_id);
    mln_runtime_offline_operation_discard(runtime, download_id);
  }
  if (downloading != MLN_STATUS_OK) return 0;

  // From here, keep pumping and watch for status-changed events on region_id.
  return region_id;
}
