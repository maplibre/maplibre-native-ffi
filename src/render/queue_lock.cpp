#include "render/queue_lock.hpp"

#include "diagnostics/diagnostics.hpp"
#include "execution/forbid_reentry.hpp"
#include "execution/process_exit.hpp"

namespace mln::core {

namespace {

// The callbacks run inside the queue's submission mutex, which a session on
// the same queue takes again to submit.
constexpr auto queue_lock_reentry_diagnostic =
  "the C API cannot be called from a queue lock callback";

}  // namespace

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
  const auto forbid = ForbidReentry{queue_lock_reentry_diagnostic};
  try {
    descriptor_.lock(descriptor_.user_data);
  } catch (...) {
    // Host lock callbacks must not unwind through the C boundary.
    return false;
  }
  return true;
}

auto QueueLock::unlock() const noexcept -> void {
  const auto forbid = ForbidReentry{queue_lock_reentry_diagnostic};
  try {
    descriptor_.unlock(descriptor_.user_data);
  } catch (...) {
    // Host lock callbacks must not unwind through the C boundary.
  }
}

auto validate_queue_lock(const mln_queue_lock* lock) -> mln_status {
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
