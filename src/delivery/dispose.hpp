#pragma once

#include <cstdint>

#include "handles/handle_table.hpp"

namespace mln::delivery {

auto dispose_owner(core::HandleKind kind, std::uint64_t handle) noexcept
  -> mln_status;

}  // namespace mln::delivery
