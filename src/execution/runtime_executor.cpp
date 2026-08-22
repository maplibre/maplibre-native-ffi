#include <exception>
#include <utility>

#include "execution/runtime_executor.hpp"

namespace mln::core {

RuntimeExecutor::~RuntimeExecutor() { stop(); }

auto RuntimeExecutor::start(std::function<void()> initialize) -> void {
  {
    const std::scoped_lock lock(mutex_);
    if (starting_ || run_loop_ != nullptr || worker_.joinable()) {
      throw std::runtime_error{"runtime executor is already running"};
    }
    starting_ = true;
    startup_error_ = nullptr;
  }

  worker_ = std::thread(
    [this, initialize = std::move(initialize)]() mutable noexcept -> void {
      try {
        auto loop = mln::util::RunLoop{mln::util::RunLoop::Type::New};
        {
          const std::scoped_lock lock(mutex_);
          worker_id_ = std::this_thread::get_id();
          run_loop_ = &loop;
        }
        if (initialize) {
          auto pooled = [&initialize]() -> mln_status {
            std::invoke(std::move(initialize));
            return MLN_STATUS_OK;
          };
          static_cast<void>(mln::c_api::with_autorelease_pool(pooled));
        }
        {
          const std::scoped_lock lock(mutex_);
          starting_ = false;
        }
        started_.notify_all();
        loop.run();
        {
          const std::scoped_lock lock(mutex_);
          run_loop_ = nullptr;
          worker_id_ = {};
        }
      } catch (...) {
        {
          const std::scoped_lock lock(mutex_);
          startup_error_ = std::current_exception();
          run_loop_ = nullptr;
          worker_id_ = {};
          starting_ = false;
        }
        started_.notify_all();
      }
    }
  );

  auto lock = std::unique_lock{mutex_};
  started_.wait(lock, [this]() noexcept -> bool { return !starting_; });
  if (startup_error_ != nullptr) {
    auto error = std::exchange(startup_error_, nullptr);
    lock.unlock();
    if (worker_.joinable()) {
      worker_.join();
    }
    std::rethrow_exception(error);
  }
}

auto RuntimeExecutor::stop() noexcept -> void {
  auto* loop = run_loop();
  if (loop != nullptr) {
    loop->invoke([loop]() noexcept -> void { loop->stop(); });
  }
  if (worker_.joinable()) {
    if (worker_.get_id() == std::this_thread::get_id()) {
      worker_.detach();
    } else {
      worker_.join();
    }
  }
}

}  // namespace mln::core
