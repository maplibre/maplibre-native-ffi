#pragma once

#include <cstdint>
#include <string>

#include <mln/storage/resource_transform.hpp>

#include "maplibre_native_c.h"

namespace mln::core {

// Hands one URL to a transform callback with a request handle, which answers
// MapLibre's finished callback once. A request that cannot be issued keeps its
// URL without reaching the callback.
auto request_resource_transform(
  mln_resource_transform_callback callback, void* user_data,
  mln_resource_kind kind, const std::string& url,
  mln::ResourceTransform::FinishedCallback finished
) -> void;

auto complete_resource_transform_request(
  mln_resource_transform_request request, mln_buffer_view url
) -> mln_status;

void release_resource_transform_request(
  mln_resource_transform_request request
) noexcept;

}  // namespace mln::core
