#include "testing/render_clock.hpp"

namespace mln::testing {

namespace detail {
std::atomic<std::int64_t> render_clock_offset_ns{0};
}  // namespace detail

void advance_render_clock(std::chrono::nanoseconds duration) noexcept {
  detail::render_clock_offset_ns.fetch_add(
    duration.count(), std::memory_order_relaxed
  );
}

}  // namespace mln::testing
