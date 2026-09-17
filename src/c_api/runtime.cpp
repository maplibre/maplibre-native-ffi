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
    .event_wake = mln_wake{
      .size = sizeof(mln_wake),
      .callback = nullptr,
      .user_data = nullptr,
      .release_user_data = nullptr,
    },
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
  mln_runtime runtime, const mln_resource_provider* provider,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_resource_provider(runtime, provider, completion);
  });
}

auto mln_runtime_clear_resource_provider(
  mln_runtime runtime, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::clear_resource_provider(runtime, completion);
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

auto mln_resource_request_set_cancel_callback(
  mln_resource_request_handle handle,
  mln_resource_request_cancel_callback callback, void* user_data,
  bool* out_cancelled
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_resource_request_cancel_callback(
      handle, callback, user_data, out_cancelled
    );
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
  mln_runtime runtime, const mln_resource_transform* transform,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_resource_transform(runtime, transform, completion);
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

auto mln_runtime_clear_resource_transform(
  mln_runtime runtime, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::clear_resource_transform(runtime, completion);
  });
}

auto mln_runtime_set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_http_header_transform(runtime, transform, completion);
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

auto mln_runtime_clear_http_header_transform(
  mln_runtime runtime, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::clear_http_header_transform(runtime, completion);
  });
}

auto mln_runtime_run_ambient_cache_operation(
  mln_runtime runtime, uint32_t operation, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::run_ambient_cache_operation_start(
      runtime, operation, completion
    );
  });
}

auto mln_runtime_set_maximum_ambient_cache_size(
  mln_runtime runtime, uint64_t size, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::set_maximum_ambient_cache_size_start(
      runtime, size, completion
    );
  });
}

auto mln_runtime_offline_region_create(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_create_start(
      runtime, definition, metadata, metadata_size, completion
    );
  });
}

auto mln_runtime_offline_region_get(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_start(runtime, region_id, completion);
  });
}

auto mln_runtime_offline_regions_list(
  mln_runtime runtime, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_list_start(runtime, completion);
  });
}

auto mln_runtime_offline_regions_merge_database(
  mln_runtime runtime, const char* side_database_path,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_regions_merge_database_start(
      runtime, side_database_path, completion
    );
  });
}

auto mln_runtime_offline_region_update_metadata(
  mln_runtime runtime, mln_offline_region_id region_id, const uint8_t* metadata,
  size_t metadata_size, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_update_metadata_start(
      runtime, region_id, metadata, metadata_size, completion
    );
  });
}

auto mln_runtime_offline_region_get_status(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_get_status_start(
      runtime, region_id, completion
    );
  });
}

auto mln_runtime_offline_region_set_observed(
  mln_runtime runtime, mln_offline_region_id region_id, bool observed,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_set_observed_start(
      runtime, region_id, observed, completion
    );
  });
}

auto mln_runtime_offline_region_set_download_state(
  mln_runtime runtime, mln_offline_region_id region_id, uint32_t state,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_set_download_state_start(
      runtime,
      mln::core::OfflineRegionDownloadStateRequest{
        .region_id = region_id, .state = state
      },
      completion
    );
  });
}

auto mln_runtime_offline_region_invalidate(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_invalidate_start(
      runtime, region_id, completion
    );
  });
}

auto mln_runtime_offline_region_delete(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::offline_region_delete_start(
      runtime, region_id, completion
    );
  });
}

auto mln_runtime_barrier(
  mln_runtime runtime, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::runtime_barrier_start(runtime, completion);
  });
}

auto mln_runtime_release(
  mln_runtime runtime, const mln_completion* completion
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::release_runtime(runtime, completion);
  });
}

auto mln_runtime_drain_events(
  mln_runtime runtime, mln_event_batch* out_batch
) noexcept -> mln_status {
  return mln::c_api::status_boundary([&]() -> mln_status {
    return mln::core::drain_runtime_events(runtime, out_batch);
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
