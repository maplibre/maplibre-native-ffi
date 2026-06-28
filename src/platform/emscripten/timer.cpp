#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <utility>

#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/timer.hpp>

#include "run_loop_wake.hpp"

namespace mbgl {
namespace util {

class TimerState : public platform::emscripten::RunLoopWake::Runnable,
                   public std::enable_shared_from_this<TimerState> {
 public:
  TimerState()
      : wake(
          static_cast<platform::emscripten::RunLoopWake*>(
            RunLoop::getLoopHandle()
          )
        ) {}

  void start(Duration timeout, Duration repeat_, std::function<void()>&& cb_) {
    stop();
    cb = std::move(cb_);
    repeat = repeat_;
    due =
      timeout == Duration::max() ? TimePoint::max() : Clock::now() + timeout;
    active = true;
    wake->addRunnable(shared_from_this());
  }

  void stop() {
    active = false;
    wake->removeRunnable(shared_from_this());
    cb = nullptr;
  }

  auto dueTime() const -> TimePoint override { return due; }

  void runTask() override {
    if (!active) {
      return;
    }

    if (repeat == Duration::zero()) {
      active = false;
      wake->removeRunnable(shared_from_this());
    } else {
      due = Clock::now() + repeat;
      wake->notify();
    }

    if (cb) {
      cb();
    }
  }

 private:
  platform::emscripten::RunLoopWake* wake;
  TimePoint due = TimePoint::max();
  Duration repeat = Duration::zero();

  std::function<void()> cb;
  std::atomic<bool> active{false};
};

class Timer::Impl {
 public:
  Impl() : state(std::make_shared<TimerState>()) {}

  ~Impl() { state->stop(); }

  void start(Duration timeout, Duration repeat, std::function<void()>&& cb) {
    state->start(timeout, repeat, std::move(cb));
  }

  void stop() { state->stop(); }

 private:
  std::shared_ptr<TimerState> state;
};

Timer::Timer() : impl(std::make_unique<Impl>()) {}

Timer::~Timer() = default;

void Timer::start(
  Duration timeout, Duration repeat, std::function<void()>&& cb
) {
  impl->start(timeout, repeat, std::move(cb));
}

void Timer::stop() { impl->stop(); }

}  // namespace util
}  // namespace mbgl
