#pragma once

#include <utility>

namespace mln::core {

// A host callback that must not call back into the C API.
//
// Some host callbacks run while native code holds a lock that a C API call
// can take again: MapLibre Native's log mutex, which is not recursive, or a
// Vulkan queue's submission mutex. A call from there would deadlock or have
// undefined behavior. The callback site holds one of these scopes across the
// call, and every status-returning C entry point on that thread returns
// MLN_STATUS_INVALID_STATE with the scope's diagnostic until it ends.
class ForbidReentry final {
 public:
  explicit ForbidReentry(const char* diagnostic) noexcept
      : previous_(std::exchange(active_, diagnostic)) {}
  ForbidReentry(const ForbidReentry&) = delete;
  ForbidReentry(ForbidReentry&&) = delete;
  auto operator=(const ForbidReentry&) -> ForbidReentry& = delete;
  auto operator=(ForbidReentry&&) -> ForbidReentry& = delete;
  ~ForbidReentry() { active_ = previous_; }

  // The diagnostic of the innermost scope on this thread, or null outside one.
  [[nodiscard]] static auto active() noexcept -> const char* { return active_; }

 private:
  static inline thread_local const char* active_ = nullptr;
  const char* previous_;
};

}  // namespace mln::core
