#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>

// The clock render sessions measure frame demand deadlines against. It is the
// steady clock plus an offset that only the internal test suite advances, so
// a case can pass a demand's deadline without waiting for it. Like the sync
// points, it is compiled into every build and never exported.
namespace mln::testing {

namespace detail {
extern std::atomic<std::int64_t> render_clock_offset_ns;
}  // namespace detail

inline auto render_clock_now() noexcept
  -> std::chrono::steady_clock::time_point {
  return std::chrono::steady_clock::now() +
         std::chrono::nanoseconds{
           detail::render_clock_offset_ns.load(std::memory_order_relaxed)
         };
}

// Moves the render clock forward for every session in the process. The offset
// only grows and lasts for the process, which single-process runners share
// across every case, so a case may rely on how far the clock moves after its
// own requests but never on the clock's value.
void advance_render_clock(std::chrono::nanoseconds duration) noexcept;

}  // namespace mln::testing
