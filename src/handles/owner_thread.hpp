#pragma once

#include <atomic>
#include <cstdint>

namespace mln::core {

// Identifies the thread that owns a handle. The platform recycles thread ids,
// so a runtime, map, or render session that outlives its owner thread would
// otherwise accept calls from a later thread with the same id. This token is
// unique for the life of the process instead.
using OwnerThreadToken = std::uint64_t;

// Zero is the token of no thread, so a handle whose owner was never recorded
// matches no caller.
inline constexpr OwnerThreadToken kNoOwnerThread = 0;

inline auto current_owner_thread() noexcept -> OwnerThreadToken {
  static std::atomic<OwnerThreadToken> next{1};
  thread_local const OwnerThreadToken token =
    next.fetch_add(1, std::memory_order_relaxed);
  return token;
}

}  // namespace mln::core
