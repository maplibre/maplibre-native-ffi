#include "wake/wake.hpp"

#include "diagnostics/diagnostics.hpp"

namespace mln::core {

Wake::Wake(const mln_wake& descriptor) noexcept : descriptor_(descriptor) {}

Wake::~Wake() {
  auto descriptor = mln_wake{};
  auto accepted = false;
  {
    auto lock = std::unique_lock{mutex_};
    closing_ = true;
    condition_.wait(lock, [this]() noexcept { return in_flight_ == 0; });
    descriptor = descriptor_;
    accepted = accepted_;
    descriptor_ = {};
  }
  if (accepted && descriptor.release_user_data != nullptr) {
    try {
      descriptor.release_user_data(descriptor.user_data);
    } catch (...) {
      // Host release callbacks must not unwind through the C boundary.
    }
  }
}

auto Wake::accept() noexcept -> void {
  const auto lock = std::scoped_lock{mutex_};
  accepted_ = true;
}

auto Wake::reject() noexcept -> void {
  const auto lock = std::scoped_lock{mutex_};
  closing_ = true;
  descriptor_ = {};
}

auto Wake::notify() noexcept -> void {
  auto callback = mln_wake_callback{};
  auto* user_data = static_cast<void*>(nullptr);
  {
    const auto lock = std::scoped_lock{mutex_};
    if (!accepted_ || closing_ || descriptor_.callback == nullptr) return;
    ++in_flight_;
    callback = descriptor_.callback;
    user_data = descriptor_.user_data;
  }
  try {
    callback(user_data);
  } catch (...) {
    // Host wake callbacks must not unwind through the C boundary.
  }
  {
    const auto lock = std::scoped_lock{mutex_};
    --in_flight_;
  }
  condition_.notify_all();
}

auto validate_wake(const mln_wake* wake) -> mln_status {
  if (wake == nullptr) {
    set_thread_error("wake must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (wake->size < sizeof(mln_wake)) {
    set_thread_error("mln_wake.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (wake->callback == nullptr && wake->release_user_data != nullptr) {
    set_thread_error("a disabled wake must not retain user data");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace mln::core
