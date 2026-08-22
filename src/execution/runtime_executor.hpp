#pragma once

#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>

#include <mbgl/util/run_loop.hpp>

#include "c_api/autorelease_pool.hpp"

namespace mln::core {

// Owns one continuously running MapLibre run loop and its native thread.
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

  [[nodiscard]] auto is_worker_thread() const noexcept -> bool {
    const std::scoped_lock lock(mutex_);
    return std::this_thread::get_id() == worker_id_;
  }

  [[nodiscard]] auto run_loop() const noexcept -> mln::util::RunLoop* {
    const std::scoped_lock lock(mutex_);
    return run_loop_;
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

  template <typename Function>
  auto invoke_sync(Function&& function) -> std::invoke_result_t<Function> {
    using Result = std::invoke_result_t<Function>;
    if (is_worker_thread()) {
      if constexpr (std::is_void_v<Result>) {
        std::invoke(std::forward<Function>(function));
        return;
      } else {
        return std::invoke(std::forward<Function>(function));
      }
    }

    auto promise = std::make_shared<std::promise<Result>>();
    auto result = promise->get_future();
    invoke(
      [promise, function = std::forward<Function>(function)]() mutable -> void {
        try {
          if constexpr (std::is_void_v<Result>) {
            std::invoke(std::move(function));
            promise->set_value();
          } else {
            promise->set_value(std::invoke(std::move(function)));
          }
        } catch (...) {
          promise->set_exception(std::current_exception());
        }
      }
    );
    return result.get();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable started_;
  std::thread worker_;
  std::thread::id worker_id_;
  mln::util::RunLoop* run_loop_ = nullptr;
  std::exception_ptr startup_error_;
  bool starting_ = false;
};

}  // namespace mln::core
