#include <atomic>
#include <cstdlib>

#include "execution/process_exit.hpp"

namespace mln::core {

namespace {

std::atomic<bool> exiting{false};

auto mark_process_exiting() noexcept -> void {
  exiting.store(true, std::memory_order_release);
}

}  // namespace

auto watch_process_exit() noexcept -> void {
  // A failed registration leaves callbacks running through exit, as they did
  // before the library watched for it.
  [[maybe_unused]] static const auto registered =
    std::atexit(mark_process_exiting) == 0;
}

auto process_exiting() noexcept -> bool {
  return exiting.load(std::memory_order_acquire);
}

}  // namespace mln::core
