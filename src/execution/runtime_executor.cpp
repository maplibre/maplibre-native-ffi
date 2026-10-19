#include <cassert>
#include <exception>
#include <memory>
#include <utility>

#include "execution/runtime_executor.hpp"

namespace mln::core {

struct RuntimeExecutor::Finalization {
  WorkerThread worker;
  std::mutex mutex;
  std::condition_variable condition;
  RetirementTask* completion = nullptr;
  bool reserved = false;
  bool requested = false;
  bool joined = false;
};

RuntimeExecutor::~RuntimeExecutor() { stop(); }

auto RuntimeExecutor::start(std::function<void()> initialize) -> void {
  {
    const std::scoped_lock lock(mutex_);
    if (starting_ || run_loop_ != nullptr || finalization_ != nullptr) {
      throw std::runtime_error{"runtime executor is already running"};
    }
    starting_ = true;
    stopping_ = false;
    startup_error_ = nullptr;
  }

  try {
    finalization_ = std::make_shared<Finalization>();
    finalization_->worker = WorkerThread(
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
    finalization_.reset();
    throw;
  }

  auto lock = std::unique_lock{mutex_};
  started_.wait(lock, [this]() noexcept -> bool { return !starting_; });
  if (startup_error_ != nullptr) {
    auto error = std::exchange(startup_error_, nullptr);
    lock.unlock();
    finalization_->worker.join();
    finalization_.reset();
    std::rethrow_exception(error);
  }
  lock.unlock();
  try {
    WorkerThread([state = finalization_]() noexcept {
      RetirementTask* completion;
      {
        auto final_lock = std::unique_lock{state->mutex};
        state->condition.wait(final_lock, [&] { return state->requested; });
        completion = state->completion;
      }
      state->worker.join();
      {
        const auto final_lock = std::scoped_lock{state->mutex};
        state->joined = true;
      }
      state->condition.notify_all();
      // Runtime destruction drains its pool before delivering the host
      // completion. This stage therefore runs outside a retirement-task pool.
      if (completion != nullptr) completion->run(completion);
    }).detach();
    finalization_->reserved = true;
  } catch (...) {
    stop();
    throw;
  }
}

auto RuntimeExecutor::invoke_retirement(RetirementTask& task) noexcept -> void {
  const auto lock = std::scoped_lock{mutex_};
  retirement_queue_.push(task);
  retirement_wake_->send();
}

auto RuntimeExecutor::retire(RetirementTask& completion) noexcept -> void {
  assert(finalization_->worker.is_current());
  const auto lock = std::scoped_lock{mutex_};
  stopping_ = true;
  {
    const auto final_lock = std::scoped_lock{finalization_->mutex};
    assert(!finalization_->requested);
    finalization_->completion = &completion;
    finalization_->requested = true;
  }
  finalization_->condition.notify_one();
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
  const auto state = finalization_;
  if (state == nullptr) return;
  if (!state->reserved) {
    state->worker.join();
  } else {
    auto lock = std::unique_lock{state->mutex};
    state->requested = true;
    state->condition.notify_one();
    state->condition.wait(lock, [&] { return state->joined; });
  }
  finalization_.reset();
}

}  // namespace mln::core
