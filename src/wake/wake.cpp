#include <utility>

#include "wake/wake.hpp"

#include "diagnostics/diagnostics.hpp"
#include "execution/process_exit.hpp"

namespace mln::core {
namespace {

// One pending invocation of a wake that names a queue.
struct WakeCall final : delivery::DeliveryItem {
  std::shared_ptr<std::atomic_bool> pending;
};

auto make_wake_call(
  mln_wake_callback callback, void* user_data,
  std::shared_ptr<std::atomic_bool> pending
) -> delivery::ItemPtr {
  auto call = std::make_unique<WakeCall>();
  call->pending = std::move(pending);
  call->user_data = user_data;
  call->callback = reinterpret_cast<delivery::HostFunction>(callback);
  call->invoke = [](delivery::DeliveryItem& item) noexcept {
    // A notify from inside the callback queues the next call.
    static_cast<WakeCall&>(item).pending->store(false);
    reinterpret_cast<mln_wake_callback>(item.callback)(item.user_data);
  };
  call->destroy = [](delivery::DeliveryItem* item) noexcept {
    delete static_cast<WakeCall*>(item);
  };
  return delivery::ItemPtr{call.release()};
}

}  // namespace

Wake::Wake(
  const mln_wake& descriptor,
  std::shared_ptr<delivery::DeliveryQueueState> queue
)
    : descriptor_(descriptor), queue_(std::move(queue)) {
  if (queue_ == nullptr) return;
  call_pending_ = std::make_shared<std::atomic_bool>(false);
  if (descriptor_.release_user_data != nullptr) {
    release_call_ = delivery::make_release_call(
      descriptor_.user_data, descriptor_.release_user_data
    );
    held_ = delivery::hold_user_data(queue_, descriptor_.user_data);
  }
}

Wake::~Wake() {
  auto descriptor = mln_wake{};
  auto accepted = false;
  {
    auto lock = std::unique_lock{mutex_};
    closing_ = true;
    condition_.wait(lock, [this]() noexcept { return in_flight_ == 0; });
    descriptor = descriptor_;
    accepted = accepted_;
    descriptor_ = {};
  }
  if (
    !accepted || descriptor.release_user_data == nullptr || process_exiting()
  ) {
    return;
  }
  if (queue_ != nullptr) {
    // A released queue abandons the user data.
    static_cast<void>(queue_->push(release_call_, held_));
    return;
  }
  try {
    descriptor.release_user_data(descriptor.user_data);
  } catch (...) {
    // Host release callbacks must not unwind through the C boundary.
  }
}

auto Wake::accept() noexcept -> void {
  const auto lock = std::scoped_lock{mutex_};
  accepted_ = true;
}

auto Wake::reject() noexcept -> void {
  const auto lock = std::scoped_lock{mutex_};
  closing_ = true;
  descriptor_ = {};
}

auto Wake::close() noexcept -> void {
  const auto lock = std::scoped_lock{mutex_};
  closing_ = true;
}

auto Wake::notify() noexcept -> void {
  auto callback = mln_wake_callback{};
  auto* user_data = static_cast<void*>(nullptr);
  {
    const auto lock = std::scoped_lock{mutex_};
    if (
      !accepted_ || closing_ || descriptor_.callback == nullptr ||
      process_exiting()
    ) {
      return;
    }
    ++in_flight_;
    callback = descriptor_.callback;
    user_data = descriptor_.user_data;
  }
  if (queue_ != nullptr) {
    push_call(callback, user_data);
  } else {
    try {
      callback(user_data);
    } catch (...) {
      // Host wake callbacks must not unwind through the C boundary.
    }
  }
  {
    const auto lock = std::scoped_lock{mutex_};
    --in_flight_;
    if (closing_ && in_flight_ == 0) condition_.notify_all();
  }
}

auto Wake::push_call(mln_wake_callback callback, void* user_data) noexcept
  -> void {
  if (call_pending_->exchange(true)) return;
  try {
    auto call = make_wake_call(callback, user_data, call_pending_);
    if (queue_->push(call)) return;
  } catch (...) {
    // The next notify tries again.
  }
  call_pending_->store(false);
}

auto resolve_wake(
  const mln_wake* wake, std::shared_ptr<delivery::DeliveryQueueState>& out_queue
) -> mln_status {
  if (wake == nullptr) {
    set_thread_error("wake must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (wake->callback == nullptr) {
    if (wake->release_user_data != nullptr) {
      set_thread_error("a disabled wake must not retain user data");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    if (wake->queue != MLN_HANDLE_NULL) {
      set_thread_error("a disabled wake must not name a queue");
      return MLN_STATUS_INVALID_ARGUMENT;
    }
    return MLN_STATUS_OK;
  }
  if (wake->queue == MLN_HANDLE_NULL) return MLN_STATUS_OK;
  out_queue = delivery::resolve_delivery_queue(wake->queue);
  if (out_queue == nullptr) return recorded_handle_fault_status();
  return MLN_STATUS_OK;
}

}  // namespace mln::core
