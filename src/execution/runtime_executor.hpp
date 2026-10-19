#pragma once

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <utility>

#include <mln/util/run_loop.hpp>

#include "c_api/autorelease_pool.hpp"
#include "execution/retirement.hpp"
#include "execution/retirement_wake.hpp"
#include "execution/worker_thread.hpp"

namespace mln::core {

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
  // Called on the worker after every lease and operation has retired. Runs
  // completion after the run loop is destroyed and the worker is detached;
  // completion may destroy this executor.
  auto retire(RetirementTask& completion) noexcept -> void;
  // Queues task without allocating. The caller must guarantee the executor has
  // not stopped: a map holds MapObject::runtime_cleanup_lease until it is
  // destroyed, which keeps runtime disposal from draining and stopping this
  // executor, and runtime disposal itself queues before it calls retire().
  auto invoke_retirement(RetirementTask& task) noexcept -> void;

  [[nodiscard]] auto run_loop() const noexcept -> mln::util::RunLoop* {
    return run_loop_.load();
  }

  template <typename Function>
  auto invoke(Function&& function) -> void {
    auto* loop = run_loop();
    if (loop == nullptr) {
      throw std::runtime_error{"runtime executor is not running"};
    }
    loop->invoke([work = std::forward<Function>(function)]() mutable -> void {
      auto pooled = [&work]() -> mln_status {
        std::invoke(std::move(work));
        return MLN_STATUS_OK;
      };
      static_cast<void>(mln::c_api::with_autorelease_pool(pooled));
    });
  }

 private:
  std::mutex mutex_;
  std::condition_variable started_;
  WorkerThread worker_;
  std::atomic<mln::util::RunLoop*> run_loop_ = nullptr;
  std::exception_ptr startup_error_;
  bool starting_ = false;
  bool stopping_ = false;
  RetirementQueue retirement_queue_;
  RetirementWake* retirement_wake_ = nullptr;
  RetirementTask* retirement_completion_ = nullptr;
  auto drain_retirement() noexcept -> void;
};

}  // namespace mln::core
