#pragma once

#include <condition_variable>
#include <mutex>

#include "execution/worker_thread.hpp"

namespace mln::core {

// Owned by the retiring object. A queue stops accessing the node before calling
// run, so the callback may destroy its owner or enqueue the node elsewhere.
struct RetirementTask {
  RetirementTask* next = nullptr;
  void (*run)(RetirementTask*) noexcept = nullptr;
  void* context = nullptr;
};

// Reserve this lane before publishing any object that needs it. Queueing an
// embedded node requires neither allocation nor a new thread.
class RetirementLane {
 public:
  // The lane runs MapLibre destructors, so it takes a worker-sized stack.
  // Throws std::system_error when the thread cannot be created.
  RetirementLane() {
    WorkerThread([this]() noexcept {
      for (;;) {
        RetirementTask* task;
        {
          auto lock = std::unique_lock{mutex_};
          condition_.wait(lock, [this] { return first_ != nullptr; });
          task = first_;
          first_ = task->next;
          if (first_ == nullptr) last_ = nullptr;
          task->next = nullptr;
        }
        task->run(task);
      }
    }).detach();
  }

  auto submit(RetirementTask& task) noexcept -> void {
    {
      const auto lock = std::scoped_lock{mutex_};
      if (last_ != nullptr)
        last_->next = &task;
      else
        first_ = &task;
      last_ = &task;
    }
    condition_.notify_one();
  }

 private:
  std::mutex mutex_;
  std::condition_variable condition_;
  RetirementTask* first_ = nullptr;
  RetirementTask* last_ = nullptr;
};

}  // namespace mln::core
