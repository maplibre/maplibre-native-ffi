#pragma once

#include <memory>
#include <string>

#include <mln/storage/file_source.hpp>
#include <mln/storage/resource.hpp>
#include <mln/util/async_request.hpp>

#include "maplibre_native_c.h"

namespace mln::core {

// resolved_url applies the tile server normalization the online source would
// have applied. The provider sees it alongside the request URL.
auto request_custom_resource(
  const mln::Resource& resource, std::string resolved_url,
  mln_resource_provider_callback provider_callback, void* user_data,
  mln::FileSource::Callback file_source_callback
) -> std::unique_ptr<mln::AsyncRequest>;

auto complete_resource_request(
  mln_resource_request_handle handle, const mln_resource_response* response
) -> mln_status;

auto resource_request_cancelled(
  mln_resource_request_handle handle, bool* out_cancelled
) -> mln_status;
auto set_resource_request_cancel_callback(
  mln_resource_request_handle handle,
  mln_resource_request_cancel_callback callback, void* user_data
) -> mln_status;
void release_resource_request(mln_resource_request_handle handle) noexcept;
auto wait_for_resource_request_retired(mln_resource_request_handle handle)
  -> mln_status;

}  // namespace mln::core
