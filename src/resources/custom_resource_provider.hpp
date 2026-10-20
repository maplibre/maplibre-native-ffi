#pragma once

#include <functional>
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
  mln::FileSource::Callback file_source_callback,
  std::function<std::unique_ptr<mln::AsyncRequest>(mln::FileSource::Callback)>
    start_native
) -> std::unique_ptr<mln::AsyncRequest>;

auto complete_resource_request(
  mln_resource_request handle, const mln_resource_response* response
) -> mln_status;

auto pass_through_resource_request(mln_resource_request handle) -> mln_status;

auto resource_request_cancelled(
  mln_resource_request handle, bool* out_cancelled
) -> mln_status;
auto set_resource_request_cancel_callback(
  mln_resource_request handle,
  const mln_resource_request_cancel_handler* handler, bool* out_cancelled
) -> mln_status;
void release_resource_request(mln_resource_request handle) noexcept;
}  // namespace mln::core
