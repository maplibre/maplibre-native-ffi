#pragma once

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <mln/util/run_loop.hpp>

#include "c_api/autorelease_pool.hpp"
#include "execution/retirement.hpp"
#include "execution/worker_thread.hpp"

namespace mln::core {

struct ExecutorTask {
  ExecutorTask* next = nullptr;
  void (*run)(ExecutorTask*) noexcept = nullptr;
  bool command = false;
};

class ExecutorQueue {
 public:
  auto push(ExecutorTask& task) noexcept -> void {
    if (last_)
      last_->next = &task;
    else
      first_ = &task;
    last_ = &task;
  }
  auto pop() noexcept -> ExecutorTask* {
    auto* task = first_;
    if (task) {
      first_ = task->next;
      if (!first_) last_ = nullptr;
      task->next = nullptr;
    }
    return task;
  }

 private:
  ExecutorTask* first_ = nullptr;
  ExecutorTask* last_ = nullptr;
};

class RuntimeExecutor {
 public:
  RuntimeExecutor() = default;
  RuntimeExecutor(const RuntimeExecutor&) = delete;
  RuntimeExecutor(RuntimeExecutor&&) = delete;
  auto operator=(const RuntimeExecutor&) -> RuntimeExecutor& = delete;
  auto operator=(RuntimeExecutor&&) -> RuntimeExecutor& = delete;
  ~RuntimeExecutor();

  auto start(std::function<void()> initialize = std::function<void()>{})
    -> void;
  auto stop() noexcept -> void;
  // Called on the worker after every lease and operation has retired. The
  // shared finalizer runs completion, which must join the worker through
  // stop() before destroying this executor or reporting runtime release.
  auto retire(RetirementTask& completion) noexcept -> void;

  [[nodiscard]] auto run_loop() const noexcept -> mln::util::RunLoop* {
    return run_loop_.load();
  }

  template <typename Function>
  auto invoke(Function&& function) -> void {
    auto task = make_task(std::forward<Function>(function));
    enqueue(*task);
    static_cast<void>(task.release());
  }

  template <typename Function>
  auto invoke_notice(Function&& function) -> void {
    auto task = make_task(std::forward<Function>(function));
    enqueue(*task, true);
    static_cast<void>(task.release());
  }

  auto enqueue(ExecutorTask& task, bool notice = false) -> void;

 private:
  template <typename Function>
  struct Task : ExecutorTask {
    explicit Task(Function function) : work(std::move(function)) {
      run = [](ExecutorTask* base) noexcept {
        auto owner = std::unique_ptr<Task>{static_cast<Task*>(base)};
        try {
          std::invoke(std::move(owner->work));
        } catch (...) {
        }
      };
    }
    Function work;
  };
  template <typename Function>
  static auto make_task(Function&& function) {
    return std::make_unique<Task<std::decay_t<Function>>>(
      std::forward<Function>(function)
    );
  }
  std::mutex mutex_;
  std::condition_variable started_;
  WorkerThread worker_;
  std::atomic<mln::util::RunLoop*> run_loop_ = nullptr;
  std::exception_ptr startup_error_;
  bool starting_ = false;
  bool stopping_ = false;
  ExecutorQueue work_queue_;
  ExecutorQueue notices_;
  bool work_scheduled_ = false;
  bool draining_ = false;
  auto drain_work() noexcept -> void;
  auto flush_notices() noexcept -> void;
};

}  // namespace mln::core
