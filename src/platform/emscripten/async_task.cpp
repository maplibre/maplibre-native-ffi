#include <mbgl/util/async_task.hpp>

#include <emscripten.h>
#include <emscripten/threading.h>

#include <atomic>
#include <functional>

#include "run_loop_wake.hpp"

namespace mbgl {
namespace util {

class AsyncTask::Impl {
 public:
  explicit Impl(std::function<void()> fn) : task(std::move(fn)) {
    wake = platform::emscripten::pending_wake_for_async_task;
  }

  void maySend() {
    if (emscripten_is_main_browser_thread()) {
      if (scheduled.exchange(true)) {
        return;
      }

      emscripten_async_call(
        [](void* userdata) {
          auto* self = static_cast<Impl*>(userdata);
          self->scheduled = false;
          self->task();
        },
        this,
        0
      );
      return;
    }

    if (wake != nullptr) {
      wake->notify();
    }
  }

 private:
  std::function<void()> task;
  platform::emscripten::RunLoopWake* wake = nullptr;
  std::atomic<bool> scheduled{false};
};

AsyncTask::AsyncTask(std::function<void()>&& fn)
    : impl(std::make_unique<Impl>(std::move(fn))) {}

AsyncTask::~AsyncTask() = default;

void AsyncTask::send() { impl->maySend(); }

}  // namespace util
}  // namespace mbgl
