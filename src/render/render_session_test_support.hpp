#pragma once

#include <atomic>

#include "maplibre_native_c/base.h"

namespace mln::core {

auto enqueue_blocking_test_render_operation(
  mln_render_session session, std::atomic_bool* entered,
  const std::atomic_bool* release, mln_operation* out_operation
) -> mln_status;

}  // namespace mln::core
