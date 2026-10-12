#define MLN_BUILDING_C

#include <cstdint>

#include "logging/logging.hpp"

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"

auto mln_log_set_callback(
  const mln_log_handler* handler, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::set_log_callback(handler);
  });
}

auto mln_log_clear_callback(mln_diagnostic* out_diagnostic) noexcept
  -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, []() -> mln_status {
    return mln::core::clear_log_callback();
  });
}

auto mln_log_set_async_severity_mask(
  std::uint32_t mask, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::set_log_async_severity_mask(mask);
  });
}
