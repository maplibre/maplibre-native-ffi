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

  // Waits as wait() does, on the core worker. In a browser WebGPU build the
  // worker returns to its event loop while it waits, because the browser
  // delivers the results of the WebGPU work it submitted only there.
  template <typename Predicate>
  void wait_on_core_worker(
    std::unique_lock<std::mutex>& lock, Predicate predicate
  ) {
    while (!predicate()) wait_on_core_worker_once(lock);
  }

 private:
  void wait_on_core_worker_once(std::unique_lock<std::mutex>& lock);

  std::condition_variable condition_;
  // Counts notifications, for a waiter on the event loop, which cannot block
  // on condition_.
  std::atomic<int32_t> notifications_{0};
};

}  // namespace mln::core
