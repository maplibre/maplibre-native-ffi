#define MLN_BUILDING_C

#include <cstddef>
#include <cstdint>

#include "runtime/runtime.hpp"

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "resources/custom_resource_provider.hpp"

auto mln_runtime_options_default(void) noexcept -> mln_runtime_options {
  return mln_runtime_options{
    .size = sizeof(mln_runtime_options),
    .flags = 0,
    .asset_path = nullptr,
    .cache_path = nullptr,
    .maximum_cache_size = 0,
  };
}

auto mln_runtime_create(
  const mln_runtime_options* options, mln_runtime** out_runtime
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::create_runtime(options, out_runtime);
  });
}

auto mln_runtime_set_resource_provider(
  mln_runtime* runtime, const mln_resource_provider* provider
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_resource_provider(runtime, provider);
  });
}

auto mln_resource_request_complete(
  mln_resource_request_handle* handle, const mln_resource_response* response
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::complete_resource_request(handle, response);
  });
}

auto mln_resource_request_cancelled(
  const mln_resource_request_handle* handle, bool* out_cancelled
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::resource_request_cancelled(handle, out_cancelled);
  });
}

auto mln_resource_request_release(mln_resource_request_handle* handle) noexcept
  -> void {
  mln::core::release_resource_request(handle);
}

auto mln_runtime_set_resource_transform(
  mln_runtime* runtime, const mln_resource_transform* transform
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_resource_transform(runtime, transform);
  });
}

auto mln_runtime_clear_resource_transform(mln_runtime* runtime) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::clear_resource_transform(runtime);
  });
}

auto mln_runtime_run_ambient_cache_operation_start(
  mln_runtime* runtime, uint32_t operation,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::run_ambient_cache_operation_start(
      runtime, operation, out_operation_id
    );
  });
}

auto mln_runtime_offline_operation_discard(
  mln_runtime* runtime, mln_offline_operation_id operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_operation_discard(runtime, operation_id);
  });
}

auto mln_runtime_offline_region_create_start(
  mln_runtime* runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_create_start(
      runtime, definition, metadata, metadata_size, out_operation_id
    );
  });
}

auto mln_runtime_offline_region_get_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_start(
      runtime, region_id, out_operation_id
    );
  });
}

auto mln_runtime_offline_regions_list_start(
  mln_runtime* runtime, mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_list_start(runtime, out_operation_id);
  });
}

auto mln_runtime_offline_regions_merge_database_start(
  mln_runtime* runtime, const char* side_database_path,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_merge_database_start(
      runtime, side_database_path, out_operation_id
    );
  });
}

auto mln_runtime_offline_region_update_metadata_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  const uint8_t* metadata, size_t metadata_size,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_update_metadata_start(
      runtime, region_id, metadata, metadata_size, out_operation_id
    );
  });
}

auto mln_runtime_offline_region_get_status_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_status_start(
      runtime, region_id, out_operation_id
    );
  });
}

auto mln_runtime_offline_region_set_observed_start(
  mln_runtime* runtime, mln_offline_region_id region_id, bool observed,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_set_observed_start(
      runtime, region_id, observed, out_operation_id
    );
  });
}

auto mln_runtime_offline_region_set_download_state_start(
  mln_runtime* runtime, mln_offline_region_id region_id, uint32_t state,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_set_download_state_start(
      runtime,
      mln::core::OfflineRegionDownloadStateRequest{
        .region_id = region_id, .state = state
      },
      out_operation_id
    );
  });
}

auto mln_runtime_offline_region_invalidate_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_invalidate_start(
      runtime, region_id, out_operation_id
    );
  });
}

auto mln_runtime_offline_region_delete_start(
  mln_runtime* runtime, mln_offline_region_id region_id,
  mln_offline_operation_id* out_operation_id
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_delete_start(
      runtime, region_id, out_operation_id
    );
  });
}

auto mln_runtime_offline_region_create_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot** out_region
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_create_take_result(
      runtime, operation_id, out_region
    );
  });
}

auto mln_runtime_offline_region_get_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot** out_region, bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_take_result(
      runtime, operation_id, out_region, out_found
    );
  });
}

auto mln_runtime_offline_regions_list_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list** out_regions
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_list_take_result(
      runtime, operation_id, out_regions
    );
  });
}

auto mln_runtime_offline_regions_merge_database_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_list** out_regions
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_merge_database_take_result(
      runtime, operation_id, out_regions
    );
  });
}

auto mln_runtime_offline_region_update_metadata_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_snapshot** out_region
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_update_metadata_take_result(
      runtime, operation_id, out_region
    );
  });
}

auto mln_runtime_offline_region_get_status_take_result(
  mln_runtime* runtime, mln_offline_operation_id operation_id,
  mln_offline_region_status* out_status
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_status_take_result(
      runtime, operation_id, out_status
    );
  });
}

auto mln_offline_region_snapshot_get(
  const mln_offline_region_snapshot* snapshot, mln_offline_region_info* out_info
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_snapshot_get(snapshot, out_info);
  });
}

auto mln_offline_region_snapshot_destroy(
  mln_offline_region_snapshot* snapshot
) noexcept -> void {
  mln::core::offline_region_snapshot_destroy(snapshot);
}

auto mln_offline_region_list_count(
  const mln_offline_region_list* list, size_t* out_count
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_list_count(list, out_count);
  });
}

auto mln_offline_region_list_get(
  const mln_offline_region_list* list, size_t index,
  mln_offline_region_info* out_info
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_list_get(list, index, out_info);
  });
}

auto mln_offline_region_list_destroy(mln_offline_region_list* list) noexcept
  -> void {
  mln::core::offline_region_list_destroy(list);
}

auto mln_runtime_destroy(mln_runtime* runtime) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::destroy_runtime(runtime);
  });
}

auto mln_runtime_run_once(mln_runtime* runtime) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::run_runtime_once(runtime);
  });
}

auto mln_runtime_poll_event(
  mln_runtime* runtime, mln_runtime_event* out_event, bool* out_has_event
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::poll_runtime_event(runtime, out_event, out_has_event);
  });
}
