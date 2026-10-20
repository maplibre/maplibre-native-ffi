#pragma once

#include <condition_variable>
#include <mutex>

#include "c_api/autorelease_pool.hpp"
#include "execution/worker_thread.hpp"

namespace mln::core {

// Owned by the retiring object. A queue stops accessing the node before calling
// run, so the callback may destroy its owner or enqueue the node elsewhere.
struct RetirementTask {
  RetirementTask* next = nullptr;
  void (*run)(RetirementTask*) noexcept = nullptr;
  void* context = nullptr;
};

// The caller serializes access to this intrusive queue.
class RetirementQueue {
 public:
  auto push(RetirementTask& task) noexcept -> void {
    if (last_ != nullptr)
      last_->next = &task;
    else
      first_ = &task;
    last_ = &task;
  }

  auto pop() noexcept -> RetirementTask* {
    auto* task = first_;
    if (task != nullptr) {
      first_ = task->next;
      if (first_ == nullptr) last_ = nullptr;
      task->next = nullptr;
    }
    return task;
  }

  [[nodiscard]] auto empty() const noexcept -> bool {
    return first_ == nullptr;
  }

 private:
  RetirementTask* first_ = nullptr;
  RetirementTask* last_ = nullptr;
};

inline auto run_retirement_task(RetirementTask* task) noexcept -> void {
  auto pooled = [task]() -> mln_status {
    task->run(task);
    return MLN_STATUS_OK;
  };
  static_cast<void>(mln::c_api::with_autorelease_pool(pooled));
}

// A shared lane for cleanup that must run outside an object's worker.
class RetirementLane {
 public:
  // The lane runs MapLibre destructors, so it takes a worker-sized stack.
  // Throws std::system_error when the thread cannot be created.
  explicit RetirementLane(bool autorelease_pool = true) {
    WorkerThread([this, autorelease_pool]() noexcept {
      for (;;) {
        RetirementTask* task;
        {
          auto lock = std::unique_lock{mutex_};
          condition_.wait(lock, [this] { return !queue_.empty(); });
          task = queue_.pop();
        }
        if (autorelease_pool)
          run_retirement_task(task);
        else
          task->run(task);
      }
    }).detach();
  }

  auto submit(RetirementTask& task) noexcept -> void {
    {
      const auto lock = std::scoped_lock{mutex_};
      queue_.push(task);
    }
    condition_.notify_one();
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  RetirementQueue queue_;
};

}  // namespace mln::core
