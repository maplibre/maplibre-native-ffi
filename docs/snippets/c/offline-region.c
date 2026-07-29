// Creating a region and starting its download. Every offline call works this
// way: start it, pump until its completion event arrives, then take the result
// or discard the operation.

#include <maplibre_native_c.h>
#include <string.h>

// Reports the operation's own status, which is separate from the status that
// starting it returned.
static mln_status await_operation(
  mln_runtime runtime, mln_offline_operation_id operation_id
) {
  for (;;) {
    mln_runtime_pump(runtime, 100);

    mln_runtime_event event = {.size = sizeof(event)};
    bool has_event = false;
    while (mln_runtime_poll_event(runtime, &event, &has_event) ==
             MLN_STATUS_OK &&
           has_event) {
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

// Returns the new region ID, or zero.
mln_offline_region_id create_region(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const char* metadata
) {
  mln_offline_operation_id create_id = 0;
  if (
    mln_runtime_offline_region_create_start(
      runtime, definition, (const uint8_t*)metadata, strlen(metadata),
      &create_id
    ) != MLN_STATUS_OK
  ) {
    return 0;
  }

  mln_offline_region_snapshot region = MLN_HANDLE_NULL;
  if (await_operation(runtime, create_id) == MLN_STATUS_OK) {
    mln_runtime_offline_region_create_take_result(runtime, create_id, &region);
  }
  if (region == NULL) {
    // An untaken operation stays live until you discard it.
    mln_runtime_offline_operation_discard(runtime, create_id);
    return 0;
  }

  mln_offline_region_info info = {.size = sizeof(info)};
  mln_offline_region_snapshot_get(region, &info);
  mln_offline_region_snapshot_destroy(region);
  return info.id;
}

// Setting the download state produces no result, so discard the operation once
// it completes. Marking the region observed follows the same shape.
bool start_download(mln_runtime runtime, mln_offline_region_id region_id) {
  mln_offline_operation_id download_id = 0;
  if (
    mln_runtime_offline_region_set_download_state_start(
      runtime, region_id, MLN_OFFLINE_REGION_DOWNLOAD_ACTIVE, &download_id
    ) != MLN_STATUS_OK
  ) {
    return false;
  }

  const mln_status status = await_operation(runtime, download_id);
  mln_runtime_offline_operation_discard(runtime, download_id);
  return status == MLN_STATUS_OK;
}
