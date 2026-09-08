#pragma once

#include <chrono>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include <mln/util/chrono.hpp>

namespace mln::platform::emscripten {

struct RunLoopWake {
  class Runnable {
   public:
    virtual ~Runnable() = default;

    virtual auto dueTime() const -> mln::TimePoint = 0;
    virtual void runTask() = 0;
    virtual auto countsForWaitForEmpty() const -> bool { return false; }
  };

  std::mutex wake_mutex;
  std::condition_variable cv;
  bool notified = false;

  std::mutex runnables_mutex;
  std::list<std::shared_ptr<Runnable>> runnables;

  void notify() {
    const std::lock_guard lock(wake_mutex);
    notified = true;
    cv.notify_one();
  }

  void addRunnable(std::shared_ptr<Runnable> runnable) {
    {
      std::lock_guard lock(runnables_mutex);
      runnables.push_back(std::move(runnable));
    }
    notify();
  }

  void removeRunnable(const std::shared_ptr<Runnable>& runnable) {
    std::lock_guard lock(runnables_mutex);
    runnables.remove(runnable);
  }

  auto processRunnables() -> mln::Milliseconds {
    auto ready = readyRunnables();
    for (auto& runnable : ready) {
      runnable->runTask();
    }
    return nextDelay();
  }

  auto emptyForWaitForEmpty() -> bool {
    std::lock_guard lock(runnables_mutex);
    for (auto& runnable : runnables) {
      if (runnable->countsForWaitForEmpty()) {
        return false;
      }
    }
    return true;
  }

 private:
  auto readyRunnables() -> std::vector<std::shared_ptr<Runnable>> {
    auto ready = std::vector<std::shared_ptr<Runnable>>{};
    auto const now = mln::Clock::now();
    std::lock_guard lock(runnables_mutex);
    for (auto& runnable : runnables) {
      if (runnable->dueTime() <= now) {
        ready.push_back(runnable);
      }
    }
    return ready;
  }

  auto nextDelay() -> mln::Milliseconds {
    std::lock_guard lock(runnables_mutex);
    if (runnables.empty()) {
      return mln::Milliseconds(-1);
    }

    auto next_due = mln::TimePoint::max();
    for (auto& runnable : runnables) {
      auto const due_time = runnable->dueTime();
      if (due_time < next_due) {
        next_due = due_time;
      }
    }

    if (next_due == mln::TimePoint::max()) {
      return mln::Milliseconds(-1);
    }

    // Rounding up keeps a sub-millisecond remainder a positive delay, so zero
    // means due rather than nearly due.
    auto const delay =
      std::chrono::ceil<mln::Milliseconds>(next_due - mln::Clock::now());
    return delay < mln::Milliseconds::zero() ? mln::Milliseconds::zero()
                                             : delay;
  }
};

}  // namespace mln::platform::emscripten
