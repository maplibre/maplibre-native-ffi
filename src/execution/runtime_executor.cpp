#include <cassert>
#include <exception>
#include <memory>
#include <utility>

#include "execution/runtime_executor.hpp"

namespace mln::core {

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
          auto retirement_wake =
            std::make_unique<RetirementWake>([this]() noexcept {
              drain_retirement();
            });
          {
            const auto lock = std::scoped_lock{mutex_};
            retirement_wake_ = retirement_wake.get();
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
            retirement_wake_ = nullptr;
            run_loop_ = nullptr;
          }
        } catch (...) {
          {
            const std::scoped_lock lock(mutex_);
            startup_error_ = std::current_exception();
            retirement_wake_ = nullptr;
            run_loop_ = nullptr;
            starting_ = false;
          }
          started_.notify_all();
        }
        if (auto* completion = retirement_completion_; completion != nullptr) {
          worker_.detach();
          // The callback may destroy this executor. No worker code accesses
          // its state after this point.
          run_retirement_task(completion);
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
    if (worker_.joinable()) {
      worker_.join();
    }
    std::rethrow_exception(error);
  }
}

auto RuntimeExecutor::invoke_retirement(RetirementTask& task) noexcept -> void {
  const auto lock = std::scoped_lock{mutex_};
  retirement_queue_.push(task);
  retirement_wake_->send();
}

auto RuntimeExecutor::retire(RetirementTask& completion) noexcept -> void {
  assert(worker_.is_current());
  const auto lock = std::scoped_lock{mutex_};
  assert(retirement_completion_ == nullptr);
  retirement_completion_ = &completion;
  stopping_ = true;
}

auto RuntimeExecutor::drain_retirement() noexcept -> void {
  for (;;) {
    RetirementTask* task;
    {
      const auto lock = std::scoped_lock{mutex_};
      task = retirement_queue_.pop();
      if (task == nullptr) {
        if (stopping_) run_loop_.load()->stop();
        return;
      }
    }
    run_retirement_task(task);
  }
}

auto RuntimeExecutor::stop() noexcept -> void {
  {
    const auto lock = std::scoped_lock{mutex_};
    if (run_loop_ != nullptr && retirement_wake_ != nullptr) {
      stopping_ = true;
      retirement_wake_->send();
    }
  }
  if (worker_.joinable()) {
    // The owner joins from outside the worker before destroying this object.
    // The worker still accesses executor state after its run loop stops.
    if (worker_.is_current()) std::terminate();
    worker_.join();
  }
}

}  // namespace mln::core
