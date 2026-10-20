#include <cassert>
#include <exception>
#include <utility>

#include "execution/runtime_executor.hpp"

namespace mln::core {

namespace {

// The release completion follows destruction of the runtime's autorelease
// pool, so this lane leaves pooling to the retiring object.
auto finalizer_lane() -> RetirementLane& {
  static auto* lane = new RetirementLane{false};
  return *lane;
}

}  // namespace

RuntimeExecutor::~RuntimeExecutor() { stop(); }

auto RuntimeExecutor::start(std::function<void()> initialize) -> void {
  {
    const std::scoped_lock lock(mutex_);
    if (starting_ || run_loop_ != nullptr || worker_.joinable()) {
      throw std::runtime_error{"runtime executor is already running"};
    }
    starting_ = true;
    stopping_ = false;
    startup_error_ = nullptr;
  }

  try {
    worker_ = WorkerThread(
      [this, initialize = std::move(initialize)]() mutable noexcept -> void {
        try {
          auto loop = mln::util::RunLoop{mln::util::RunLoop::Type::New};
          {
            const std::scoped_lock lock(mutex_);
            run_loop_ = &loop;
          }
          if (initialize) {
            // Moved into a local, so the initializer and whatever it captures
            // are released once it has run rather than at thread exit.
            auto once = std::move(initialize);
            auto pooled = [&once]() -> mln_status {
              std::invoke(once);
              return MLN_STATUS_OK;
            };
            static_cast<void>(mln::c_api::with_autorelease_pool(pooled));
          }
          {
            const std::scoped_lock lock(mutex_);
            starting_ = false;
          }
          started_.notify_all();
          loop.run();
          {
            const std::scoped_lock lock(mutex_);
            run_loop_ = nullptr;
          }
        } catch (...) {
          {
            const std::scoped_lock lock(mutex_);
            startup_error_ = std::current_exception();
            run_loop_ = nullptr;
            starting_ = false;
          }
          started_.notify_all();
        }
      }
    );
  } catch (...) {
    // A thread that never started leaves no startup report behind, so a later
    // start() must not find this executor half-running.
    {
      const std::scoped_lock lock(mutex_);
      starting_ = false;
      startup_error_ = nullptr;
    }
    throw;
  }

  auto lock = std::unique_lock{mutex_};
  started_.wait(lock, [this]() noexcept -> bool { return !starting_; });
  if (startup_error_ != nullptr) {
    auto error = std::exchange(startup_error_, nullptr);
    lock.unlock();
    worker_.join();
    std::rethrow_exception(error);
  }
}

auto RuntimeExecutor::enqueue(ExecutorTask& task, bool notice) -> void {
  const auto lock = std::scoped_lock{mutex_};
  auto* loop = run_loop();
  if (loop == nullptr || stopping_) {
    throw std::runtime_error{"runtime executor is not running"};
  }
  if (!work_scheduled_ && (!notice || !draining_)) {
    loop->invoke([this] { drain_work(); });
    work_scheduled_ = true;
  }
  (notice ? notices_ : work_queue_).push(task);
}

auto RuntimeExecutor::flush_notices() noexcept -> void {
  for (;;) {
    ExecutorTask* task;
    {
      const auto lock = std::scoped_lock{mutex_};
      task = notices_.pop();
    }
    if (!task) return;
    task->run(task);
  }
}

auto RuntimeExecutor::drain_work() noexcept -> void {
  // A submission during this burst schedules the next run-loop task at once,
  // ahead of renderer mailbox messages submitted after it.
  auto burst = ExecutorQueue{};
  {
    const auto lock = std::scoped_lock{mutex_};
    burst = std::exchange(work_queue_, {});
    work_scheduled_ = false;
    draining_ = true;
  }
  while (auto* task = burst.pop()) {
    if (!task->command) flush_notices();
    auto pooled = [task]() -> mln_status {
      task->run(task);
      return MLN_STATUS_OK;
    };
    static_cast<void>(mln::c_api::with_autorelease_pool(pooled));
  }
  flush_notices();
  {
    const auto lock = std::scoped_lock{mutex_};
    draining_ = false;
  }
}

auto RuntimeExecutor::retire(RetirementTask& completion) noexcept -> void {
  assert(worker_.is_current());
  {
    const auto lock = std::scoped_lock{mutex_};
    stopping_ = true;
    run_loop_.load()->stop();
  }
  finalizer_lane().submit(completion);
}

auto RuntimeExecutor::stop() noexcept -> void {
  {
    const auto lock = std::scoped_lock{mutex_};
    if (auto* loop = run_loop(); loop != nullptr && !stopping_) {
      stopping_ = true;
      loop->invoke([this, loop] {
        drain_work();
        loop->stop();
      });
    }
  }
  if (worker_.joinable()) worker_.join();
}

}  // namespace mln::core
