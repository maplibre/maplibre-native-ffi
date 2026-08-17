#include <chrono>
#include <cstring>
#include <utility>

#include "operation/operation.hpp"

#include "diagnostics/diagnostics.hpp"
#include "handles/handle_table.hpp"

namespace mln::core {

template <>
struct HandleTraits<OperationObject> {
  static constexpr auto kind = HandleKind::Operation;
  static constexpr auto leasable = true;
};

OperationObject::OperationObject(
  std::uint32_t kind, bool cancellable, CancelCallback cancel
)
    : kind_(kind),
      cancellable_(cancellable),
      cancel_callback_(std::move(cancel)) {}

auto OperationObject::publish(
  std::shared_ptr<NotificationEndpoint> endpoint
) noexcept -> void {
  const std::scoped_lock lock(mutex_);
  endpoint_ = std::move(endpoint);
}

auto OperationObject::set_terminal_callback(TerminalCallback callback) noexcept
  -> void {
  auto invoke_now = false;
  {
    const std::scoped_lock lock(mutex_);
    if (completed_) {
      invoke_now = true;
    } else {
      terminal_callback_ = std::move(callback);
    }
  }
  if (invoke_now && callback) {
    try {
      callback();
    } catch (...) {
      // Terminal observation cannot reopen a completed operation.
    }
  }
}

auto OperationObject::complete(
  mln_status status, std::string diagnostic, std::any result
) noexcept -> void {
  auto endpoint = std::shared_ptr<NotificationEndpoint>{};
  auto terminal = TerminalCallback{};
  {
    const std::scoped_lock lock(mutex_);
    if (completed_) {
      return;
    }
    completed_ = true;
    status_ = status;
    diagnostic_ = std::move(diagnostic);
    result_ = std::move(result);
    result_available_ = true;
    cancel_callback_ = {};
    terminal = std::move(terminal_callback_);
    if (observer_attached_) {
      endpoint = endpoint_;
    } else {
      result_.reset();
      result_available_ = false;
    }
  }
  condition_.notify_all();
  if (endpoint != nullptr) {
    endpoint->mark_ready();
  }
  if (terminal) {
    try {
      terminal();
    } catch (...) {
      // Terminal observation cannot reopen a completed operation.
    }
  }
}

auto OperationObject::finish_cancelled_locked(
  std::shared_ptr<NotificationEndpoint>& out_endpoint,
  CancelCallback& out_cancel, TerminalCallback& out_terminal
) noexcept -> void {
  completed_ = true;
  status_ = MLN_STATUS_CANCELLED;
  diagnostic_ = "operation was cancelled";
  result_ = std::monostate{};
  result_available_ = true;
  out_cancel = std::move(cancel_callback_);
  out_terminal = std::move(terminal_callback_);
  if (observer_attached_) {
    out_endpoint = endpoint_;
  } else {
    result_.reset();
    result_available_ = false;
  }
}

auto OperationObject::cancel() -> mln_status {
  auto endpoint = std::shared_ptr<NotificationEndpoint>{};
  auto cancel = CancelCallback{};
  auto terminal = TerminalCallback{};
  {
    const std::scoped_lock lock(mutex_);
    if (completed_) {
      set_thread_error("operation is already completed");
      return MLN_STATUS_INVALID_STATE;
    }
    if (!cancellable_) {
      set_thread_error("operation does not support cancellation");
      return MLN_STATUS_UNSUPPORTED;
    }
    finish_cancelled_locked(endpoint, cancel, terminal);
  }
  condition_.notify_all();
  if (cancel) {
    try {
      cancel();
    } catch (...) {
      // Cancellation is already terminal and must not be reopened.
    }
  }
  if (endpoint != nullptr) {
    endpoint->mark_ready();
  }
  if (terminal) {
    try {
      terminal();
    } catch (...) {
      // Cancellation is already terminal.
    }
  }
  return MLN_STATUS_OK;
}

auto OperationObject::poll(bool* out_completed) -> mln_status {
  if (out_completed == nullptr) {
    set_thread_error("out_completed must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const std::scoped_lock lock(mutex_);
  *out_completed = completed_;
  return MLN_STATUS_OK;
}

auto OperationObject::wait(std::int64_t timeout_ms, bool* out_completed)
  -> mln_status {
  if (out_completed == nullptr) {
    set_thread_error("out_completed must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  auto lock = std::unique_lock{mutex_};
  if (timeout_ms < 0) {
    condition_.wait(lock, [this]() noexcept -> bool { return completed_; });
  } else if (timeout_ms > 0) {
    static_cast<void>(condition_.wait_for(
      lock, std::chrono::milliseconds{timeout_ms},
      [this]() noexcept -> bool { return completed_; }
    ));
  }
  *out_completed = completed_;
  return MLN_STATUS_OK;
}

auto OperationObject::get_status(mln_status* out_status) -> mln_status {
  if (out_status == nullptr) {
    set_thread_error("out_status must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const std::scoped_lock lock(mutex_);
  if (!completed_) {
    set_thread_error("operation is still pending");
    return MLN_STATUS_INVALID_STATE;
  }
  *out_status = status_;
  return MLN_STATUS_OK;
}

auto OperationObject::copy_diagnostic(
  char* out_diagnostic, std::size_t diagnostic_capacity,
  std::size_t* out_diagnostic_size
) -> mln_status {
  if (out_diagnostic_size == nullptr) {
    set_thread_error("out_diagnostic_size must not be null");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (out_diagnostic == nullptr && diagnostic_capacity != 0) {
    set_thread_error(
      "out_diagnostic must not be null when capacity is nonzero"
    );
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  const std::scoped_lock lock(mutex_);
  if (!completed_) {
    set_thread_error("operation is still pending");
    return MLN_STATUS_INVALID_STATE;
  }
  *out_diagnostic_size = diagnostic_.size();
  if (out_diagnostic == nullptr) {
    return MLN_STATUS_OK;
  }
  if (diagnostic_capacity < diagnostic_.size()) {
    set_thread_error("out_diagnostic capacity is too small");
    return MLN_STATUS_INVALID_ARGUMENT;
  }
  if (!diagnostic_.empty()) {
    std::memcpy(out_diagnostic, diagnostic_.data(), diagnostic_.size());
  }
  return MLN_STATUS_OK;
}

auto OperationObject::discard_result() -> mln_status {
  const std::scoped_lock lock(mutex_);
  if (!completed_) {
    set_thread_error("operation is still pending");
    return MLN_STATUS_INVALID_STATE;
  }
  if (!result_available_) {
    set_thread_error("operation result was already taken or discarded");
    return MLN_STATUS_INVALID_STATE;
  }
  result_.reset();
  result_available_ = false;
  return MLN_STATUS_OK;
}

auto OperationObject::release_observer() noexcept -> void {
  auto endpoint = std::shared_ptr<NotificationEndpoint>{};
  auto cancel = CancelCallback{};
  auto terminal = TerminalCallback{};
  {
    const std::scoped_lock lock(mutex_);
    if (!observer_attached_) {
      return;
    }
    observer_attached_ = false;
    endpoint = std::move(endpoint_);
    if (!completed_ && cancellable_) {
      finish_cancelled_locked(endpoint, cancel, terminal);
    }
    result_.reset();
    result_available_ = false;
  }
  condition_.notify_all();
  if (endpoint != nullptr) {
    endpoint->detach();
  }
  if (cancel) {
    try {
      cancel();
    } catch (...) {
      // Observer release is unconditional.
    }
  }
  if (terminal) {
    try {
      terminal();
    } catch (...) {
      // Observer release is unconditional.
    }
  }
  endpoint.reset();
}

auto OperationObject::set_operation_error(const char* message) noexcept
  -> void {
  set_thread_error(message);
}

auto lease_operation(mln_operation operation)
  -> std::shared_ptr<OperationObject> {
  return handle_table<OperationObject>().lease(operation);
}

auto register_operation(
  const std::shared_ptr<NotificationSourceObject>& source, std::uint32_t kind,
  bool cancellable, OperationObject::CancelCallback cancel,
  mln_operation* out_operation, std::shared_ptr<OperationObject>& out_state
) -> mln_status {
  if (source == nullptr) {
    set_thread_error("operation notification source is unavailable");
    return MLN_STATUS_INVALID_STATE;
  }
  if (out_operation == nullptr || *out_operation != MLN_HANDLE_NULL) {
    set_thread_error("out_operation must point to the null handle");
    return MLN_STATUS_INVALID_ARGUMENT;
  }

  auto state =
    std::make_shared<OperationObject>(kind, cancellable, std::move(cancel));
  const auto handle = handle_table<OperationObject>().insert(state);
  try {
    auto endpoint =
      source->associate(handle, MLN_NOTIFICATION_ENDPOINT_OPERATION, false);
    if (endpoint == nullptr) {
      static_cast<void>(handle_table<OperationObject>().remove(handle));
      return MLN_STATUS_INVALID_STATE;
    }
    state->publish(std::move(endpoint));
  } catch (...) {
    static_cast<void>(handle_table<OperationObject>().remove(handle));
    throw;
  }
  *out_operation = handle;
  out_state = std::move(state);
  return MLN_STATUS_OK;
}

auto abandon_operation(mln_operation operation) noexcept -> void {
  auto state = handle_table<OperationObject>().remove(operation);
  if (state != nullptr) {
    state->release_observer();
  }
}

auto poll_operation(mln_operation operation, bool* out_completed)
  -> mln_status {
  const auto live = lease_operation(operation);
  return live == nullptr ? MLN_STATUS_INVALID_ARGUMENT
                         : live->poll(out_completed);
}

auto wait_operation(
  mln_operation operation, std::int64_t timeout_ms, bool* out_completed
) -> mln_status {
  const auto live = lease_operation(operation);
  return live == nullptr ? MLN_STATUS_INVALID_ARGUMENT
                         : live->wait(timeout_ms, out_completed);
}

auto cancel_operation(mln_operation operation) -> mln_status {
  const auto live = lease_operation(operation);
  return live == nullptr ? MLN_STATUS_INVALID_ARGUMENT : live->cancel();
}

auto get_operation_status(mln_operation operation, mln_status* out_status)
  -> mln_status {
  const auto live = lease_operation(operation);
  return live == nullptr ? MLN_STATUS_INVALID_ARGUMENT
                         : live->get_status(out_status);
}

auto copy_operation_diagnostic(
  mln_operation operation, char* out_diagnostic,
  std::size_t diagnostic_capacity, std::size_t* out_diagnostic_size
) -> mln_status {
  const auto live = lease_operation(operation);
  return live == nullptr
           ? MLN_STATUS_INVALID_ARGUMENT
           : live->copy_diagnostic(
               out_diagnostic, diagnostic_capacity, out_diagnostic_size
             );
}

auto discard_operation_result(mln_operation operation) -> mln_status {
  const auto live = lease_operation(operation);
  return live == nullptr ? MLN_STATUS_INVALID_ARGUMENT : live->discard_result();
}

auto release_operation(mln_operation operation) noexcept -> void {
  auto live = handle_table<OperationObject>().remove(operation);
  if (live != nullptr) {
    live->release_observer();
  }
}

}  // namespace mln::core
