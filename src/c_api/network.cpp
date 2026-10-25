#define MLN_BUILDING_C

#include "c_api/boundary.hpp"
#include "maplibre_native_c.h"
#include "resources/network_status.hpp"

auto mln_network_get_status(
  mln_network_status* out_status, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::network_status_get(out_status);
  });
}

auto mln_network_set_status(
  mln_network_status status, mln_diagnostic* out_diagnostic
) noexcept -> mln_status {
  return mln::c_api::status_boundary(out_diagnostic, [&]() -> mln_status {
    return mln::core::network_status_set(status);
  });
}
