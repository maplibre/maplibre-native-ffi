#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <utility>

namespace mln::core {

// The condition that a render session's core worker waits on for driver
// work, and that other threads wait on for the worker. Wait while holding the
// session's control mutex, as with a std::condition_variable.
class WorkerCondition {
 public:
  void notify_one() noexcept;
  void notify_all() noexcept;

  template <typename Predicate>
  void wait(std::unique_lock<std::mutex>& lock, Predicate predicate) {
    condition_.wait(lock, std::move(predicate));
  }

#if defined(__EMSCRIPTEN__)
  // The notifications so far. Read it under the session's control mutex,
  // after finding nothing to do, and pass it to resume_after_notification().
  [[nodiscard]] auto notifications() const noexcept -> int32_t;

  // Calls `resume(user_data)` from the calling thread's event loop once a
  // notification after `seen` arrives, or right away if one already has. It
  // returns without waiting, for a core worker that runs from its event loop.
  // Only one such wait may be pending.
  void resume_after_notification(
    int32_t seen, void (*resume)(void*), void* user_data
  );
#endif

 private:
  std::condition_variable condition_;
  // Counts notifications, for a waiter on the event loop, which cannot block
  // on condition_.
  std::atomic<int32_t> notifications_{0};
#if defined(__EMSCRIPTEN__)
  void (*resume_)(void*) = nullptr;
  void* resume_data_ = nullptr;
#endif
};

}  // namespace mln::core
