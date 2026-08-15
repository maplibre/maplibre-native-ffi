#pragma once

#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <utility>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

// Coordinates handle lookup, committed submissions, child reservations, and
// close. Registry locks are never held while waiting for a submission or child.
class ControlState {
 public:
  [[nodiscard]] auto acquire() -> bool {
    const std::scoped_lock lock(mutex_);
    if (closing_) {
      set_thread_error("handle is closing");
      return false;
    }
    ++submissions_;
    return true;
  }

  auto release() noexcept -> void {
    {
      const std::scoped_lock lock(mutex_);
      if (submissions_ > 0) {
        --submissions_;
      }
    }
    condition_.notify_all();
  }

  [[nodiscard]] auto reserve_child() -> bool {
    const std::scoped_lock lock(mutex_);
    if (closing_) {
      set_thread_error("handle is closing");
      return false;
    }
    ++child_reservations_;
    return true;
  }

  auto commit_child() noexcept -> void {
    const std::scoped_lock lock(mutex_);
    if (child_reservations_ > 0) {
      --child_reservations_;
    }
    ++children_;
  }

  auto abandon_child_reservation() noexcept -> void {
    {
      const std::scoped_lock lock(mutex_);
      if (child_reservations_ > 0) {
        --child_reservations_;
      }
    }
    condition_.notify_all();
  }

  auto release_child() noexcept -> void {
    {
      const std::scoped_lock lock(mutex_);
      if (children_ > 0) {
        --children_;
      }
    }
    condition_.notify_all();
  }

  [[nodiscard]] auto begin_close() -> mln_status {
    const std::scoped_lock lock(mutex_);
    if (closing_) {
      set_thread_error("handle is already closing");
      return MLN_STATUS_INVALID_STATE;
    }
    if (children_ != 0 || child_reservations_ != 0) {
      set_thread_error("handle still owns live or pending children");
      return MLN_STATUS_INVALID_STATE;
    }
    closing_ = true;
    return MLN_STATUS_OK;
  }
  // Rolls back a close that failed before any close work became reachable.
  auto abort_close() noexcept -> void {
    const std::scoped_lock lock(mutex_);
    closing_ = false;
  }

  auto wait_for_submissions() noexcept -> void {
    auto lock = std::unique_lock{mutex_};
    condition_.wait(lock, [this]() noexcept -> bool {
      return submissions_ == 0;
    });
  }

  [[nodiscard]] auto is_closing() const noexcept -> bool {
    const std::scoped_lock lock(mutex_);
    return closing_;
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::size_t submissions_ = 0;
  std::size_t children_ = 0;
  std::size_t child_reservations_ = 0;
  bool closing_ = false;
};

class ControlLease {
 public:
  ControlLease() = default;
  explicit ControlLease(ControlState* state) : state_(state) {}
  ControlLease(const ControlLease&) = delete;
  ControlLease(ControlLease&& other) noexcept
      : state_(std::exchange(other.state_, nullptr)) {}
  auto operator=(const ControlLease&) -> ControlLease& = delete;
  auto operator=(ControlLease&& other) noexcept -> ControlLease& {
    if (this != &other) {
      reset();
      state_ = std::exchange(other.state_, nullptr);
    }
    return *this;
  }
  ~ControlLease() { reset(); }

  auto reset() noexcept -> void {
    if (state_ != nullptr) {
      state_->release();
      state_ = nullptr;
    }
  }

 private:
  ControlState* state_ = nullptr;
};

}  // namespace mln::core
