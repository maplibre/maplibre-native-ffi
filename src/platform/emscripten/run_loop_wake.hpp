#pragma once

#include <atomic>
#include <condition_variable>
#include <list>
#include <memory>
#include <mutex>
#include <vector>

#include <mbgl/util/chrono.hpp>

namespace mbgl::platform::emscripten {

inline std::atomic_bool run_loop_trace_enabled = false;

struct RunLoopWake {
  struct Stats {
    std::size_t readyCount = 0;
    std::size_t runnableCount = 0;
    double elapsedMs = 0.0;
  };

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
  std::mutex stats_mutex;
  Stats lastStats;

  void notify() {
    std::lock_guard lock(wake_mutex);
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

  auto processRunnables() -> mbgl::Milliseconds {
    auto const started = mbgl::Clock::now();
    auto ready = readyRunnables();
    for (auto& runnable : ready) {
      runnable->runTask();
    }
    auto const elapsed =
      std::chrono::duration<double, std::milli>(mbgl::Clock::now() - started)
        .count();
    {
      std::lock_guard lock(stats_mutex);
      lastStats = Stats{
        .readyCount = ready.size(),
        .runnableCount = runnableCount(),
        .elapsedMs = elapsed,
      };
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

  auto stats() -> Stats {
    std::lock_guard lock(stats_mutex);
    return lastStats;
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

  auto runnableCount() -> std::size_t {
    std::lock_guard lock(runnables_mutex);
    return runnables.size();
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

    auto const delay = std::chrono::duration_cast<mbgl::Milliseconds>(
      next_due - mbgl::Clock::now()
    );
    return delay < mbgl::Milliseconds::zero() ? mbgl::Milliseconds::zero()
                                              : delay;
  }
};

}  // namespace mbgl::platform::emscripten
