// Reading the offline regions that the cache database holds, and deleting the
// ones that the host no longer needs.

#include <maplibre_native_c.h>
#include <string.h>

typedef struct offline_cleanup {
  mln_runtime runtime;
  const char* keep_metadata;
  size_t deleted;
} offline_cleanup;

static void region_deleted(
  void* user_data, const mln_completion_result* result
) {
  offline_cleanup* cleanup = user_data;
  if (result->status == MLN_STATUS_OK) cleanup->deleted++;
}

static void regions_listed(
  void* user_data, const mln_completion_result* result
) {
  offline_cleanup* cleanup = user_data;
  // #region result
  if (result->status != MLN_STATUS_OK) return;
  const mln_offline_region_info* regions = result->value;
  // #endregion result

  // #region entries
  for (size_t index = 0; index < result->value_count; index++) {
    const mln_offline_region_info* info = &regions[index];
    // Copy the definition and metadata here if the host keeps them.
    // #endregion entries
    if (
      info->metadata_size == strlen(cleanup->keep_metadata) &&
      memcmp(info->metadata, cleanup->keep_metadata, info->metadata_size) == 0
    ) {
      continue;
    }

    // #region delete
    const mln_completion completion = {
      .size = sizeof(mln_completion),
      .callback = region_deleted,
      .user_data = cleanup,
    };
    (void)mln_runtime_offline_region_delete(
      cleanup->runtime, info->id, &completion
    );
    // #endregion delete
  }
}

mln_status delete_other_regions(
  mln_runtime runtime, const char* keep_metadata, offline_cleanup* cleanup
) {
  cleanup->runtime = runtime;
  cleanup->keep_metadata = keep_metadata;
  cleanup->deleted = 0;
  const mln_completion completion = {
    .size = sizeof(mln_completion),
    .callback = regions_listed,
    .user_data = cleanup,
  };

  // #region list
  return mln_runtime_offline_regions_list(runtime, &completion);
  // #endregion list
}
