#pragma once

#include <cstdint>

#include "maplibre_native_c.h"

namespace mln::core {

auto network_get_status(std::uint32_t* out_status) -> mln_status;
auto network_set_status(std::uint32_t status) -> mln_status;

}  // namespace mln::core
