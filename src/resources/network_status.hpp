#pragma once

#include "maplibre_native_c.h"

namespace mln::core {

auto network_status_get(mln_network_status* out_status) -> mln_status;
auto network_status_set(mln_network_status status) -> mln_status;

}  // namespace mln::core
