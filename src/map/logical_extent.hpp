#pragma once

#include <cmath>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

// The rule every consumer of a logical extent shares: a nonzero width and
// height, and a finite, positive scale factor. Consumers add their own rules
// about the scale factor on top.
inline auto validate_logical_extent(
  const mln_logical_extent& extent, const char* message
) -> mln_status {
  if (
    extent.width == 0 || extent.height == 0 ||
    !std::isfinite(extent.scale_factor) || extent.scale_factor <= 0.0
  ) {
    set_thread_error(message);
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace mln::core
