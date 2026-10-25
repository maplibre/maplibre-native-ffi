#define MLN_BUILDING_C

#include <cstddef>
#include <cstdint>

#include "runtime/runtime.hpp"

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "resources/custom_resource_provider.hpp"
#include "resources/resource_transform.hpp"

auto mln_runtime_create(
  const mln_runtime_options* options, mln_runtime* out_runtime,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::create_runtime(options, out_runtime);
  });
}

auto mln_runtime_set_resource_provider(
  mln_runtime runtime, const mln_resource_provider* provider,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::set_resource_provider(runtime, provider, completion);
  });
}

auto mln_runtime_clear_resource_provider(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::clear_resource_provider(runtime, completion);
  });
}

auto mln_resource_request_complete(
  mln_resource_request handle, const mln_resource_response* response,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::complete_resource_request(handle, response);
  });
}

auto mln_resource_request_pass_through(
  mln_resource_request handle, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::pass_through_resource_request(handle);
  });
}

auto mln_resource_request_is_cancelled(
  mln_resource_request handle, bool* out_cancelled,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::resource_request_cancelled(handle, out_cancelled);
  });
}

auto mln_resource_request_set_cancel_callback(
  mln_resource_request handle,
  const mln_resource_request_cancel_handler* handler, bool* out_cancelled,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::set_resource_request_cancel_callback(
      handle, handler, out_cancelled
    );
  });
}

auto mln_resource_request_release(mln_resource_request handle) noexcept
  -> void {
  mln::core::release_resource_request(handle);
}

auto mln_runtime_set_resource_transform(
  mln_runtime runtime, const mln_resource_transform* transform,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::set_resource_transform(runtime, transform, completion);
  });
}

auto mln_runtime_clear_resource_transform(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::clear_resource_transform(runtime, completion);
  });
}

auto mln_resource_transform_request_complete(
  mln_resource_transform_request request, mln_buffer_view url,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::complete_resource_transform_request(request, url);
  });
}

auto mln_resource_transform_request_release(
  mln_resource_transform_request request
) noexcept -> void {
  mln::core::release_resource_transform_request(request);
}

auto mln_runtime_set_http_header_transform(
  mln_runtime runtime, const mln_http_header_transform* transform,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::set_http_header_transform(runtime, transform, completion);
  });
}

auto mln_http_header_transform_response_set_header(
  mln_http_header_transform_response* response, mln_buffer_view name,
  mln_buffer_view value, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::http_header_transform_response_set(response, name, value);
  });
}

auto mln_runtime_clear_http_header_transform(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::clear_http_header_transform(runtime, completion);
  });
}

auto mln_runtime_run_ambient_cache_operation(
  mln_runtime runtime, mln_ambient_cache_operation operation,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::run_ambient_cache_operation_start(
      runtime, operation, completion
    );
  });
}

auto mln_runtime_set_maximum_ambient_cache_size(
  mln_runtime runtime, uint64_t size, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::set_maximum_ambient_cache_size_start(
      runtime, size, completion
    );
  });
}

auto mln_runtime_create_offline_region(
  mln_runtime runtime, const mln_offline_region_definition* definition,
  const uint8_t* metadata, size_t metadata_size,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_region_create_start(
      runtime, definition, metadata, metadata_size, completion
    );
  });
}

auto mln_runtime_get_offline_region(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_region_get_start(runtime, region_id, completion);
  });
}

auto mln_runtime_list_offline_regions(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_regions_list_start(runtime, completion);
  });
}

auto mln_runtime_merge_offline_regions(
  mln_runtime runtime, mln_buffer_view side_database_path,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_regions_merge_database_start(
      runtime, side_database_path, completion
    );
  });
}

auto mln_runtime_update_offline_region_metadata(
  mln_runtime runtime, mln_offline_region_id region_id, const uint8_t* metadata,
  size_t metadata_size, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_region_update_metadata_start(
      runtime, region_id, metadata, metadata_size, completion
    );
  });
}

auto mln_runtime_get_offline_region_status(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_region_get_status_start(
      runtime, region_id, completion
    );
  });
}

auto mln_runtime_set_offline_region_observed(
  mln_runtime runtime, mln_offline_region_id region_id, bool observed,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_region_set_observed_start(
      runtime, region_id, observed, completion
    );
  });
}

auto mln_runtime_set_offline_region_download_state(
  mln_runtime runtime, mln_offline_region_id region_id,
  mln_offline_region_download_state state, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_region_set_download_state_start(
      runtime,
      mln::core::OfflineRegionDownloadStateRequest{
        .region_id = region_id, .state = state
      },
      completion
    );
  });
}

auto mln_runtime_invalidate_offline_region(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_region_invalidate_start(
      runtime, region_id, completion
    );
  });
}

auto mln_runtime_delete_offline_region(
  mln_runtime runtime, mln_offline_region_id region_id,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::offline_region_delete_start(
      runtime, region_id, completion
    );
  });
}

auto mln_runtime_release(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::release_runtime(runtime, completion);
  });
}

auto mln_runtime_set_event_handler(
  mln_runtime runtime, const mln_runtime_event_handler* handler,
  const mln_completion* completion, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::set_runtime_event_handler(runtime, handler, completion);
  });
}

auto mln_runtime_clear_event_handler(
  mln_runtime runtime, const mln_completion* completion,
  mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::clear_runtime_event_handler(runtime, completion);
  });
}

auto mln_runtime_set_event_mask(
  mln_runtime runtime, uint64_t mask, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::set_runtime_event_mask(runtime, mask);
  });
}

auto mln_runtime_get_event_mask(
  mln_runtime runtime, uint64_t* out_mask, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::get_runtime_event_mask(runtime, out_mask);
  });
}

extern "C" MLN_API auto mln_runtime_dispose(
  mln_runtime runtime, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary<false>(out_diagnostic, [&]() {
    return mln::core::dispose_runtime(runtime);
  });
}
