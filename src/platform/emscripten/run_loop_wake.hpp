#pragma once

#include <condition_variable>
#include <mutex>

namespace mbgl::platform::emscripten {

struct RunLoopWake {
  std::mutex mutex;
  std::condition_variable cv;

  void notify() {
    std::lock_guard lock(mutex);
    cv.notify_one();
  }
};

// Set while constructing an AsyncTask so it can capture the owning RunLoop wake
// state without changing the public AsyncTask API.
inline RunLoopWake* pending_wake_for_async_task = nullptr;

}  // namespace mbgl::platform::emscripten
