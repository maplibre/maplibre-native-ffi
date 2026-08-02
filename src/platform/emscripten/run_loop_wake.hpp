#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include <mbgl/util/chrono.hpp>

namespace mbgl::platform::emscripten {

struct RunLoopWake {
  class Runnable {
   public:
    virtual ~Runnable() = default;

    virtual auto dueTime() const -> mbgl::TimePoint = 0;
    virtual void runTask() = 0;
    virtual auto countsForWaitForEmpty() const -> bool { return false; }
  };

  std::mutex wake_mutex;
  std::condition_variable cv;
  bool notified = false;

  std::mutex runnables_mutex;
  std::list<std::shared_ptr<Runnable>> runnables;

  // Reports readiness to whatever hosts this loop, beyond the condition
  // variable run() parks on. A host that drives runOnce() from
  // mln_runtime_pump() instead parks on the runtime's wake state, and only
  // RunLoop::push() reports queued work there -- timers and async tasks, which
  // is how a fetch response comes back, arrive as runnables and would otherwise
  // leave such a host asleep until its timeout, or forever if it passed a
  // negative one. RunLoop sets this; see its constructor.
  std::function<void()> platform_wake;

  void notify() {
    {
      std::lock_guard lock(wake_mutex);
      notified = true;
      cv.notify_one();
    }
    // Outside the wake lock: this ends up taking the host's own wake lock, and
    // push() reaches it while holding the run loop mutex.
    if (platform_wake) {
      platform_wake();
    }
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

  auto processRunnables() -> mbgl::Milliseconds {
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
    auto const now = mbgl::Clock::now();
    std::lock_guard lock(runnables_mutex);
    for (auto& runnable : runnables) {
      if (runnable->dueTime() <= now) {
        ready.push_back(runnable);
      }
    }
    return ready;
  }

  auto nextDelay() -> mbgl::Milliseconds {
    std::lock_guard lock(runnables_mutex);
    if (runnables.empty()) {
      return mbgl::Milliseconds(-1);
    }

    auto next_due = mbgl::TimePoint::max();
    for (auto& runnable : runnables) {
      auto const due_time = runnable->dueTime();
      if (due_time < next_due) {
        next_due = due_time;
      }
    }

    if (next_due == mbgl::TimePoint::max()) {
      return mbgl::Milliseconds(-1);
    }

    // Rounding up keeps a remainder under a millisecond a positive delay, so
    // zero means due, not nearly due. A caller that only waits for what it gets
    // back here -- see RunLoop::runOnce() -- would otherwise wait for nothing.
    auto const delay =
      std::chrono::ceil<mbgl::Milliseconds>(next_due - mbgl::Clock::now());
    return delay < mbgl::Milliseconds::zero() ? mbgl::Milliseconds::zero()
                                              : delay;
  }
};

}  // namespace mbgl::platform::emscripten
