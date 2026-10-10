#pragma once

#include "maplibre_native_c/base.h"
#include "maplibre_native_c/render_target.h"

namespace mln::core {

// The host's lock on a graphics queue that a session shares with it.
//
// A session holds one, and its backend's queue registration shares it while
// the backend can call it. Each holder keeps the user data alive, so the last
// one to let go runs the host's release, after every call has returned.
class QueueLock final {
 public:
  explicit QueueLock(const mln_queue_lock& descriptor) noexcept;
  QueueLock(const QueueLock&) = delete;
  QueueLock(QueueLock&&) = delete;
  auto operator=(const QueueLock&) -> QueueLock& = delete;
  auto operator=(QueueLock&&) -> QueueLock& = delete;
  ~QueueLock();

  // Whether the host passed callbacks. A disabled lock locks nothing.
  [[nodiscard]] auto enabled() const noexcept -> bool {
    return descriptor_.lock != nullptr;
  }

  // Hands ownership of the user data to this object, so that destruction
  // releases it. Before this, destruction leaves the user data to the host.
  auto accept() noexcept -> void { accepted_ = true; }

  // Calls the host's lock, unless the process has begun to exit, and reports
  // whether it did. Once exit begins, the host's runtime may be gone, so no
  // new lock starts; the caller then leaves the queue alone.
  [[nodiscard]] auto lock() const noexcept -> bool;
  // Gives back a lock that lock() took, even once exit has begun, so that a
  // host thread waiting for the queue is never left waiting.
  auto unlock() const noexcept -> void;

 private:
  mln_queue_lock descriptor_{};
  bool accepted_ = false;
};

// Holds a queue lock, if there is one, for one call on the queue.
class QueueLockGuard final {
 public:
  explicit QueueLockGuard(const QueueLock* lock) noexcept
      : lock_(lock != nullptr && lock->lock() ? lock : nullptr),
        refused_(lock != nullptr && lock_ == nullptr) {}
  QueueLockGuard(const QueueLockGuard&) = delete;
  QueueLockGuard(QueueLockGuard&&) = delete;
  auto operator=(const QueueLockGuard&) -> QueueLockGuard& = delete;
  auto operator=(QueueLockGuard&&) -> QueueLockGuard& = delete;
  ~QueueLockGuard() {
    if (lock_ != nullptr) lock_->unlock();
  }

  // Whether the queue has a lock that this guard could not take, because the
  // process has begun to exit. The call must then not reach the queue.
  [[nodiscard]] auto refused() const noexcept -> bool { return refused_; }

 private:
  // The lock this guard took, or null.
  const QueueLock* lock_;
  bool refused_;
};

// Checks the descriptor's shape. A backend decides whether it supports an
// enabled lock.
auto validate_queue_lock(const mln_queue_lock* lock) -> mln_status;

}  // namespace mln::core
