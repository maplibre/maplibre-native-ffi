#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <utility>

#include "diagnostics/diagnostics.hpp"
#include "maplibre_native_c.h"

namespace mln::core {

// Coordinates handle lookup, committed submissions, live children, and close.
// Registry locks are never held while waiting for a submission or child.
class ControlState {
 public:
  [[nodiscard]] auto acquire(bool child_retirement = false) -> bool {
    const std::scoped_lock lock(mutex_);
    if (closing_ && !(child_retirement && children_ != 0)) {
      set_thread_error("handle is closing");
      return false;
    }
    ++submissions_;
    return true;
  }

  auto release() noexcept -> void { release_count(false); }

  [[nodiscard]] auto retain_child() -> bool {
    const std::scoped_lock lock(mutex_);
    if (closing_) {
      set_thread_error("handle is closing");
      return false;
    }
    ++children_;
    return true;
  }

  auto release_child() noexcept -> void { release_count(true); }

  [[nodiscard]] auto begin_close(bool defer_children = false) -> mln_status {
    const std::scoped_lock lock(mutex_);
    if (closing_) {
      set_thread_error("handle is already closing");
      return MLN_STATUS_INVALID_STATE;
    }
    // A child being created counts here from acceptance, so it is pending
    // rather than live until its handle exists.
    if (!defer_children && children_ != 0) {
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
    drained_ = {};
  }

  // Runs once, outside the control lock, after close has begun and every
  // submission lease and child have retired.
  auto notify_when_drained(std::function<void()> callback) noexcept -> void {
    auto invoke_now = false;
    {
      const std::scoped_lock lock(mutex_);
      if (closing_ && submissions_ == 0 && children_ == 0) {
        invoke_now = true;
      } else {
        drained_ = std::move(callback);
      }
    }
    if (invoke_now && callback) {
      try {
        callback();
      } catch (...) {
        // A drained callback cannot reopen a handle that finished closing.
      }
    }
  }

  auto notify_when_drained(
    void (*callback)(void*) noexcept, void* context
  ) noexcept -> void {
    bool ready;
    {
      const auto lock = std::scoped_lock{mutex_};
      ready = closing_ && submissions_ == 0 && children_ == 0;
      if (!ready) {
        drained_callback_ = callback;
        drained_context_ = context;
      }
    }
    if (ready) callback(context);
  }

  // Blocks until no submission lease is outstanding. When one is, it first
  // calls `before_wait` once, outside the control lock.
  auto wait_for_submissions(void (*before_wait)() noexcept = nullptr) noexcept
    -> void {
    auto lock = std::unique_lock{mutex_};
    if (before_wait != nullptr && submissions_ != 0) {
      lock.unlock();
      before_wait();
      lock.lock();
    }
    condition_.wait(lock, [this]() noexcept -> bool {
      return submissions_ == 0;
    });
  }

  [[nodiscard]] auto is_closing() const noexcept -> bool {
    const std::scoped_lock lock(mutex_);
    return closing_;
  }

 private:
  auto release_count(bool child) noexcept -> void {
    auto drained = std::function<void()>{};
    auto callback = static_cast<void (*)(void*) noexcept>(nullptr);
    void* context = nullptr;
    {
      const std::scoped_lock lock(mutex_);
      auto& count = child ? children_ : submissions_;
      if (count > 0) --count;
      if (closing_ && submissions_ == 0 && children_ == 0) {
        drained = std::move(drained_);
        callback = std::exchange(drained_callback_, nullptr);
        context = drained_context_;
      }
    }
    condition_.notify_all();
    if (callback != nullptr) callback(context);
    if (drained) {
      try {
        drained();
      } catch (...) {
        // A drained callback cannot reopen a handle that finished closing.
      }
    }
  }

  mutable std::mutex mutex_;
  std::condition_variable condition_;
  std::size_t submissions_ = 0;
  std::size_t children_ = 0;
  bool closing_ = false;
  std::function<void()> drained_;
  void (*drained_callback_)(void*) noexcept = nullptr;
  void* drained_context_ = nullptr;
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
