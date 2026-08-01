// Reading the offline regions that the cache database holds, and deleting the
// ones that the host no longer needs.

#include <maplibre_native_c.h>
#include <stdbool.h>
#include <string.h>

// Returns the operation's own status, which the completion event carries.
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

// An operation with no result still holds runtime state until a discard.
static mln_status finish_operation(
  mln_runtime runtime, mln_offline_operation_id operation_id
) {
  const mln_status status = await_operation(runtime, operation_id);
  mln_runtime_offline_operation_discard(runtime, operation_id);
  return status;
}

// Deletes every stored region whose metadata differs from keep_metadata, and
// returns how many it deleted.
size_t delete_other_regions(mln_runtime runtime, const char* keep_metadata) {
  mln_offline_operation_id list_id = 0;
  if (
    mln_runtime_offline_regions_list_start(runtime, &list_id) != MLN_STATUS_OK
  ) {
    return 0;
  }

  mln_offline_region_list list = MLN_HANDLE_NULL;
  if (await_operation(runtime, list_id) == MLN_STATUS_OK) {
    mln_runtime_offline_regions_list_take_result(runtime, list_id, &list);
  }
  if (list == MLN_HANDLE_NULL) {
    mln_runtime_offline_operation_discard(runtime, list_id);
    return 0;
  }

  size_t count = 0;
  mln_offline_region_list_count(list, &count);

  size_t deleted = 0;
  for (size_t index = 0; index < count; index++) {
    mln_offline_region_info info = {.size = sizeof(info)};
    if (mln_offline_region_list_get(list, index, &info) != MLN_STATUS_OK) {
      continue;
    }
    if (
      info.metadata_size == strlen(keep_metadata) &&
      memcmp(info.metadata, keep_metadata, info.metadata_size) == 0
    ) {
      continue;
    }

    mln_offline_operation_id delete_id = 0;
    if (
      mln_runtime_offline_region_delete_start(runtime, info.id, &delete_id) ==
        MLN_STATUS_OK &&
      finish_operation(runtime, delete_id) == MLN_STATUS_OK
    ) {
      deleted++;
    }
  }

  mln_offline_region_list_destroy(list);
  return deleted;
}
