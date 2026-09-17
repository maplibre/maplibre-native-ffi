#pragma once

#include <functional>
#include <thread>

#if !defined(_WIN32)
#include <pthread.h>
#endif

namespace mln::core {

// A joinable thread for native workers that run map, style, and render work.
//
// std::thread takes the platform default stack, which musl fixes at 128 KiB
// and Apple platforms at 512 KiB for secondary threads. MapLibre's style and
// tile code assumes the main-thread-sized stacks of the host applications it
// was written for, so every worker here reserves at least kStackSize.
class WorkerThread {
 public:
  static constexpr std::size_t kStackSize = std::size_t{8} << 20;

  WorkerThread() = default;
  // Throws std::system_error when the thread cannot be created.
  explicit WorkerThread(std::function<void()> body);
  WorkerThread(WorkerThread&& other) noexcept;
  auto operator=(WorkerThread&& other) noexcept -> WorkerThread&;
  WorkerThread(const WorkerThread&) = delete;
  auto operator=(const WorkerThread&) -> WorkerThread& = delete;
  // Terminates when the thread is still joinable, like std::thread.
  ~WorkerThread();

  [[nodiscard]] auto joinable() const noexcept -> bool;
  // Whether the calling thread is this worker.
  [[nodiscard]] auto is_current() const noexcept -> bool;
  auto join() -> void;
  auto detach() -> void;

 private:
#if defined(_WIN32) || defined(__EMSCRIPTEN__)
  std::thread thread_;
#else
  pthread_t handle_{};
  bool joinable_ = false;
#endif
};

}  // namespace mln::core
