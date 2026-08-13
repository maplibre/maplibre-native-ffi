// Reading the offline regions that the cache database holds, and deleting the
// ones that the host no longer needs.

#include <maplibre_native_c.h>
#include <stdbool.h>
#include <string.h>

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
  return mln_operation_get_status(operation, &status) == MLN_STATUS_OK
           ? status
           : MLN_STATUS_NATIVE_ERROR;
}

static mln_status finish_operation(
  mln_runtime runtime, mln_operation operation
) {
  const mln_status status = await_operation(runtime, operation);
  mln_operation_discard_result(operation);
  mln_operation_release(operation);
  return status;
}

// Deletes every stored region whose metadata differs from keep_metadata, and
// returns how many it deleted.
size_t delete_other_regions(mln_runtime runtime, const char* keep_metadata) {
  // Operation readiness comes from the runtime's notification source.

  // #region list
  mln_operation list_operation = MLN_HANDLE_NULL;
  if (
    mln_runtime_offline_regions_list_start(runtime, &list_operation) !=
    MLN_STATUS_OK
  ) {
    return 0;
  }
  // #endregion list

  // #region result
  mln_offline_region_list list = MLN_HANDLE_NULL;
  if (await_operation(runtime, list_operation) == MLN_STATUS_OK) {
    mln_runtime_offline_regions_list_take_result(list_operation, &list);
  }
  if (list == MLN_HANDLE_NULL) {
    mln_operation_discard_result(list_operation);
    mln_operation_release(list_operation);
    return 0;
  }
  mln_operation_release(list_operation);
  // #endregion result

  size_t deleted = 0;

  // #region entries
  size_t count = 0;
  mln_offline_region_list_count(list, &count);

  for (size_t index = 0; index < count; index++) {
    // Copy the definition and metadata before destroying the list.
    mln_offline_region_info info = {.size = sizeof(info)};
    if (mln_offline_region_list_get(list, index, &info) != MLN_STATUS_OK) {
      continue;
    }
    // #endregion entries
    if (
      info.metadata_size == strlen(keep_metadata) &&
      memcmp(info.metadata, keep_metadata, info.metadata_size) == 0
    ) {
      continue;
    }

    // #region delete
    mln_operation delete_operation = MLN_HANDLE_NULL;
    if (
      mln_runtime_offline_region_delete_start(
        runtime, info.id, &delete_operation
      ) == MLN_STATUS_OK &&
      finish_operation(runtime, delete_operation) == MLN_STATUS_OK
    ) {
      deleted++;
    }
    // #endregion delete
  }

  mln_offline_region_list_destroy(list);
  return deleted;
}
