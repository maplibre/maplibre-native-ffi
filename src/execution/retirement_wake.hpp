#pragma once

#include <functional>
#include <memory>
#include <utility>

#include <mln/util/async_task.hpp>
#include <mln/util/run_loop.hpp>

// Android and the browser run the library's own run loop; see
// src/platform/run_loop.
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
#include <atomic>

#include "platform/run_loop/run_loop_wake.hpp"
#endif

namespace mln::core {

// The library's run loop allocates a list entry on each AsyncTask send.
// Retirement reserves its registration so sending only signals existing state.
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
class RetirementWake {
  struct State : mln::platform::RunLoopWake::Runnable {
    explicit State(std::function<void()> callback)
        : callback(std::move(callback)) {}
    auto dueTime() const -> mln::TimePoint override {
      return queued.load() ? mln::TimePoint::min() : mln::TimePoint::max();
    }
    void runTask() override {
      if (queued.exchange(false)) callback();
    }
    auto countsForWaitForEmpty() const -> bool override {
      return queued.load();
    }
    std::function<void()> callback;
    std::atomic_bool queued = false;
  };

 public:
  explicit RetirementWake(std::function<void()> callback)
      : wake_(
          static_cast<mln::platform::RunLoopWake*>(
            mln::util::RunLoop::getLoopHandle()
          )
        ),
        state_(std::make_shared<State>(std::move(callback))) {
    wake_->addRunnable(state_);
  }
  ~RetirementWake() { wake_->removeRunnable(state_); }
  auto send() noexcept -> void {
    state_->queued = true;
    wake_->notify();
  }

 private:
  mln::platform::RunLoopWake* wake_;
  std::shared_ptr<State> state_;
};
#else
using RetirementWake = mln::util::AsyncTask;
#endif

}  // namespace mln::core
