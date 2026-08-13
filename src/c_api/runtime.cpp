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
    .event_mask = MLN_RUNTIME_EVENT_MASK_ALL,
    .notification_source = MLN_HANDLE_NULL,
  };
}

auto mln_runtime_create(
  const mln_runtime_options* options, mln_runtime* out_runtime
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::create_runtime(options, out_runtime);
  });
}

auto mln_runtime_set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_resource_provider(runtime, provider);
  });
}

auto mln_runtime_clear_resource_provider(mln_runtime runtime) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::clear_resource_provider(runtime);
  });
}

auto mln_resource_request_complete(
  mln_resource_request_handle handle, const mln_resource_response* response
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::complete_resource_request(handle, response);
  });
}

auto mln_resource_request_cancelled(
  mln_resource_request_handle handle, bool* out_cancelled
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::resource_request_cancelled(handle, out_cancelled);
  });
}

auto mln_resource_request_wait_until_retired(
  mln_resource_request_handle handle
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::wait_for_resource_request_retired(handle);
  });
}

auto mln_resource_request_release(mln_resource_request_handle handle) noexcept
  -> void {
  mln::core::release_resource_request(handle);
}

auto mln_runtime_set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_resource_transform(runtime, transform);
  });
}

auto mln_resource_transform_response_set_url(
  mln_resource_transform_response* response, const char* url, size_t url_size
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::resource_transform_response_set_url(
      response, url, url_size
    );
  });
}

auto mln_runtime_clear_resource_transform(mln_runtime runtime) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::clear_resource_transform(runtime);
  });
}

auto mln_runtime_set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_http_header_transform(runtime, transform);
  });
}

auto mln_http_header_transform_response_set(
  mln_http_header_transform_response* response, const char* name,
  size_t name_size, const char* value, size_t value_size
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::http_header_transform_response_set(
      response, name, name_size, value, value_size
    );
  });
}

auto mln_runtime_clear_http_header_transform(mln_runtime runtime) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::clear_http_header_transform(runtime);
  });
}

auto mln_runtime_run_ambient_cache_operation_start(
  mln_runtime runtime, uint32_t operation, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::run_ambient_cache_operation_start(
      runtime, operation, out_operation
    );
  });
}

auto mln_runtime_set_maximum_ambient_cache_size_start(
  mln_runtime runtime, uint64_t size, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_maximum_ambient_cache_size_start(
      runtime, size, out_operation
    );
  });
}

auto mln_runtime_offline_region_create_start(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_create_start(
      runtime, definition, metadata, metadata_size, out_operation
    );
  });
}

auto mln_runtime_offline_region_get_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_start(
      runtime, region_id, out_operation
    );
  });
}

auto mln_runtime_offline_regions_list_start(
  mln_runtime runtime, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_list_start(runtime, out_operation);
  });
}

auto mln_runtime_offline_regions_merge_database_start(
  mln_runtime runtime, const char* side_database_path,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_merge_database_start(
      runtime, side_database_path, out_operation
    );
  });
}

auto mln_runtime_offline_region_update_metadata_start(
  mln_runtime runtime, mln_offline_region_id region_id, const uint8_t* metadata,
  size_t metadata_size, mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_update_metadata_start(
      runtime, region_id, metadata, metadata_size, out_operation
    );
  });
}

auto mln_runtime_offline_region_get_status_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_status_start(
      runtime, region_id, out_operation
    );
  });
}

auto mln_runtime_offline_region_set_observed_start(
  mln_runtime runtime, mln_offline_region_id region_id, bool observed,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_set_observed_start(
      runtime, region_id, observed, out_operation
    );
  });
}

auto mln_runtime_offline_region_set_download_state_start(
  mln_runtime runtime, mln_offline_region_id region_id, uint32_t state,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_set_download_state_start(
      runtime,
      mln::core::OfflineRegionDownloadStateRequest{
        .region_id = region_id, .state = state
      },
      out_operation
    );
  });
}

auto mln_runtime_offline_region_invalidate_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_invalidate_start(
      runtime, region_id, out_operation
    );
  });
}

auto mln_runtime_offline_region_delete_start(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_operation* out_operation
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_delete_start(
      runtime, region_id, out_operation
    );
  });
}

auto mln_runtime_offline_region_create_take_result(
  mln_operation operation, mln_offline_region_snapshot* out_region
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_create_take_result(operation, out_region);
  });
}

auto mln_runtime_offline_region_get_take_result(
  mln_operation operation, mln_offline_region_snapshot* out_region,
  bool* out_found
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_take_result(
      operation, out_region, out_found
    );
  });
}

auto mln_runtime_offline_regions_list_take_result(
  mln_operation operation, mln_offline_region_list* out_regions
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_list_take_result(operation, out_regions);
  });
}

auto mln_runtime_offline_regions_merge_database_take_result(
  mln_operation operation, mln_offline_region_list* out_regions
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_merge_database_take_result(
      operation, out_regions
    );
  });
}

auto mln_runtime_offline_region_update_metadata_take_result(
  mln_operation operation, mln_offline_region_snapshot* out_region
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_update_metadata_take_result(
      operation, out_region
    );
  });
}

auto mln_runtime_offline_region_get_status_take_result(
  mln_operation operation, mln_offline_region_status* out_status
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_status_take_result(
      operation, out_status
    );
  });
}

auto mln_offline_region_snapshot_get(
  mln_offline_region_snapshot snapshot, mln_offline_region_info* out_info
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_snapshot_get(snapshot, out_info);
  });
}

auto mln_offline_region_snapshot_destroy(
  mln_offline_region_snapshot snapshot
) noexcept -> void {
  mln::core::offline_region_snapshot_destroy(snapshot);
}

auto mln_offline_region_list_count(
  mln_offline_region_list list, size_t* out_count
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_list_count(list, out_count);
  });
}

auto mln_offline_region_list_get(
  mln_offline_region_list list, size_t index, mln_offline_region_info* out_info
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_list_get(list, index, out_info);
  });
}

auto mln_offline_region_list_destroy(mln_offline_region_list list) noexcept
  -> void {
  mln::core::offline_region_list_destroy(list);
}

auto mln_runtime_destroy(mln_runtime runtime) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::destroy_runtime(runtime);
  });
}

auto mln_runtime_pump(mln_runtime runtime, int64_t timeout_ms) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::pump_runtime(runtime, timeout_ms);
  });
}

auto mln_runtime_wake_source_acquire(
  mln_runtime runtime, mln_wake_source* out_source
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::acquire_wake_source(runtime, out_source);
  });
}

auto mln_wake_source_signal(mln_wake_source source) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::signal_wake_source(source);
  });
}

auto mln_wake_source_destroy(mln_wake_source source) noexcept -> void {
  mln::core::destroy_wake_source(source);
}

auto mln_runtime_drain_events(
  mln_runtime runtime, size_t max_events, mln_event_batch* out_batch
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::drain_runtime_events(runtime, max_events, out_batch);
  });
}

auto mln_event_batch_get(
  mln_event_batch batch, mln_runtime_event_batch_view* out_view
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::get_event_batch(batch, out_view);
  });
}

auto mln_event_batch_release(mln_event_batch batch) noexcept -> void {
  mln::core::release_event_batch(batch);
}

auto mln_runtime_set_event_mask(mln_runtime runtime, uint64_t mask) noexcept
  -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_runtime_event_mask(runtime, mask);
  });
}

auto mln_runtime_get_event_mask(
  mln_runtime runtime, uint64_t* out_mask
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::get_runtime_event_mask(runtime, out_mask);
  });
}
