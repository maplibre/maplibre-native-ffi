#include <mbgl/util/run_loop.hpp>
#include <mbgl/util/timer.hpp>

#include <emscripten.h>
#include <emscripten/eventloop.h>
#include <emscripten/threading.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace mbgl {
namespace util {

class Timer::Impl {
 public:
  void start(uint64_t timeout, uint64_t repeat, std::function<void()>&& cb_) {
    stop();
    cb = std::move(cb_);
    repeat_ms_ = repeat;

    if (emscripten_is_main_browser_thread()) {
      startOnMainThread(timeout, repeat);
      return;
    }

    startOnWorkerThread(timeout, repeat);
  }

  void stop() {
    if (handle_ != 0) {
      if (repeat_ms_ > 0) {
        emscripten_clear_interval(handle_);
      } else {
        emscripten_clear_timeout(handle_);
      }
      handle_ = 0;
    }

    cancelled_ = true;
    if (worker_.joinable()) {
      worker_.join();
    }
    cancelled_ = false;

    cb = nullptr;
    repeat_ms_ = 0;
  }

 private:
  void startOnMainThread(uint64_t timeout, uint64_t repeat) {
    if (repeat > 0) {
      handle_ = emscripten_set_interval(
        [](void* userdata) {
          auto* self = static_cast<Impl*>(userdata);
          if (self->cb) {
            self->cb();
          }
        },
        static_cast<double>(timeout),
        this
      );
      return;
    }

    handle_ = emscripten_set_timeout(
      [](void* userdata) {
        auto* self = static_cast<Impl*>(userdata);
        if (self->cb) {
          self->cb();
        }
        self->handle_ = 0;
      },
      static_cast<double>(timeout),
      this
    );
  }

  void startOnWorkerThread(uint64_t timeout, uint64_t repeat) {
    auto* loop = RunLoop::Get();
    cancelled_ = false;
    worker_ = std::thread([this, loop, timeout, repeat]() {
      uint64_t delay = timeout;
      while (!cancelled_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        if (cancelled_) {
          return;
        }

        loop->invoke([this]() {
          if (cb) {
            cb();
          }
        });

        if (cancelled_ || repeat == 0) {
          return;
        }

        delay = repeat;
      }
    });
  }

  std::function<void()> cb;
  int handle_ = 0;
  uint64_t repeat_ms_ = 0;
  std::atomic<bool> cancelled_{false};
  std::thread worker_;
};

Timer::Timer() : impl(std::make_unique<Impl>()) {}

Timer::~Timer() = default;

void Timer::start(Duration timeout, Duration repeat, std::function<void()>&& cb) {
  impl->start(
    static_cast<uint64_t>(std::chrono::duration_cast<Milliseconds>(timeout).count()),
    static_cast<uint64_t>(std::chrono::duration_cast<Milliseconds>(repeat).count()),
    std::move(cb)
  );
}

void Timer::stop() { impl->stop(); }

}  // namespace util
}  // namespace mbgl
