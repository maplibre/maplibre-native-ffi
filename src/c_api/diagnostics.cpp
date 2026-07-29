#define MLN_BUILDING_C

#include <atomic>
#include <cstdint>

#include "diagnostics/diagnostics.hpp"

#include "maplibre_native_c.h"

auto mln_thread_last_error_message(void) noexcept -> const char* {
  return mln::core::thread_last_error_message();
}

namespace {

// A counter rather than the platform thread id: those are not uniformly
// available as an integer, and only comparison is promised. Zero is never
// handed out, so a host can use it as "not yet observed".
auto next_thread_token() noexcept -> uint64_t {
  static std::atomic<uint64_t> counter{0};
  return counter.fetch_add(1, std::memory_order_relaxed) + 1;
}

}  // namespace

auto mln_thread_token(void) noexcept -> uint64_t {
  static thread_local const uint64_t token = next_thread_token();
  return token;
}
