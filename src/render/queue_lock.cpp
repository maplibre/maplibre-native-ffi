#include "render/queue_lock.hpp"

#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"

namespace mln::core {

QueueLock::QueueLock(const mln_queue_lock& descriptor) noexcept
    : descriptor_(descriptor) {}

QueueLock::~QueueLock() {
  if (
    accepted_ && descriptor_.release_user_data != nullptr && !process_exiting()
  ) {
    try {
      descriptor_.release_user_data(descriptor_.user_data);
    } catch (...) {
      // Host release callbacks must not unwind through the C boundary.
    }
  }
}

auto QueueLock::lock() const noexcept -> bool {
  if (process_exiting()) return false;
  try {
    descriptor_.lock(descriptor_.user_data);
  } catch (...) {
    // Host lock callbacks must not unwind through the C boundary.
  }
  return true;
}

auto QueueLock::unlock() const noexcept -> void {
  try {
    descriptor_.unlock(descriptor_.user_data);
  } catch (...) {
    // Host lock callbacks must not unwind through the C boundary.
  }
}

auto validate_queue_lock(const mln_queue_lock* lock) -> mln_status {
  if (lock->size < sizeof(mln_queue_lock)) {
    set_thread_error("mln_queue_lock.size is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if ((lock->lock == nullptr) != (lock->unlock == nullptr)) {
    set_thread_error("a queue lock needs both its lock and unlock callbacks");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (lock->lock == nullptr && lock->release_user_data != nullptr) {
    set_thread_error("a disabled queue lock must not retain user data");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  return MLN_STATUS_OK;
}

}  // namespace mln::core
