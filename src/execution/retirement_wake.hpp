#pragma once

#include <functional>
#include <memory>
#include <utility>

#include <mln/util/async_task.hpp>
#include <mln/util/run_loop.hpp>

#if defined(__ANDROID__)
#include <cerrno>
#include <cstdint>
#include <stdexcept>

#include <sys/eventfd.h>
#include <unistd.h>
#elif defined(__EMSCRIPTEN__)
#include <atomic>

#include "platform/emscripten/run_loop_wake.hpp"
#endif

namespace mln::core {

// Android and browser AsyncTask implementations allocate a list entry on send.
// Retirement reserves its registration so sending only signals existing state.
#if defined(__ANDROID__)
class RetirementWake {
 public:
  explicit RetirementWake(std::function<void()> callback)
      : loop_(mln::util::RunLoop::Get()),
        fd_(eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC)) {
    if (fd_ < 0) throw std::runtime_error{"retirement eventfd creation failed"};
    try {
      loop_->addWatch(
        fd_, mln::util::RunLoop::Event::Read,
        [callback = std::move(callback)](int fd, mln::util::RunLoop::Event) {
          std::uint64_t count;
          while (read(fd, &count, sizeof(count)) < 0 && errno == EINTR) {
          }
          callback();
        }
      );
    } catch (...) {
      close(fd_);
      throw;
    }
  }
  ~RetirementWake() {
    loop_->removeWatch(fd_);
    close(fd_);
  }
  auto send() noexcept -> void {
    const std::uint64_t one = 1;
    while (write(fd_, &one, sizeof(one)) < 0 && errno == EINTR) {
    }
  }

 private:
  mln::util::RunLoop* loop_;
  int fd_;
};
#elif defined(__EMSCRIPTEN__)
class RetirementWake {
  struct State : mln::platform::emscripten::RunLoopWake::Runnable {
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
          static_cast<mln::platform::emscripten::RunLoopWake*>(
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
  mln::platform::emscripten::RunLoopWake* wake_;
  std::shared_ptr<State> state_;
};
#else
using RetirementWake = mln::util::AsyncTask;
#endif

}  // namespace mln::core
