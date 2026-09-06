#include <atomic>
#include <functional>
#include <memory>

#include <mln/util/async_task.hpp>
#include <mln/util/run_loop.hpp>

#include "run_loop_wake.hpp"

namespace mln {
namespace util {

class AsyncTaskState : public platform::emscripten::RunLoopWake::Runnable,
                       public std::enable_shared_from_this<AsyncTaskState> {
 public:
  explicit AsyncTaskState(std::function<void()> fn)
      : wake(
          static_cast<platform::emscripten::RunLoopWake*>(
            RunLoop::getLoopHandle()
          )
        ),
        task(std::move(fn)) {}

  void cancel() {
    alive = false;
    wake->removeRunnable(shared_from_this());
  }

  void maySend() {
    if (!queued.exchange(true)) {
      wake->addRunnable(shared_from_this());
    }
  }

  auto dueTime() const -> mln::TimePoint override { return mln::Clock::now(); }

  auto countsForWaitForEmpty() const -> bool override { return true; }

  // Unlists first: a listed task counts against RunLoop::waitForEmpty(), so one
  // that ran and stayed listed would keep that loop spinning.
  void runTask() override {
    wake->removeRunnable(shared_from_this());
    if (!queued.exchange(false)) {
      return;
    }
    if (alive) {
      task();
    }
  }

 private:
  platform::emscripten::RunLoopWake* wake;
  std::atomic<bool> queued{false};
  std::atomic<bool> alive{true};
  std::function<void()> task;
};

class AsyncTask::Impl {
 public:
  explicit Impl(std::function<void()> fn)
      : state(std::make_shared<AsyncTaskState>(std::move(fn))) {}

  ~Impl() { state->cancel(); }

  void maySend() { state->maySend(); }

 private:
  std::shared_ptr<AsyncTaskState> state;
};

AsyncTask::AsyncTask(std::function<void()>&& fn)
    : impl(std::make_unique<Impl>(std::move(fn))) {}

AsyncTask::~AsyncTask() = default;

void AsyncTask::send() { impl->maySend(); }

}  // namespace util
}  // namespace mln
