#include <algorithm>
#include <exception>
#include <memory>
#include <system_error>
#include <utility>

#include "execution/worker_thread.hpp"

namespace mln::core {

#if defined(_WIN32) || defined(__EMSCRIPTEN__)

// Windows sizes thread stacks from the executable's own header, and
// Emscripten sizes them from the link-time pthread stack setting, so both keep
// std::thread.
WorkerThread::WorkerThread(std::function<void()> body)
    : thread_{std::move(body)} {}

WorkerThread::WorkerThread(WorkerThread&& other) noexcept = default;

auto WorkerThread::operator=(WorkerThread&& other) noexcept
  -> WorkerThread& = default;

WorkerThread::~WorkerThread() = default;

auto WorkerThread::joinable() const noexcept -> bool {
  return thread_.joinable();
}

auto WorkerThread::is_current() const noexcept -> bool {
  return thread_.get_id() == std::this_thread::get_id();
}

auto WorkerThread::join() -> void { thread_.join(); }

auto WorkerThread::detach() -> void { thread_.detach(); }

#else

namespace {

auto run_worker(void* opaque) -> void* {
  auto body = std::unique_ptr<std::function<void()> >{
    static_cast<std::function<void()>*>(opaque)
  };
  (*body)();
  return nullptr;
}

}  // namespace

WorkerThread::WorkerThread(std::function<void()> body) {
  auto attributes = pthread_attr_t{};
  if (const auto error = pthread_attr_init(&attributes); error != 0) {
    throw std::system_error{
      error, std::generic_category(), "initializing worker thread attributes"
    };
  }
  auto stack_size = std::size_t{0};
  static_cast<void>(pthread_attr_getstacksize(&attributes, &stack_size));
  static_cast<void>(
    pthread_attr_setstacksize(&attributes, std::max(stack_size, kStackSize))
  );
  auto call = std::make_unique<std::function<void()> >(std::move(body));
  const auto error =
    pthread_create(&handle_, &attributes, run_worker, call.get());
  pthread_attr_destroy(&attributes);
  if (error != 0) {
    throw std::system_error{
      error, std::generic_category(), "creating a worker thread"
    };
  }
  static_cast<void>(call.release());
  joinable_ = true;
}

WorkerThread::WorkerThread(WorkerThread&& other) noexcept
    : handle_{other.handle_}, joinable_{other.joinable_} {
  other.joinable_ = false;
}

auto WorkerThread::operator=(WorkerThread&& other) noexcept -> WorkerThread& {
  if (this != &other) {
    if (joinable_) {
      std::terminate();
    }
    handle_ = other.handle_;
    joinable_ = other.joinable_;
    other.joinable_ = false;
  }
  return *this;
}

WorkerThread::~WorkerThread() {
  if (joinable_) {
    std::terminate();
  }
}

auto WorkerThread::joinable() const noexcept -> bool { return joinable_; }

auto WorkerThread::is_current() const noexcept -> bool {
  return joinable_ && pthread_equal(handle_, pthread_self()) != 0;
}

auto WorkerThread::detach() -> void {
  if (!joinable_) {
    throw std::system_error{
      std::make_error_code(std::errc::invalid_argument),
      "detaching a worker thread that is not joinable"
    };
  }
  joinable_ = false;
  static_cast<void>(pthread_detach(handle_));
}

auto WorkerThread::join() -> void {
  if (!joinable_) {
    throw std::system_error{
      std::make_error_code(std::errc::invalid_argument),
      "joining a worker thread that is not joinable"
    };
  }
  joinable_ = false;
  static_cast<void>(pthread_join(handle_, nullptr));
}

#endif

}  // namespace mln::core
